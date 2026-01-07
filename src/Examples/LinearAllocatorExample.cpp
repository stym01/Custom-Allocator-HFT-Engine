#include <iostream>
#include <vector>
#include <string>
#include "../../Includes/LinearAllocator.h" 

class Player {
public:
    int id;
    std::string name;

    Player(int i, std::string n) : id(i), name(n) {
        std::cout << "[Constructor] Player " << name << " created at address " << this << std::endl;
    }

    ~Player() {
        std::cout << "[Destructor] Player " << name << " destroyed." << std::endl;
    }
};

int main() {
    
    const size_t POOL_SIZE = 1024 * 1024; // 1MB
    Allocator* myAllocator=new LinearAllocator(POOL_SIZE);
    
    //calling Init() to actually malloc the big chunk
    myAllocator->Init(); 
    std::cout << "Allocator initialized. Memory Pool Ready.\n" << std::endl;

    void* raw_memory = myAllocator->Allocate(sizeof(Player), alignof(Player));
    std::cout<<"l";
    if (raw_memory == nullptr) {
        return -1; 
    }
    std::cout<<"K";
    // Syntax: new (address) Type(arguments);
    Player* p1 = new (raw_memory) Player(1, "Voldemort"); //build player at 'raw_memory' addr


    void* raw_memory2 = myAllocator->Allocate(sizeof(Player), alignof(Player));
    Player* p2 = new (raw_memory2) Player(2, "Thanos");


    std::cout << "\nUsing objects: " << p1->name << " and " << p2->name << std::endl;
    std::cout << "Memory Used: " << myAllocator->GetUsedMemory() << " bytes\n" << std::endl;

    //destructn as we cannot use 'delete p1'. bcz 'delete' tries to free memory to the OS, but this memory belongs to our LinearAllocator soWe must manually call the destructor to clean up the std::string inside Player.

    p2->~Player();
    myAllocator->Deallocate(p2); //not needed
    p1->~Player();
    myAllocator->Deallocate(p1);

    myAllocator->Reset();
    std::cout << "\nAllocator reset. Memory Used: " << myAllocator->GetUsedMemory() << " bytes" << std::endl;
    delete myAllocator; //since i used new

    return 0;
}