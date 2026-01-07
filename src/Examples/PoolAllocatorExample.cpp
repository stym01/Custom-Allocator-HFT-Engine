#include <iostream>
#include <vector>
#include <string>
#include "../../Includes/PoolAllocator.h" 

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
    const size_t POOL_SIZE = 1024; 
    
    Allocator* playerPool = new PoolAllocator(POOL_SIZE, sizeof(Player), alignof(Player));
    
    playerPool->Init();

    void* slot1 = playerPool->Allocate(sizeof(Player));
    Player* p1 = new (slot1) Player(1, "A");

    void* slot2 = playerPool->Allocate(sizeof(Player));
    Player* p2 = new (slot2) Player(2, "B");

    // can free in RANDOM order
    p1->~Player();
    playerPool->Deallocate(p1);

    //reuse that slotand this should get the exact same memory address that p1 had
    void* slot3 = playerPool->Allocate(sizeof(Player));
    Player* p3 = new (slot3) Player(3, "C (Reused)");
    
    std::cout << "P1 Address (Freed): " << p1 << std::endl;
    std::cout << "P3 Address (New):   " << p3 << std::endl; //should be same

    delete playerPool;
    return 0;
}