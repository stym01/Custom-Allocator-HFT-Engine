#include <iostream>
#include <vector>
#include <string>
#include <algorithm> 

#include "../Includes/PoolAllocator.h"
#include "../Includes/LinearAllocator.h"

enum OrderType { BUY, SELL }; 

struct Order {
    int id;
    OrderType type;
    double price;
    int quantity;
    Order* next; //LL for the Order Book...

    Order(int i, OrderType t, double p, int q) 
        : id(i), type(t), price(p), quantity(q), next(nullptr) {}
};

class OrderBook {
private:
    PoolAllocator* orderPool;
    Order* buySideHead;  //sorted High to Low as highest bidder first...
    Order* sellSideHead; //sorted Low to High as lowest seller first...

public:
    OrderBook() {
        orderPool = new PoolAllocator(100000 * sizeof(Order), sizeof(Order), alignof(Order));
        orderPool->Init();
        buySideHead = nullptr;
        sellSideHead = nullptr;
    }

    ~OrderBook() {
        delete orderPool;
    }

    //hot path so no 'new', no 'malloc'...
    void ProcessOrder(int id, OrderType type, double price, int quantity) {
        
        if (type == BUY) {
            // Attempt to match with Sellers (sellSideHead)
            // Sellers are sorted Low-to-High. We want cheap sellers.
            while (sellSideHead != nullptr && sellSideHead->price <= price && quantity > 0) {
                int tradeQty = std::min(quantity, sellSideHead->quantity);
                
                std::cout << "[TRADE] MATCH! Buy Order " << id << " bought " << tradeQty 
                          << " units : " << sellSideHead->price << " from Seller " << sellSideHead->id << std::endl;
                
                quantity -= tradeQty;
                sellSideHead->quantity -= tradeQty;

                //remove filled sell order...
                if (sellSideHead->quantity == 0) {
                    Order* filled = sellSideHead;
                    sellSideHead = sellSideHead->next;
                    
                    filled->~Order();
                    orderPool->Deallocate(filled); 
                }
            }
        } 
        
        else { 
            // Attempt to match with Buyers (buySideHead)
            // Buyers are sorted High-to-Low. We want rich buyers.
            while (buySideHead != nullptr && buySideHead->price >= price && quantity > 0) {
                int tradeQty = std::min(quantity, buySideHead->quantity);
                
                std::cout << "[TRADE] MATCH! Sell Order " << id << " sold " << tradeQty 
                          << " units : " << buySideHead->price << " to Buyer " << buySideHead->id << std::endl;
                
                quantity -= tradeQty;
                buySideHead->quantity -= tradeQty;

                // Remove filled buy order
                if (buySideHead->quantity == 0) {
                    Order* filled = buySideHead;
                    buySideHead = buySideHead->next;
                    
                    filled->~Order();
                    orderPool->Deallocate(filled); 
                }
            }
        }
        // add rem to bookk...
        if (quantity > 0) {
            void* mem = orderPool->Allocate(sizeof(Order));
            Order* newOrder = new (mem) Order(id, type, price, quantity);
            
            if (type == BUY) {
                InsertBuyOrder(newOrder);
                std::cout << "[BOOK] BUY Order " << id << " placed @ " << price << std::endl;
            } else {
                InsertSellOrder(newOrder);
                std::cout << "[BOOK] SELL Order " << id << " placed @ " << price << std::endl;
            }
        }
    }

    // Insert High-to-Low
    void InsertBuyOrder(Order* ord) {
        if (!buySideHead || ord->price > buySideHead->price) {
            ord->next = buySideHead;
            buySideHead = ord;
        } else {
            Order* curr = buySideHead;
            while (curr->next && curr->next->price >= ord->price) {
                curr = curr->next;
            }
            ord->next = curr->next;
            curr->next = ord;
        }
    }

    // Insert Low-to-High
    void InsertSellOrder(Order* ord) {
        if (!sellSideHead || ord->price < sellSideHead->price) {
            ord->next = sellSideHead;
            sellSideHead = ord;
        } else {
            Order* curr = sellSideHead;
            while (curr->next && curr->next->price <= ord->price) {
                curr = curr->next;
            }
            ord->next = curr->next;
            curr->next = ord;
        }
    }
};

struct IncomingMessage {
    char symbol[4];
    int orderId;
    char side; // 'B' or 'S'
    double price;
    int qty;
};

int main() {
    // Linear Allocator for network packets
    LinearAllocator* msgBuffer = new LinearAllocator(1024 * 1024); 
    msgBuffer->Init();

    OrderBook engine;

    std::cout << "market open\n" << std::endl;

    // SIMULATION LOOP
    for (int i = 0; i < 6; ++i) {
        
        //Allocate packet memory (Fast Linear Allocation)
        void* pktMem = msgBuffer->Allocate(sizeof(IncomingMessage), alignof(IncomingMessage));
        IncomingMessage* msg = new (pktMem) IncomingMessage();
        
        msg->orderId = 100 + i;
        msg->side = (i % 2 == 0) ? 'B' : 'S';
        msg->qty = 10;
        
        // Prices designed to cross: Buys at 100, 101, 102... Sells at 99, 100, 101...
        msg->price = (msg->side == 'B') ? 100.0 + i : 100.0 + (i - 1); 

        //Process
        OrderType type = (msg->side == 'B') ? BUY : SELL;
        engine.ProcessOrder(msg->orderId, type, msg->price, msg->qty);

        //reset Linear Allocator for next packet
        msgBuffer->Reset(); 
    }

    std::cout << "\n MArket Closed" << std::endl;
    return 0;
}