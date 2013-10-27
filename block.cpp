#include <cache.h>
#include <block.h>

using namespace std;

Block::Block(){    
    _tag=0;
    _addr=0;
    _dirtyBit.reset();
    _validBit.reset();
    _LRUcounter=0;
}

Block::~Block(){
}

void Block::setTag(int tag){
    _tag=tag;
}

void Block::setAddr(uint32_t addr){
    _addr=addr;
}

bool Block::isTag(int tag){
    if (_tag==tag)
        return true;
    else
        return false;
}

bool Block::isAddr(uint32_t addr){
    if( _addr==addr)
        return true;
    else
        return false;
}

Block& Block::operator=(Block j){
    _tag=j._tag;
    _addr=j._addr;
    _dirtyBit=j._dirtyBit;
    _validBit=j._validBit;
    _LRUcounter=j._LRUcounter;

    return *this;
}


