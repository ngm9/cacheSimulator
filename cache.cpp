#include <cache.h>
#include <iostream>
#include <fstream>
#include <bitset>
#include <sstream>
#include <cstdlib>
#include <buffer.h>

using namespace std;

CACHE::CACHE(uint32_t size, uint32_t assoc, uint8_t blksize, uint32_t nPref, uint32_t mPref, CACHE* memBelow, const char* trFile, bool isL1){

    register int i=0;
    _size=size;
    _assoc=assoc;
    _blksize=blksize;
    _memBelow=memBelow;
    _nCacheSet= (int)_size /(int) (_assoc * _blksize);
    _blockOffset=blksize-1;
    _indexBits=((_nCacheSet-1) * (_blksize));
    

    _nBitsBlksize=calcBits(_blksize);
    _nBitsCacheSet=calcBits(_nCacheSet);

    _nPref=nPref;
    _mPref=mPref;

    // initializing the cache
    _blockptr = new Block*[_nCacheSet];
    for(i=0; i < _nCacheSet; i++){
        _blockptr[i]=new Block[_assoc];
    }

    // initializing counter variables
    _nReads=0;
    _nReadMisses=0;
    _nWrites=0;
    _nPrefetch=0;
    _nWriteMisses=0;
    _nWriteMissBufMiss=0;
    _nReadsPrefetch=0;
    _nReadsPrefetchBufMiss=0;
    _nReadsNotPrefetched=0;
    _nReadsMissesNotPrefetchBufMiss=0;
    _nWritesMissPrefetchHit=0;
    _totalMemBlksTransported=0;
    _nWriteBacks=0;
    _nWriteBacksToMem=0;
    _nReadMissPrefHit=0;
    _b=0;
    _nReadMissBufHit=0;
    _nReadMissNotPrefetch=0;
    _nReadMissPrefetch=0;

    //initializing the stream buffer, if at all
    if(_nPref!=0){
        _bufptr=new Buffer[_nPref];
        for(i=0;i<_nPref;i++){
            _bufptr[i].initialize(_mPref, _nBitsBlksize);
        }
    }

    if(isL1){
        run(trFile);
        cout << "===== L1 contents =====" << endl;
        printCache();
        if(_nPref!=0){
            cout << "===== L1-SB contents =====" << endl;
            printBuf();
        }
        if(_memBelow!=NULL){
            cout << "===== L2 contents =====" << endl;
            _memBelow->printCache();
            if(_memBelow->_nPref!=0){
                cout << "===== L2-SB contents =====" << endl;
                _memBelow->printBuf();
            }
        }
    }
}

CACHE::~CACHE(){
    int i=0;
    for(i=0; i<_nCacheSet; i++){
        delete[] _blockptr[i];
    }

    delete _blockptr;
}

void CACHE::run(const char* trFile){

    register string tmp, mode, address;
    ifstream _inFile(trFile, ifstream::in);

    while(getline(_inFile, tmp)){

        istringstream t(tmp);
        t >> mode;
        t >> address;
       
//        cout << mode << " " << address << endl;
        _currAddr = (unsigned int)strtoul(address.c_str(), NULL, 16);
        if (mode=="r"){
            read(_currAddr, false); 
        }
        else{
            write(_currAddr);
        }
    }
}

uint32_t CACHE::calcBits(uint32_t number){
    
    int k=0;
    while(number){
        number=number >> 1;
        k++;
    }
    return (uint32_t)(k-1);
}

void CACHE::printCache(){
    int i=0, j=0, k=0;
    for(i=0; i < _nCacheSet ; i++){;
        cout << "Set" << "\t" << std::dec << i << ":" << "\t";
        for(j=0; j < _assoc; j++){
            for(k=0; k< _assoc; k++){
                if(_blockptr[i][k]._LRUcounter==j){
                    cout << std::hex << _blockptr[i][k]._tag << " "  ;
                    if (_blockptr[i][k]._dirtyBit.any())
                        cout << "D" << " " ;
                }
            }
        }
        cout << endl;
    }
}


void CACHE::printBuf(){
    int i=0, j=0, k=0;
    
    for(i=0; i< _nPref; i++){ 
        for(j=0; j< _nPref; j++){
            if(_bufptr[j]._bufLRUcounter==i){
                for(k=0; k<_mPref; k++){
                    cout << std::hex << _bufptr[j]._buf[k]._addr << "\t" ;
                }
            }
        }
        cout << endl;
    }
}
void CACHE::read(uint32_t currAddr, bool isPrefetch){

    _currAddr=currAddr;
   
    register uint32_t index= (unsigned int) (_currAddr & _indexBits) >> _nBitsBlksize;
    register int i=0, j=0, k=0, l=0;
    register int b=0, c=0, d=0, e=0;
    register uint32_t tag= _currAddr >> ((_nBitsBlksize + _nBitsCacheSet));

//    cout << "read: " << _currAddr << " Prefetch: " << isPrefetch << endl;
//    cout << "index: " << (unsigned int)index << endl;
//    cout << "tag: " << std::hex <<tag << endl;
//    cout << endl << endl;

    
    _nReads++; //  reads. (A)

    if(!isPrefetch)
        _nReadsNotPrefetched++;
    else
        _nReadsPrefetch++;

    for(i=0; i < _assoc; i++){
        if(_blockptr[index][i].isTag(tag))
            break;
    }

    if (i==_assoc){
        // read miss in Cache.
        _nReadMisses++;
        if(!isPrefetch)
            _nReadMissNotPrefetch++;
        else
            _nReadMissPrefetch++;
        // check in first entries of all stream buffers
        for(b=0; b<_nPref; b++){
            if(_bufptr[b].checkFirst(_currAddr))
                break;
        }

        if(b==_nPref){
        // scenario #2. miss in Cache and miss in Buffer        

        // 0. make space. check if any invalid blocks preset
            for(j=0; j<_assoc; j++){
                if(_blockptr[index][j]._validBit.none())
                    break;
            }
            _nReadsMissesNotPrefetchBufMiss++;

            if(j!=_assoc){  
            // 1.   invalid blocks present
                if(_memBelow==NULL){    
                // 2. main memory below
                    _totalMemBlksTransported++;             // read the block from the memory
                    _blockptr[index][j].setTag(tag);
                    _blockptr[index][j].setAddr(_currAddr);

                    for(k=0; k<_assoc; k++){                // update LRU counters
                        if (_blockptr[index][k]._validBit.any())
                            _blockptr[index][k]._LRUcounter++;
                    }
                    _blockptr[index][j]._validBit.set();
                    _blockptr[index][j]._LRUcounter=0;

                    if(_nPref!=0){

                        // PREFETCH m blocks
                        // chose the LRU buffer
                        int maxLRUbuf=-1;
                        for(c=0; c<_nPref; c++){
                            if(_bufptr[c]._bufLRUcounter > maxLRUbuf)
                                maxLRUbuf=_bufptr[c]._bufLRUcounter;
                        }
                        for(c=0; c<_nPref; c++){
                            if(_bufptr[c]._bufLRUcounter==maxLRUbuf)
                                break;
                        }
                        // send in M read requests to main mem. push them into the buffer. turn all blocks valid.
                        for(d=0; d<_mPref; d++){
                            _nPrefetch++;
                            _totalMemBlksTransported++;
                            _bufptr[c]._buf[d]._addr=(((_currAddr >> _nBitsBlksize)+(d+1))<<_nBitsBlksize);
                            _bufptr[c]._buf[d]._validBit.set();
                        }
                        
                        // update LRU of all bufs.
                        for(e=0; e<_nPref;e++){
                            _bufptr[e]._bufLRUcounter++;
                        }
                        _bufptr[c]._bufLRUcounter=0;
                    }
                    // send the block up to CPU
                }
                else{
                // L2 below. Send it a read req.
                    _memBelow->read(_currAddr, false);      
                    _blockptr[index][j].setTag(tag);        // receive the block from L2.
                    _blockptr[index][j].setAddr(_currAddr);

                    for(k=0; k <_assoc; k++){                // update LRU counters
                        if (_blockptr[index][k]._validBit.any())
                            _blockptr[index][k]._LRUcounter++;
                    }
                    _blockptr[index][j]._validBit.set();
                    _blockptr[index][j]._LRUcounter=0;

                    if(_nPref!=0){
                        // PREFETCH m blocks
                        // chose the LRU buffer
                        int maxLRUbuf=-1;
                        for(c=0;c<_nPref;c++){
                            if(_bufptr[c]._bufLRUcounter > maxLRUbuf)
                                maxLRUbuf=_bufptr[c]._bufLRUcounter;
                        }
                        for(c=0; c<_nPref; c++){
                            if(_bufptr[c]._bufLRUcounter==maxLRUbuf)
                                break;
                        }
                        // send in M read requests to L2. push them into the buffer. turn all blocks valid.
                        for(d=0; d<_mPref; d++){
                            _nPrefetch++;
                            _memBelow->read((((_currAddr >> _nBitsBlksize)+(d+1))<< _nBitsBlksize), true);
                            _bufptr[c]._buf[d]._addr=(((_currAddr >> _nBitsBlksize)+(d+1))<<_nBitsBlksize);
                            _bufptr[c]._buf[d]._validBit.set();
                        }
                        
                        // update LRU of all bufs.
                        for(e=0; e<_nPref;e++){
                            _bufptr[e]._bufLRUcounter++;
                        }
                        _bufptr[c]._bufLRUcounter=0;
                    }
                    // send block up to CPU
                }
            }
            else{ 
                // no invalid blocks present.
                // find LRU
                int max=-1;
                for(j=0; j<_assoc; j++){
                    if(_blockptr[index][j]._LRUcounter > max)
                        max=_blockptr[index][j]._LRUcounter;
                }
                for(j=0; j<_assoc; j++){
                    if(_blockptr[index][j]._LRUcounter == max)
                        break;
                }
                // check if the LRU block is dirty
                if(_blockptr[index][j]._dirtyBit.any()){
                    // it is dirty. check if there is L2 below
                    if (_memBelow==NULL){
                        // there is main memory below
                        _nWriteBacksToMem++;
                        _totalMemBlksTransported++; // wrote the dirty block to memory
                        _blockptr[index][j]._dirtyBit.reset();

                        if(_nPref!=0){
                        //invalidate all stream buffer blocks corresponding to this block
                            for(c=0;c<_nPref;c++){
                                for(d=0;d<_mPref;d++){
                                    if(_bufptr[c]._buf[d].isAddr(((_blockptr[index][j]._addr) >> _nBitsBlksize)<<_nBitsBlksize))
                                        _bufptr[c]._buf[d]._validBit.reset();
                                }
                            }
                        }
                        _totalMemBlksTransported++; // sent a read to mem / received a block from mem
                        _blockptr[index][j].setTag(tag); 
                        _blockptr[index][j].setAddr(_currAddr);
                        
                        for(k=0; k <_assoc; k++){   // Update LRU
                                _blockptr[index][k]._LRUcounter++;
                        }
                        _blockptr[index][j]._LRUcounter=0;

                        /* PREFETCH STUFF*/
                        if(_nPref!=0){
                            // chose the LRU buffer
                            int maxLRUbuf=-1;
                            for(c=0;c<_nPref;c++){
                                if(_bufptr[c]._bufLRUcounter > maxLRUbuf)
                                    maxLRUbuf=_bufptr[c]._bufLRUcounter;
                            }
                            for(c=0; c<_nPref; c++){
                                if(_bufptr[c]._bufLRUcounter==maxLRUbuf)
                                    break;
                            }
                            // send in M read requests to main mem. push them into the buffer. turn all blocks valid.
                            for(d=0; d<_mPref; d++){
                                _nPrefetch++;
                                _totalMemBlksTransported++;
                                _bufptr[c]._buf[d]._addr=(((_currAddr >> _nBitsBlksize)+(d+1))<<_nBitsBlksize);
                                _bufptr[c]._buf[d]._validBit.set();
                            }
                            
                            // update LRU of all bufs.
                            for(e=0; e<_nPref; e++){
                                _bufptr[e]._bufLRUcounter++;
                            }
                            _bufptr[c]._bufLRUcounter=0;
                        }
                        /*PREFETCH STUFF ENDS*/
                    }
                    else{
                        // there is L2 below
                        _nWriteBacks++;
                        _memBelow->write(_blockptr[index][j]._addr);
                        _blockptr[index][j]._dirtyBit.reset();

                        if(_nPref!=0){
                        //invalidate all stream buffer blocks corresponding to this block
                            for(c=0;c<_nPref;c++){
                                for(d=0;d<_mPref;d++){
                                    if(_bufptr[c]._buf[d].isAddr(((_blockptr[index][j]._addr) >> _nBitsBlksize)<<_nBitsBlksize))
                                        _bufptr[c]._buf[d]._validBit.reset();
                                }
                            }
                        }
                        _memBelow->read(_currAddr, false);
                        _blockptr[index][j].setTag(tag);
                        _blockptr[index][j].setAddr(_currAddr);
                        for(k=0; k <_assoc; k++){               // Update LRU
                                _blockptr[index][k]._LRUcounter++;
                        }
                        _blockptr[index][j]._LRUcounter=0;
                        
                        /* PREFETCH STUFF*/
                        if(_nPref!=0){
                            // chose the LRU buffer
                            int maxLRUbuf=-1;
                            for(c=0;c<_nPref;c++){
                                if(_bufptr[c]._bufLRUcounter > maxLRUbuf)
                                    maxLRUbuf=_bufptr[c]._bufLRUcounter;
                            }
                            for(c=0; c<_nPref; c++){
                                if(_bufptr[c]._bufLRUcounter==maxLRUbuf)
                                    break;
                            }
                            // send in M read requests to main mem. push them into the buffer. turn all blocks valid.
                            for(d=0; d<_mPref; d++){
                                _nPrefetch++;
                                _memBelow->read((((_currAddr >> _nBitsBlksize)+(d+1))<< _nBitsBlksize), true);
                                _bufptr[c]._buf[d]._addr=(((_currAddr >> _nBitsBlksize)+(d+1))<<_nBitsBlksize);
                                _bufptr[c]._buf[d]._validBit.set();
                            }

                            // update LRU of all bufs.
                            for(e=0; e<_nPref;e++){
                                _bufptr[e]._bufLRUcounter++;
                            }
                            _bufptr[c]._bufLRUcounter=0;
                        }
                        /*PREFETCH STUFF ENDS*/
                    }
                }
                else{
                // it is not dirty
                    if (_memBelow==NULL){
                    // there is main memory below
                        _totalMemBlksTransported++;
                        _blockptr[index][j].setTag(tag);
                        _blockptr[index][j].setAddr(_currAddr);
                        for(k=0; k <_assoc; k++){               // Update LRU
                                _blockptr[index][k]._LRUcounter++;
                        }  
                        _blockptr[index][j]._LRUcounter=0;

                        /* PREFETCH STUFF*/
                        if(_nPref!=0){
                            // chose the LRU buffer
                            int maxLRUbuf=-1;
                            for(c=0;c<_nPref;c++){
                                if(_bufptr[c]._bufLRUcounter > maxLRUbuf)
                                    maxLRUbuf=_bufptr[c]._bufLRUcounter;
                            }
                            for(c=0; c<_nPref; c++){
                                if(_bufptr[c]._bufLRUcounter==maxLRUbuf)
                                    break;
                            }
                            // send in M read requests to main mem. push them into the buffer. turn all blocks valid.
                            for(d=0; d<_mPref; d++){
                                _nPrefetch++;
                                _totalMemBlksTransported++;
                                _bufptr[c]._buf[d]._addr=(((_currAddr >> _nBitsBlksize)+(d+1))<<_nBitsBlksize);
                                _bufptr[c]._buf[d]._validBit.set();
                            }

                            // update LRU of all bufs.
                            for(e=0; e<_nPref;e++){
                                _bufptr[e]._bufLRUcounter++;
                            }
                            _bufptr[c]._bufLRUcounter=0;
                        }
                        /*PREFETCH STUFF ENDS*/
                    }
                    else{
                    // there is L2 below
                        _memBelow->read(_currAddr, false);      // send a read below to L2
                        _blockptr[index][j].setTag(tag);
                        _blockptr[index][j].setAddr(_currAddr);
                        
                        for(k=0; k <_assoc; k++){               // Update LRU
                                _blockptr[index][k]._LRUcounter++;
                        }  
                        _blockptr[index][j]._LRUcounter=0;
                        /* PREFETCH STUFF*/
                        if(_nPref!=0){
                            // chose the LRU buffer
                            int maxLRUbuf=-1;
                            for(c=0;c<_nPref;c++){
                                if(_bufptr[c]._bufLRUcounter > maxLRUbuf)
                                    maxLRUbuf=_bufptr[c]._bufLRUcounter;
                            }
                            for(c=0; c<_nPref; c++){
                                if(_bufptr[c]._bufLRUcounter==maxLRUbuf)
                                    break;
                            }
                            // send in M read requests to main mem. push them into the buffer. turn all blocks valid.
                            for(d=0; d<_mPref; d++){
                                _nPrefetch++;
                                _memBelow->read((((_currAddr >> _nBitsBlksize)+(d+1))<< _nBitsBlksize), true);
                                _bufptr[c]._buf[d]._addr=(((_currAddr >> _nBitsBlksize)+(d+1))<<_nBitsBlksize);
                                _bufptr[c]._buf[d]._validBit.set();
                            }
                            
                            // update LRU of all bufs.
                            for(e=0; e<_nPref;e++){
                                _bufptr[e]._bufLRUcounter++;
                            }
                            _bufptr[c]._bufLRUcounter=0;
                        }
                        /*PREFETCH STUFF ENDS*/
                    }
                }
            }
        }
        else{
        // scenario 3. cache miss. stream buffer hit. 
        // make space for the block. 
        
        if(_nPref!=0){
            _nReadMissPrefHit++; // read misses that hit in stream buffer

            // check for invalid blocks
            for(j=0; j<_assoc; j++){
                if(_blockptr[index][j]._validBit.none())
                    break;
            }
            if(j==_assoc){
            // no invalid blocks present.
            // find LRU
                int max=-1;
                for(k=0; k<_assoc; k++){
                    if(_blockptr[index][k]._LRUcounter > max)
                        max=_blockptr[index][k]._LRUcounter;
                }
                for(k=0; k<_assoc; k++){
                    if(_blockptr[index][k]._LRUcounter == max)
                        break;
                }
                if(_blockptr[index][k]._dirtyBit.any()){
                // dirty bit is set
                    if(_memBelow==NULL){
                    // main mem below.
                        _totalMemBlksTransported++; // write back to Mem.
                        _blockptr[index][k]._dirtyBit.reset(); // clear the entry here.
                        
                        if(_nPref!=0){
                        //invalidate all stream buffer blocks corresponding to this block
                            for(c=0;c<_nPref;c++){
                                for(d=0;d<_mPref;d++){
                                    if(_bufptr[c]._buf[d].isAddr(((_blockptr[index][k]._addr) >> _nBitsBlksize)<<_nBitsBlksize))
                                        _bufptr[c]._buf[d]._validBit.reset();
                                }
                            }
                        }

                        _blockptr[index][k].setTag(tag);
                        _blockptr[index][k].setAddr(_currAddr);
                        for(l=0; l <_assoc; l++){   // Update LRU
                                _blockptr[index][l]._LRUcounter++;
                        }
                        _blockptr[index][k]._LRUcounter=0;
                    }
                    else{
                    // L2 below.
                        // write back the dirty block
                        _memBelow->write(_blockptr[index][k]._addr);
                        _nWriteBacks++;
                        _blockptr[index][k]._dirtyBit.reset();
                        
                        if(_nPref!=0){
                        //invalidate all stream buffer blocks corresponding to writeback block
                            for(c=0;c<_nPref;c++){
                                for(d=0;d<_mPref;d++){
                                    if(_bufptr[c]._buf[d].isAddr(((_blockptr[index][j]._addr) >> _nBitsBlksize)<<_nBitsBlksize))
                                        _bufptr[c]._buf[d]._validBit.reset();
                                }
                            }
                        }
                        _blockptr[index][k].setTag(tag);
                        _blockptr[index][k].setAddr(_currAddr); 
                        for(l=0; l <_assoc; l++){   // Update LRU
                                _blockptr[index][l]._LRUcounter++;
                        }
                        _blockptr[index][k]._LRUcounter=0;
                    }
                }
                else{
                // block is not dirty
                    if(_memBelow==NULL){
                    // main mem below.
                        _blockptr[index][k].setTag(tag);
                        _blockptr[index][k].setAddr(_currAddr);
                        for(l=0; l <_assoc; l++){   // Update LRU
                                _blockptr[index][l]._LRUcounter++;
                        }
                        _blockptr[index][k]._LRUcounter=0;

                    }
                    else{
                    // L2 below.
                        _blockptr[index][k].setTag(tag);
                        _blockptr[index][k].setAddr(_currAddr); 
                        for(l=0; l <_assoc; l++){   // Update LRU
                                _blockptr[index][l]._LRUcounter++;
                        }
                        _blockptr[index][k]._LRUcounter=0;
                    }
                } 
            }
            else{
            // found invalid block
                _blockptr[index][j].setTag(tag);
                _blockptr[index][j].setAddr(_currAddr);
                _blockptr[index][j]._validBit.set();
                for(k=0; k<_assoc; k++){                // update LRU counters
                    if (_blockptr[index][k]._validBit.any())
                        _blockptr[index][k]._LRUcounter++;
                }
                _blockptr[index][j]._LRUcounter=0;
            }
            // space is made and block is copied. update stream buffers
            _bufptr[b].shiftUp();
            for(c=0;c<_nPref;c++){
                if(_bufptr[c]._bufLRUcounter < _bufptr[b]._bufLRUcounter)
                    _bufptr[c]._bufLRUcounter++;
            }
            _bufptr[b]._bufLRUcounter=0;
            // read x+m+1 into the stream buffer into last block of stream buffer. validate that block.
            if(_memBelow==NULL){
                _totalMemBlksTransported++;
                _nPrefetch++;
                _bufptr[b]._buf[_mPref-1]._addr= (((_currAddr >> _nBitsBlksize)+(_mPref)) << _nBitsBlksize);
                _bufptr[b]._buf[_mPref-1]._validBit.set();
            }
            else{
                _nPrefetch++;
                _memBelow->read(((( _currAddr >> _nBitsBlksize)+_mPref) << _nBitsBlksize), true);
                _bufptr[b]._buf[_mPref-1]._addr= (((_currAddr >> _nBitsBlksize)+(_mPref)) << _nBitsBlksize);
                _bufptr[b]._buf[_mPref-1]._validBit.set();
            }
        }
        }// end scenario 3.
    }// end read miss
    else{
        // hit. do nothing. increment counters
        for(k=0; k <_assoc; k++){               // Update LRU
            if (_blockptr[index][k]._LRUcounter < _blockptr[index][i]._LRUcounter)
                _blockptr[index][k]._LRUcounter++;
        }  
        _blockptr[index][i]._LRUcounter=0;
    }
}

void CACHE::write(uint32_t currAddr){

    _currAddr=currAddr;
    
    register uint32_t index= (uint32_t) (_currAddr & _indexBits) >>  _nBitsBlksize;
    register int i=0, j=0, k=0, l=0;
    register int b=0, c=0, d=0, e=0;
    register uint32_t tag= _currAddr >> ((_nBitsBlksize + _nBitsCacheSet));
//    cout << "write: " << _currAddr << endl;
//    cout << "tag " << std::hex << tag << endl;
//    cout << "index " << index << endl;
//    cout << endl << endl;

    _nWrites++; // (C) 

    for(i=0; i<_assoc; i++){
        if(_blockptr[index][i].isTag(tag))
            break;
    }

    if (i==_assoc){      
    // write miss
        _nWriteMisses++;
        
        //check the first entry in all stream buffers.
        for(b=0; b<_nPref; b++){
            if(_bufptr[b].checkFirst(_currAddr))
                break;
        }

        if(b==_nPref){
        // scenaior 2. cache miss, buffer miss. in addition to fetching normal block. prefetch M blocks.
            
            _nWriteMissBufMiss++;
            
            // check if there are any invalid blocks present
            for(j=0; j<_assoc; j++){
                if(_blockptr[index][j]._validBit.none())
                    break;
            }

            if(j!=_assoc){
            // found an invalid block
                if(_memBelow==NULL){
                // if main memory below
                    _totalMemBlksTransported++;             // send a read req to mem
                    _blockptr[index][j].setTag(tag);        // receive the block in L2
                    _blockptr[index][j].setAddr(_currAddr);

                    for(k=0; k<_assoc; k++){                // update LRU counters
                        if (_blockptr[index][k]._validBit.any())
                            _blockptr[index][k]._LRUcounter++;
                    }
                    _blockptr[index][j]._validBit.set();
                    _blockptr[index][j]._LRUcounter=0;                
                    _blockptr[index][j]._dirtyBit.set();    // write to the block
                    /* PREFETCH STUFF*/
                    if(_nPref!=0){
                        // chose the LRU buffer
                        int maxLRUbuf=-1;
                        for(c=0;c<_nPref;c++){
                            if(_bufptr[c]._bufLRUcounter > maxLRUbuf)
                                maxLRUbuf=_bufptr[c]._bufLRUcounter;
                        }
                        for(c=0; c<_nPref; c++){
                            if(_bufptr[c]._bufLRUcounter==maxLRUbuf)
                                break;
                        }
                        // send in M read requests to main mem. push them into the buffer. turn all blocks valid.
                        for(d=0; d<_mPref; d++){
                            _totalMemBlksTransported++;
                            _nPrefetch++;
                            _bufptr[c]._buf[d]._addr=(((_currAddr >> _nBitsBlksize)+(d+1))<<_nBitsBlksize);
                            _bufptr[c]._buf[d]._validBit.set();
                        }

                        // update LRU of all bufs.
                        for(e=0; e<_nPref;e++){
                            _bufptr[e]._bufLRUcounter++;
                        }
                        _bufptr[c]._bufLRUcounter=0;
                    }
                    /*PREFETCH STUFF ENDS*/
                }
                else{
                // L2 below
                    _memBelow->read(_currAddr, false);      
                    _blockptr[index][j].setTag(tag);        // receive the block from L2.
                    _blockptr[index][j].setAddr(_currAddr);

                    for(k=0; k<_assoc; k++){                // update LRU counters
                        if (_blockptr[index][k]._validBit.any())
                            _blockptr[index][k]._LRUcounter++;
                    }
                    _blockptr[index][j]._validBit.set();
                    _blockptr[index][j]._LRUcounter=0;
                    _blockptr[index][j]._dirtyBit.set();    // write to the block
                    /* PREFETCH STUFF*/
                    if(_nPref!=0){
                        // chose the LRU buffer
                        int maxLRUbuf=-1;
                  
                        for(c=0;c<_nPref;c++){
                            if(_bufptr[c]._bufLRUcounter > maxLRUbuf)
                                maxLRUbuf=_bufptr[c]._bufLRUcounter;
                        }
                        for(c=0; c<_nPref; c++){
                            if(_bufptr[c]._bufLRUcounter==maxLRUbuf)
                                break;
                        }
                        // send in M read requests to main mem. push them into the buffer. turn all blocks valid.
                        for(d=0; d<_mPref; d++){
                            _nPrefetch++;
                            _memBelow->read((((_currAddr >> _nBitsBlksize)+(d+1))<< _nBitsBlksize), true);
                            _bufptr[c]._buf[d]._addr=(((_currAddr >> _nBitsBlksize)+(d+1))<<_nBitsBlksize);
                            _bufptr[c]._buf[d]._validBit.set();
                        }
                        
                        // update LRU of all bufs.
                        for(e=0; e<_nPref;e++){
                            _bufptr[e]._bufLRUcounter++;
                        }
                        _bufptr[c]._bufLRUcounter=0;
                    }
                    /*PREFETCH STUFF ENDS*/
                }
            }
            else{
            // no invalid blocks, find the LRU block
                int max=0;
                for(j=0; j<_assoc; j++){
                    if(_blockptr[index][j]._LRUcounter > max)
                        max=_blockptr[index][j]._LRUcounter;
                }
    //            cout << "max: " << max << endl;
                for(j=0; j<_assoc; j++){
                    if(_blockptr[index][j]._LRUcounter == max)
                        break;
                }
                // check if the LRU block is dirty
                if(_blockptr[index][j]._dirtyBit.any()){
                // it is dirty. check if there is L2 below
                    if (_memBelow==NULL){
                    // there is main memory below
                        _nWriteBacksToMem++;
                        _totalMemBlksTransported++; // wrote the dirty block to memory
                        
                        if(_nPref!=0){
                        //invalidate all stream buffer blocks corresponding to this block
                            for(c=0;c<_nPref;c++){
                                for(d=0;d<_mPref;d++){
                                    if(_bufptr[c]._buf[d].isAddr(((_blockptr[index][j]._addr) >> _nBitsBlksize)<<_nBitsBlksize))
                                        _bufptr[c]._buf[d]._validBit.reset();
                                }
                            }
                        }
                        _totalMemBlksTransported++; // sent a read to mem / received a block from mem
                        _blockptr[index][j].setTag(tag); 
                        _blockptr[index][j].setAddr(_currAddr);
                        
                        for(k=0; k <_assoc; k++){   // Update LRU
                                _blockptr[index][k]._LRUcounter++;
                        }
                        _blockptr[index][j]._LRUcounter=0;
                        _blockptr[index][j]._dirtyBit.set();

                        /* PREFETCH STUFF*/
                        if(_nPref!=0){
                            // chose the LRU buffer
                            int maxLRUbuf=-1;
                            for(c=0;c<_nPref;c++){
                                if(_bufptr[c]._bufLRUcounter > maxLRUbuf)
                                    maxLRUbuf=_bufptr[c]._bufLRUcounter;
                            }
                            for(c=0; c<_nPref; c++){
                                if(_bufptr[c]._bufLRUcounter==maxLRUbuf)
                                    break;
                            }
                            // send in M read requests to main mem. push them into the buffer. turn all blocks valid.
                            for(d=0; d<_mPref; d++){
                                _nPrefetch++;
                                _totalMemBlksTransported++;
                                _bufptr[c]._buf[d]._addr=(((_currAddr >> _nBitsBlksize)+(d+1))<<_nBitsBlksize);
                                _bufptr[c]._buf[d]._validBit.set();
                            }

                            // update LRU of all bufs.
                            for(e=0; e<_nPref;e++){
                                _bufptr[e]._bufLRUcounter++;
                            }
                            _bufptr[c]._bufLRUcounter=0;
                        }
                        /*PREFETCH STUFF ENDS*/
                    }
                    else{
                          // there is L2 below
                        _nWriteBacks++;                     // write the dirty block to L2
                        _memBelow->write(_blockptr[index][j]._addr); 
                        _blockptr[index][j]._dirtyBit.reset();
 
                        if(_nPref!=0){
                            //invalidate all stream buffer blocks corresponding to this block
                            for(c=0;c<_nPref;c++){
                                for(d=0;d<_mPref;d++){
                                    if(_bufptr[c]._buf[d].isAddr(((_blockptr[index][j]._addr) >> _nBitsBlksize)<<_nBitsBlksize))
                                        _bufptr[c]._buf[d]._validBit.reset();
                                }
                            }
                        }
                        _memBelow->read(_currAddr, false);  // read this block from L2
                        _blockptr[index][j].setTag(tag);    // receive it in L1 
                        _blockptr[index][j].setAddr(_currAddr);

                        for(k=0; k <_assoc; k++){               // Update LRU
                                _blockptr[index][k]._LRUcounter++;
                        }
                        _blockptr[index][j]._LRUcounter=0;
                        _blockptr[index][j]._dirtyBit.set();    // write to the block
                        /* PREFETCH STUFF*/
                        if(_nPref!=0){
                            // chose the LRU buffer
                            int maxLRUbuf=-1;
                            for(c=0;c<_nPref;c++){
                                if(_bufptr[c]._bufLRUcounter > maxLRUbuf)
                                    maxLRUbuf=_bufptr[c]._bufLRUcounter;
                            }
                            for(c=0; c<_nPref; c++){
                                if(_bufptr[c]._bufLRUcounter==maxLRUbuf)
                                    break;
                            }
                            // send in M read requests to main mem. push them into the buffer. turn all blocks valid.
                            for(d=0; d<_mPref; d++){
                                _nPrefetch++;
                                _memBelow->read((((_currAddr >> _nBitsBlksize)+(d+1))<< _nBitsBlksize), true);
                                _bufptr[c]._buf[d]._addr=(((_currAddr >> _nBitsBlksize)+(d+1))<<_nBitsBlksize);
                                _bufptr[c]._buf[d]._validBit.set();
                            }
                            
                            // update LRU of all bufs.
                            for(e=0; e<_nPref;e++){
                                _bufptr[e]._bufLRUcounter++;
                            }
                            _bufptr[c]._bufLRUcounter=0;
                        }
                        /*PREFETCH STUFF ENDS*/
                    }
                }
                else{
                // it is not dirty
                    if (_memBelow==NULL){
                    // there is main memory below
                        _totalMemBlksTransported++;             // send a read request to mem
                        _blockptr[index][j].setTag(tag);        // receive it in L2
                        _blockptr[index][j].setAddr(_currAddr);
                        for(k=0; k <_assoc; k++){               // Update LRU
                                _blockptr[index][k]._LRUcounter++;
                        }  
                        _blockptr[index][j]._LRUcounter=0;
                        _blockptr[index][j]._dirtyBit.set();    // write to the block. make it dirty
                        /* PREFETCH STUFF*/
                        if(_nPref!=0){
                            // chose the LRU buffer
                            int maxLRUbuf=-1;
                            for(c=0;c<_nPref;c++){
                                if(_bufptr[c]._bufLRUcounter > maxLRUbuf)
                                    maxLRUbuf=_bufptr[c]._bufLRUcounter;
                            }
                            for(c=0; c<_nPref; c++){
                                if(_bufptr[c]._bufLRUcounter==maxLRUbuf)
                                    break;
                            }
                            // send in M read requests to main mem. push them into the buffer. turn all blocks valid.
                            for(d=0; d<_mPref; d++){
                                _nPrefetch++;
                                _totalMemBlksTransported++;
                                _bufptr[c]._buf[d]._addr=(((_currAddr >> _nBitsBlksize)+(d+1))<<_nBitsBlksize);
                                _bufptr[c]._buf[d]._validBit.set();
                            }

                            // update LRU of all bufs.
                            for(e=0; e<_nPref;e++){
                                _bufptr[e]._bufLRUcounter++;
                            }
                            _bufptr[c]._bufLRUcounter=0;
                        }
                        /*PREFETCH STUFF ENDS*/
                    }
                    else{
                      // there is L2 below
                        _memBelow->read(_currAddr, false);      // send a read req to L2
                        _blockptr[index][j].setTag(tag);        // receive it in L1
                        _blockptr[index][j].setAddr(_currAddr);
                        for(k=0; k <_assoc; k++){               // Update LRU
                                _blockptr[index][k]._LRUcounter++;
                        }  
                        _blockptr[index][j]._LRUcounter=0;
                        _blockptr[index][j]._dirtyBit.set();    // write to block. make it dirty
                        /* PREFETCH STUFF*/
                        if(_nPref!=0){
                            // chose the LRU buffer
                            int maxLRUbuf=-1;
                            for(c=0;c<_nPref;c++){
                                if(_bufptr[c]._bufLRUcounter > maxLRUbuf)
                                    maxLRUbuf=_bufptr[c]._bufLRUcounter;
                            }
                            for(c=0; c<_nPref; c++){
                                if(_bufptr[c]._bufLRUcounter==maxLRUbuf)
                                    break;
                            }
                            // send in M read requests to main mem. push them into the buffer. turn all blocks valid.
                            for(d=0; d<_mPref; d++){
                                _nPrefetch++;
                                _memBelow->read((((_currAddr >> _nBitsBlksize)+(d+1))<< _nBitsBlksize), true);
                                _bufptr[c]._buf[d]._addr=(((_currAddr >> _nBitsBlksize)+(d+1))<<_nBitsBlksize);
                                _bufptr[c]._buf[d]._validBit.set();
                            }
                            
                            // update LRU of all bufs.
                            for(e=0; e<_nPref;e++){
                                _bufptr[e]._bufLRUcounter++;
                            }
                            _bufptr[c]._bufLRUcounter=0;
                        }
                        /*PREFETCH STUFF ENDS*/

                    } // end. there is L2 below.
                } // end. it is not dirty
            }// end. invalid blocks check
        }// end (b==_assoc) scenario 2. 
        else{
        // scenario 3. cache miss. buffer hit.
        // make space for the block. bring the block from buffer to cache. write to it. make it dirty. shift the buf up.
            //check for invalid blocks.
            if(_nPref!=0){
            _nWritesMissPrefetchHit++;
            for(j=0; j<_assoc; j++){
                if(_blockptr[index][j]._validBit.none())
                    break;
            }
            if(j==_assoc){
            // no invalid blocks present.
            // find LRU
                int max=-1;
                for(k=0; k<_assoc; k++){
                    if(_blockptr[index][k]._LRUcounter > max)
                        max=_blockptr[index][k]._LRUcounter;
                }
                for(k=0; k<_assoc; k++){
                    if(_blockptr[index][k]._LRUcounter == max)
                        break;
                }
                if(_blockptr[index][k]._dirtyBit.any()){
                // dirty bit is set
                    if(_memBelow==NULL){
                    // main mem below.
                        _totalMemBlksTransported++; // write back to Mem.
                        _blockptr[index][k]._dirtyBit.reset(); // clear the entry here.
                        
                        if(_nPref!=0){
                        //invalidate all stream buffer blocks corresponding to this block
                            for(c=0;c<_nPref;c++){
                                for(d=0;d<_mPref;d++){
                                    if(_bufptr[c]._buf[d].isAddr(((_blockptr[index][j]._addr) >> _nBitsBlksize)<<_nBitsBlksize))
                                        _bufptr[c]._buf[d]._validBit.reset();
                                }
                            }
                        }

                        _blockptr[index][k].setTag(tag);
                        _blockptr[index][k].setAddr(_currAddr);
                        for(l=0; l <_assoc; l++){   // Update LRU
                                _blockptr[index][l]._LRUcounter++;
                        }
                        _blockptr[index][k]._LRUcounter=0;
                        _blockptr[index][k]._dirtyBit.set();
                    }
                    else{
                    // L2 below.
                        _memBelow->write(_blockptr[index][k]._addr);
                        _nWriteBacks++;
                        _blockptr[index][k]._dirtyBit.reset();
                        
                        if(_nPref!=0){
                        //invalidate all stream buffer blocks corresponding to writeback block
                            for(c=0;c<_nPref;c++){
                                for(d=0;d<_mPref;d++){
                                    if(_bufptr[c]._buf[d].isAddr(((_blockptr[index][k]._addr) >> _nBitsBlksize)<<_nBitsBlksize))
                                        _bufptr[c]._buf[d]._validBit.reset();
                                }
                            }
                        }
                        _blockptr[index][k].setTag(tag);
                        _blockptr[index][k].setAddr(_currAddr); 
                        for(l=0; l <_assoc; l++){   // Update LRU
                                _blockptr[index][l]._LRUcounter++;
                        }
                        _blockptr[index][k]._LRUcounter=0;
                        _blockptr[index][k]._dirtyBit.set();
                    }
                }
                else{
                // block is not dirty
                    if(_memBelow==NULL){
                    // main mem below.
                        _blockptr[index][k].setTag(tag);
                        _blockptr[index][k].setAddr(_currAddr);
                        for(l=0; l <_assoc; l++){   // Update LRU
                                _blockptr[index][l]._LRUcounter++;
                        }
                        _blockptr[index][k]._LRUcounter=0;
                        _blockptr[index][k]._dirtyBit.set();

                    }
                    else{
                    // L2 below.
                        _blockptr[index][k].setTag(tag);
                        _blockptr[index][k].setAddr(_currAddr); 
                        for(l=0; l <_assoc; l++){   // Update LRU
                                _blockptr[index][l]._LRUcounter++;
                        }
                        _blockptr[index][k]._LRUcounter=0;
                        _blockptr[index][k]._dirtyBit.set();
                    }
                } 
            }
            else{
            // found invalid block
                _blockptr[index][j].setTag(tag);
                _blockptr[index][j].setAddr(_currAddr);
                _blockptr[index][j]._validBit.set();
                for(k=0; k<_assoc; k++){                // update LRU counters
                    if (_blockptr[index][k]._validBit.any())
                        _blockptr[index][k]._LRUcounter++;
                }
                _blockptr[index][j]._LRUcounter=0;
                _blockptr[index][j]._dirtyBit.set();
            }
            // space is made and block is copied. update stream buffers
            _bufptr[b].shiftUp();
            for(c=0;c<_nPref;c++){
                if(_bufptr[c]._bufLRUcounter < _bufptr[b]._bufLRUcounter)
                    _bufptr[c]._bufLRUcounter++;
            }
            _bufptr[b]._bufLRUcounter=0;
            // read x+m+1 into the stream buffer. validate that block.
            if(_memBelow==NULL){
                _totalMemBlksTransported++;
                _nPrefetch++;
                _bufptr[b]._buf[_mPref-1]._addr+=_blksize;
                _bufptr[b]._buf[_mPref-1]._validBit.set();
            }
            else{
                _nPrefetch++;
                _memBelow->read(((( _currAddr >> _nBitsBlksize)+_mPref) << _nBitsBlksize), true);
                _bufptr[b]._buf[_mPref-1]._addr+= _blksize;
                _bufptr[b]._buf[_mPref-1]._validBit.set();
            }

        } // end (b!=_assoc) scenario 3.
        }
    }// end check miss / hit (i==_assoc)
    else{
        // hit. do nothing. increment counters
        for(k=0; k <_assoc; k++){               // Update LRU
            if (_blockptr[index][k]._LRUcounter < _blockptr[index][i]._LRUcounter)
                _blockptr[index][k]._LRUcounter++;
        }  
        _blockptr[index][i]._LRUcounter=0;
        _blockptr[index][i]._dirtyBit.set();
    }
}


          // 0. check if any invalid blocks preset
          // 1. if yes,
          // 2.   if _memBelow==NULL
          // 3.       send a read request to main memory. 
          // 4.       _totalMemBlksTransported++;
          // 5.       enter the tag here to represent the read complete. turn the valid bit
          // 6.       update LRU counters of all; update counters.
          // 7.   else,
          // 8.       send a request to L2.
          // 9.       enter the tag here to represent read complete.
          // 10.      update LRU counters of all; update counters.
          // 11.else,
          // 12.  find the LRU block.
          // 13.  if block==dirty,
          // 14.      if _memBelow==NULL
          // 15.          _nWriteBacksToMem++; _totalMemBlksTransported++;
          // 16.          delete the entry here. dirty bit=0, valid bit=1
          // 17.          send a read for currAddr. (_memBelow->read(_currAddr, false))
          // 18.          add currAddr to cleared space. dirty bit=0, valid bit=1
          // 19.          update LRU to all blocks; update counters.
          // 20.      else,
          // 21.          _nWriteBackstoL2++;
          // 22.          delete entry here. Dirty bit=0, vaid bit=1
          // 23.          send a read for currAddr
          // 24.          add currAddr to cleared space. dirty bit=0, valid bit=1
          // 25.          update LRU to all blocks; update counters.
          // 26.  else,
          // 27.      if _memBelow==NULL
          // 28.          _totalMemBlksTransported++;
          // 29.          delete entry here.  send a read req to mem
          // 30.          receive block here. set the tag.  
          // 31.          update LRUs
          // 32.      else
          // 33.          delete entry here. send read req to L2
          // 34.          receive block here
          // 35.          update LRU. 
