/*******************************************************
                          cache.cc
                  Ahmad Samih & Yan Solihin
                           2009
                {aasamih,solihin}@ece.ncsu.edu
 ********************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <vector>
#include "cache.h"
using namespace std;

Cache::Cache(int s, int a, int b) {
    ulong i, j;

    reads = readMisses = writes = 0;
    writeMisses = writeBacks = currentCycle = 0;
    memoryTransactions = cacheToCacheTransfers = 0;
    
    size = (ulong) (s);
    assoc = (ulong) (a);
    lineSize = (ulong) (b);
    sets = (ulong) ((s / b) / a);
    numLines = (ulong) (s / b);
    log2Sets = (ulong) (log2(sets));
    log2Blk = (ulong) (log2(b));

    //*******************//
    //initialize your counters here//
    //*******************//

    tagMask = 0;
    for (i = 0; i < log2Sets; i++) {
        tagMask <<= 1;
        tagMask |= 1;
    }

    /**create a two dimentional cache, sized as cache[sets][assoc]**/
    cache = new cacheLine*[sets];
    for (i = 0; i < sets; i++) {
        cache[i] = new cacheLine[assoc];
        for (j = 0; j < assoc; j++) {
            cache[i][j].invalidate();
        }
    }

}

/**you might add other parameters to Access()
since this function is an entry point 
to the memory hierarchy (i.e. caches)**/
void Cache::Access(ulong addr, uchar op, vector<Cache*> &cachesArray) {
    currentCycle++; /*per cache global counter to maintain LRU order 
			among cache ways, updated on every cache access*/

    if (op == 'w') {
        writes++;
    } else {
        reads++;
    }

    cacheLine *line = findLine(addr);
    bool snoop;
    if (line == NULL) { /*miss*/
        cacheLine *newline = fillLine(addr);
        
        if (op == 'w') {
            snoop = busRd(addr, cachesArray);
            if (snoop) {
                busUpd(addr, cachesArray);
                newline->setFlags(SHARED_MODIFIED);
                cacheToCacheTransfers++;
            } else {
                newline->setFlags(MODIFIED);
                memoryTransactions++;
            }
            writeMisses++;            
        } else {
            snoop = busRd(addr, cachesArray);
            if (snoop) {
                newline->setFlags(SHARED_CLEAN);
                cacheToCacheTransfers++;
            } else {  
                newline->setFlags(EXCLUSIVE);
                memoryTransactions++;
            } 
            readMisses++;
        }
    } else {
        /**since it's a hit, update LRU and update dirty flag**/
        updateLRU(line);
        
        if (op == 'w') {
            if (line->getFlags() == EXCLUSIVE) {
                line->setFlags(MODIFIED);
            } else if (line->getFlags() != MODIFIED) {
                snoop = busUpd(addr, cachesArray);
                if (snoop) {
                    line->setFlags(SHARED_MODIFIED);
                } else {
                    line->setFlags(MODIFIED);
                }
            }
        }
    }
}

bool Cache::busUpd(ulong addr, std::vector<Cache*> &cachesArray) {
    bool snoop;
    
    for (unsigned int i = 0; i < cachesArray.size(); i++) {
        snoop = cachesArray[i]->busUpd(addr) || snoop;
    }
    
    return snoop;
}

bool Cache::busRd(ulong addr, std::vector<Cache*> &cachesArray) {
    bool snoop;
    
    for (unsigned int i = 0; i < cachesArray.size(); i++) {
        snoop = cachesArray[i]->busRd(addr) || snoop;
    }
    
    return snoop;
}

bool Cache::busUpd(ulong addr) {
    cacheLine *line = findLine(addr);
    if (line == NULL) {
        return false;
    } else {
        if (line->getFlags() == SHARED_CLEAN || line->getFlags() == SHARED_MODIFIED) {
            line->setFlags(SHARED_CLEAN);
            return true;
        } else { // EXLUSIVE and MODIFIED don't care.
            return false;
        }
    }    
}

bool Cache::busRd(ulong addr) {
    cacheLine *line = findLine(addr);
    
    if (line == NULL) {
        return false;
    } else {
        if (line->getFlags() == EXCLUSIVE) {
            line->setFlags(SHARED_CLEAN);
            return true;
        } else if (line->getFlags() == MODIFIED || line->getFlags() == SHARED_MODIFIED) {
            line->setFlags(SHARED_MODIFIED);
            return true;
        } else if (line->getFlags() == SHARED_CLEAN) {
            return true;
        } else {
            return false;
        }        
    }
}

/*look up line*/
cacheLine *Cache::findLine(ulong addr) {
    ulong i, j, tag, pos;

    pos = assoc;
    tag = calcTag(addr);
    i = calcIndex(addr);

    for (j = 0; j < assoc; j++) {
        if (cache[i][j].isValid()) {
            if (cache[i][j].getTag() == tag) {
                pos = j;
                break;
            }
        }
    }
    
    if (pos == assoc) {
        return NULL;
    } else {
        return &(cache[i][pos]);
    }
}

/*upgrade LRU line to be MRU line*/
void Cache::updateLRU(cacheLine *line) {
    line->setSeq(currentCycle);
}

/*return an invalid line as LRU, if any, otherwise return LRU line*/
cacheLine * Cache::getLRU(ulong addr) {
    ulong i, j, victim, min;

    victim = assoc;
    min = currentCycle;
    i = calcIndex(addr);

    for (j = 0; j < assoc; j++) {
        if (cache[i][j].isValid() == 0) return &(cache[i][j]);
    }
    
    for (j = 0; j < assoc; j++) {
        if (cache[i][j].getSeq() <= min) {
            victim = j;
            min = cache[i][j].getSeq();
        }
    }
    
    assert(victim != assoc);

    return &(cache[i][victim]);
}

/*find a victim, move it to MRU position*/
cacheLine *Cache::findLineToReplace(ulong addr) {
    cacheLine * victim = getLRU(addr);
    updateLRU(victim);

    return (victim);
}

/*allocate a new line*/
cacheLine *Cache::fillLine(ulong addr) {
    ulong tag;

    cacheLine *victim = findLineToReplace(addr);
    assert(victim != 0);
    if (victim->getFlags() == MODIFIED || victim->getFlags() == SHARED_MODIFIED) {
        writeBack(addr);
    }
    
    tag = calcTag(addr);
    victim->setTag(tag);
    victim->setFlags(VALID);
    /**note that this cache line has been already 
       upgraded to MRU in the previous function (findLineToReplace)**/
    
    return victim;
}

void Cache::printStats() {
    
    //printf("===== Simulation results      =====\n");
    /****print out the rest of statistics here.****/
    /****follow the ouput file format**************/
    
    printf("01. number of reads:                              %li\n", reads);
    printf("02. number of read misses:                        %li\n", readMisses);
    printf("03. number of writes:                             %li\n", writes);
    printf("04. number of write misses:                       %li\n", writeMisses);
    printf("05. total miss rate:                              %f\n", (writeMisses + readMisses + 0.0) / (reads + writes));
    printf("06. number of writebacks:                         %li\n", writeBacks);
    printf("07. number of memory transactions:                %li\n", memoryTransactions);
    printf("08. number of cache to cache transfers:           %li\n", cacheToCacheTransfers);
}      
       