#include <cache.h>
#include <block.h>
#include <buffer.h>

using namespace std;

Buffer::Buffer(){
    _bufLRUcounter=0;
}

Buffer::~Buffer(){
    
}

void Buffer::initialize(int m, int nBitsBlksize){
    _m=m;
    _nBitsBlksize = nBitsBlksize;
    _buf=new Block[_m];
}

bool Buffer::checkFirst(uint32_t addr){
    if (_buf[0].isAddr(((addr >> _nBitsBlksize) << _nBitsBlksize)))
        if(_buf[0]._validBit.any())
            return true; //address matches and block is valid.
        else
            return false;
    else
        return false; // address does not match
}

void Buffer::shiftUp(){
    int i=0;
    for(i=0; i < (_m-1);i++){
        _buf[i]._addr=_buf[i+1]._addr;
        _buf[i]._tag=_buf[i+1]._tag;
        _buf[i]._validBit=_buf[i+1]._validBit;
        _buf[i]._dirtyBit=_buf[i+1]._dirtyBit;
        _buf[i]._LRUcounter=_buf[i+1]._LRUcounter;
    }
}


