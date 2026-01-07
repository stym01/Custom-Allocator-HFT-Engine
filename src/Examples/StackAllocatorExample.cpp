#include <iostream>
#include <vector>
#include <string>
#include "../../Includes/StackAllocator.h" 

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
    Allocator* myAllocator=new StackAllocator(POOL_SIZE);
    
    myAllocator->Init(); 
    std::cout << "Allocator initialized. Memory Pool Ready.\n" << std::endl;

    void* raw_memory = myAllocator->Allocate(sizeof(Player), alignof(Player));
    
    if (raw_memory == nullptr) {
        return -1; 
    }

    Player* p1 = new (raw_memory) Player(1, "Slayer");

    void* raw_memory2 = myAllocator->Allocate(sizeof(Player), alignof(Player));
    Player* p2 = new (raw_memory2) Player(2, "DragonBorn");

    std::cout << "\nUsing objects: " << p1->name << " and " << p2->name << std::endl;
    std::cout << "Memory Used: " << myAllocator->GetUsedMemory() << " bytes\n" << std::endl;

    //since stack allocator frees memory in strictly LIFO order hence 
    //if we delete the first el first it will misbehave like used_memory=96,0,48 byter if 1 then 2 deleted but it's 96,48,0 if 2 and then 1 deleted

    p2->~Player();
    myAllocator->Deallocate(p2); 
    std::cout << "\n Memory Used: " << myAllocator->GetUsedMemory() << " bytes" << std::endl;
    
    p1->~Player();
    myAllocator->Deallocate(p1);
    std::cout << "\n Memory Used: " << myAllocator->GetUsedMemory() << " bytes" << std::endl;

    myAllocator->Reset();
    std::cout << "\nAllocator reset. Memory Used: " << myAllocator->GetUsedMemory() << " bytes" << std::endl;
    delete myAllocator; //since i used new

    return 0;
}