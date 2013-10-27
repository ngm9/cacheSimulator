#include <iostream>
#include <string>
#include <cache.h>
#include <cstdlib>
#include <iomanip>

using namespace std;

int main(int argc, char* argv[]){

    int blksize=atoi(argv[1]);
    int size_l1=atoi(argv[2]);
    int assoc_l1=atoi(argv[3]);
    int pref_l1_N=atoi(argv[4]);
    int pref_l1_M=atoi(argv[5]);
    int size_l2=atoi(argv[6]);
    int assoc_l2=atoi(argv[7]);
    int pref_l2_N=atoi(argv[8]);
    int pref_l2_M=atoi(argv[9]);
    
    cout.flush();

    if(atoi(argv[6])!=0){
        cout << std:: dec << "===== Simulator Configuration =====" << endl;
        cout << "BLOCKSIZE:     " << blksize << endl;
        cout << "L1_SIZE:       " << size_l1 << endl;
        cout << "L1_ASSOC:      " << assoc_l1 << endl;
        cout << "L1_PREF_N:     " << pref_l1_N << endl;
        cout << "L1_PREF_M:     " << pref_l1_M << endl;
        cout << "L2_SIZE:       " << size_l2 << endl;
        cout << "L2_ASSOC:      " << assoc_l2 << endl;
        cout << "L2_PREF_N:     " << pref_l2_N << endl;
        cout << "L2_PREF_M:     " << pref_l2_M << endl;
        cout << "trace_file:     " << argv[10] << endl;
        CACHE L2(size_l2, assoc_l2, (uint8_t)blksize, pref_l2_N, pref_l2_M, NULL, argv[10], false);
        CACHE L1(size_l1, assoc_l1, (uint8_t)blksize, pref_l1_N, pref_l1_M, &L2, argv[10], true);
 
        cout << "===== Simulation results (raw) =====" << endl;
        cout << "a. number of l1 reads:              " << std:: dec << L1._nReads << endl; 
        cout << "b. number of l1 read misses:        " << L1._nReadMisses - L1._nReadMissPrefHit << endl;
        cout << "c. number of l1 writes:             " << L1._nWrites << endl;
        cout << "d. number of l1 write misses:       " << L1._nWriteMissBufMiss << endl;
        cout << "e. L1 miss rate:                    " <<  std::setprecision (6) << (float)((L1._nReadMisses- L1._nReadMissPrefHit) + L1._nWriteMissBufMiss) / (L1._nReads + L1._nWrites) << endl;
        cout << "f. number of L1 writebacks:         " << L1._nWriteBacks << endl;
        cout << "g. number of L1 prefetches:         " << L1._nPrefetch << endl;
        cout << "h. number of L2 reads that did not originate from L1 prefetches: " << L2._nReadsNotPrefetched << endl;
        cout << "i. number of L2 read misses that did not originate from L1 prefetches: " << (L2._nReadMissNotPrefetch - L2._nReadMissPrefHit) << endl;
        cout << "j. number of L2 reads that originated from L1 prefetches: " << L2._nReadsPrefetch << endl;
        cout << "k. number of L2 read misses that originated from L1 prefetches: " << L2._nReadMissPrefetch - L2._nReadMissPrefHit << endl; 
        cout << "l. number of L2 writes:              " << L1._nWriteBacks << endl;
        cout << "m. number of L2 write misses:        " << L2._nWriteMissBufMiss << endl;
        cout << "n. L2 miss rate:                     " <<  (float) (L2._nReadMissNotPrefetch - L2._nReadMissPrefHit) / L2._nReadsNotPrefetched << endl;
        cout << "o. number of L2 writebacks:          " << L2._nWriteBacksToMem << endl;
        cout << "p. number of L2 prefetches:         " << L2._nPrefetch << endl;
        cout << "q. total memory traffic:            " << L2._totalMemBlksTransported << endl;
       
    }
    else{
        cout << std:: dec << "===== Simulator Configuration =====" << endl;
        cout << "BLOCKSIZE:     " << blksize << endl;
        cout << "L1_SIZE:       " << size_l1 << endl;
        cout << "L1_ASSOC:      " << assoc_l1 << endl;
        cout << "L1_PREF_N:     " << pref_l1_N << endl;
        cout << "L1_PREF_M:     " << pref_l1_M << endl;
        cout << "L2_SIZE:       " << size_l2 << endl;
        cout << "L2_ASSOC:      " << assoc_l2 << endl;
        cout << "L2_PREF_N:     " << pref_l2_N << endl;
        cout << "L2_PREF_M:     " << pref_l2_M << endl;
        cout << "trace_file:     " << argv[10] << endl;

        CACHE L1(size_l1, assoc_l1, (uint8_t)blksize, pref_l1_N, pref_l1_M, NULL, argv[10], true);
    
        cout << "===== Simulation results (raw) =====" << endl;
        cout << "a. number of l1 reads:              " << std::dec << L1._nReads << endl; 
        cout << "b. number of l1 read misses:        " << L1._nReadMisses << endl;
        cout << "c. number of l1 writes:             " << L1._nWrites << endl;
        cout << "d. number of l1 write misses:       " << L1._nWriteMissBufMiss << endl;
        cout << "e. L1 miss rate:                    " << std::setprecision (7) <<(float)(L1._nReadMisses + L1._nWriteMissBufMiss) / (L1._nReads + L1._nWrites) << endl;
        cout << "f. number of L1 writebacks:         " << L1._nWriteBacksToMem << endl;
        cout << "g. number of L1 prefetches:         " << L1._nPrefetch << endl;
        cout << "h. number of L2 reads that did not originate from L1 prefetches: " << "0" << endl;
        cout << "i. number of L2 read misses that did not originate from L1 prefetches: " << "0" << endl;
        cout << "j. number of L2 reads that originated from L1 prefetches: " << "0" << endl;
        cout << "k. number of L2 read misses that originated from L1 prefetches: " << "0" << endl; 
        cout << "l. number of L2 writes:              " << "0" << endl;
        cout << "m. number of L2 write misses:        " << "0" << endl;
        cout << "n. L2 miss rate:                    " << "0" << endl;
        cout << "o. number of L2 writebacks:          " << "0" << endl;
        cout << "p. number of L2 prefetches:         " << "0" << endl;
        cout << "q. total memory traffic:            " << L1._totalMemBlksTransported << endl;
    }
    
    
}
