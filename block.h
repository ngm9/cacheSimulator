#ifndef BLOCK_H
#define BLOCK_H

#include <bitset>

using namespace std;

class Block{

  public:
    Block();
    ~Block();
    void setTag(int tag);
    bool isTag(int tag);
    void setAddr(uint32_t addr);    
    bool isAddr(uint32_t addr);
    Block& operator=(Block);

    uint32_t _tag;
    uint32_t _addr;
    bitset<1> _dirtyBit;
    bitset<1> _validBit;
    int _LRUcounter;
};

#endif
