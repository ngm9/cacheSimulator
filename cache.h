#ifndef CACHE_H
#define CACHE_H

#include <string>
#include <bitset>
#include <fstream>
#include <block.h>
#include <buffer.h>

using namespace std;

class CACHE{

  public:
    CACHE(uint32_t, uint32_t, uint8_t, uint32_t, uint32_t, CACHE*, const char*, bool);
    ~CACHE();
    void run(const char*);
    void read(uint32_t currAddr, bool isPrefetch);   
    void write(uint32_t currAddr);
    uint32_t calcBits(uint32_t _blockOffset);
    void printCache();
    void printBuf();

    uint32_t _nPref;
    uint32_t _mPref;
    
    // counters
    uint32_t _nReads; // (A)
    uint32_t _nReadMisses; 
    uint32_t _nReadMissBufMiss; //(B)
    uint32_t _nWrites; // (C)
    uint32_t _nWriteMisses;
    uint32_t _nWriteMissBufMiss; // (D) (M)
    uint32_t _nPrefetch; //(G) 
    uint32_t _nReadsPrefetch; // (J) 
    uint32_t _nReadsPrefetchBufMiss; // (K)
    uint32_t _nReadsNotPrefetched; //(H)
    uint32_t _nReadsMissesNotPrefetchBufMiss; // (I
    uint32_t _nWritesMissPrefetchHit;
    uint32_t _totalMemBlksTransported;
    uint32_t _nWriteBacks; // (F) (L)
    uint32_t _nWriteBacksToMem; // (O)
    uint32_t _nReadMissPrefHit;   
  
    uint32_t _b;
    uint32_t _nReadMissBufHit;
    uint32_t _nReadMissNotPrefetch; // I1
    uint32_t _nReadMissPrefetch;
  private:
    // constant properties
    uint32_t _size;
    uint32_t _assoc;
    uint8_t _blksize;
    uint32_t _nCacheSet;

    uint32_t _nBitsBlksize;
    uint32_t _nBitsBlockOffset;
    uint32_t _nBitsCacheSet;

    // state variables
    Block** _blockptr;
    Buffer* _bufptr;
    bitset<1>* _dirtyptr;
    bitset<1>* _validptr;
    uint32_t _currAddr;
    CACHE* _memBelow;
    uint32_t _blockOffset;
    uint32_t _indexBits;

};

#endif
