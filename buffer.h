#ifndef BUFFER_H
#define BUFFER_H

#include <block.h>
#include <cache.h>
#include <queue>

class Buffer{
  
    public:
        Buffer();
        ~Buffer();
        bool checkFirst(uint32_t);
        void initialize(int , int);
        void shiftUp();

        Block* _buf; 
        int _bufLRUcounter;
        int _m;
        int _nBitsBlksize;
};

#endif
