#include <unistd.h>
#include <getopt.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include "cachelab.h"

//command argument
int cache_set_bit;
int cache_lines;
int cache_block_bit;

//counter variable to print
int num_cache_hit;
int num_cache_miss;
int num_cache_evict;

//used to get set and tag bit
unsigned long cache_set_mask;
unsigned long cache_tag_mask;

//trace file name
char *trace_file_name;

unsigned long **cache; //store tag bit
unsigned long **timestamp; //store timestamp for LRU policy
unsigned char **valid; //store valid bit

//get info from command line and store in cache variable(command argument)
void parse_from_command_line(int argc, char *argv[]);

//simulate cache for given trace file
void simulate_trace(FILE *fp);

//initialize cache struct
void init_cache();

//clean up cache struct
void clear_cache();

//get set index in cache using cache_set_mask
unsigned long get_set_index(unsigned long x);

//get tag bit in cache using cache_tag_mask
unsigned long get_tag(unsigned long x);

//figure out what happened in cache(hit, miss, evict)
//when address x accessed in time t
void process_inst(unsigned long x, unsigned long t);

int main(int argc, char *argv[]){
    //get info from command
    parse_from_command_line(argc,argv);
    
    //read trace file
    FILE *fp = fopen(trace_file_name, "r");

    //make cache
    init_cache();
    
    //simulate cache
    simulate_trace(fp);
    
    //print result
    printSummary(num_cache_hit, num_cache_miss, num_cache_evict);

    //clean cache
    clear_cache();

    //close file
    fclose(fp);
    
    return 0;
}

void parse_from_command_line(int argc, char *argv[]){
    int c;
    while((c = getopt(argc, argv, "s:E:b:t:")) != -1){
        switch(c){
            case 's':
                //set bit
                cache_set_bit = atoi(optarg);
                break;
            case 'E':
                //lines
                cache_lines = atoi(optarg);
                break;
            case 'b':
                //block offset
                cache_block_bit = atoi(optarg);
                break;
            case 't':
                //file name
                trace_file_name = optarg;
                break;
        }
    }
}

void simulate_trace(FILE *fp){
    char inst; //instruction type
    unsigned long addr; //address
    unsigned long t = 1; //time var
    int siz; //size to get from memory (not used)
    while(fscanf(fp," %c %lx,%d",&inst,&addr,&siz)!=EOF){
        switch(inst){
            case 'M':
                //modify case
                process_inst(addr,t++);
                process_inst(addr,t++);
                break;
            case 'L':
                //load case
                process_inst(addr,t++);
                break;
            case 'S':
                //store case
                process_inst(addr,t++);
                break;
        }
    }
}

void init_cache(){
    //allocate three array (cache, timestamp, valid)
    //size is 2 ^ (cache_set_bit) *  cache_lines ([2^s][e])

    cache = calloc(1LL<<cache_set_bit,sizeof(*cache));
    timestamp = calloc(1LL<<cache_set_bit,sizeof(*timestamp));
    valid = calloc(1LL<<cache_set_bit,sizeof(*valid));

    for(size_t i=0;i<(1LL<<cache_set_bit);i++){
        cache[i] = calloc(cache_lines,sizeof(unsigned long));
        timestamp[i] = calloc(cache_lines,sizeof(unsigned long));
        valid[i] = calloc(cache_lines,sizeof(unsigned char));
    }

    //make mask variable
    cache_set_mask = ((1LL<<cache_set_bit)-1)<<cache_block_bit;
    cache_tag_mask = ((1LL<<(64-cache_block_bit-cache_set_bit))-1)<<(cache_set_bit+cache_block_bit);
}

void clear_cache(){
    //free all of them
    for(size_t i=0;i<(1LL<<cache_set_bit);i++){
        free(cache[i]);
        free(timestamp[i]);
        free(valid[i]);
    }

    free(cache);
    free(timestamp);
    free(valid);
}

unsigned long get_set_index(unsigned long x){
    //use mask var
    return (x&cache_set_mask)>>cache_block_bit;
}

unsigned long get_tag(unsigned long x){
    //use mask var
    return (x&cache_tag_mask)>>(cache_block_bit+cache_set_bit);
}

void process_inst(unsigned long x, unsigned long t){
    //get index and tag bit
    unsigned long idx = get_set_index(x);
    unsigned long tag = get_tag(x);

    //mininal timestamp and cache index
    //to find empty cache or victim cache
    unsigned long min_t = t;
    size_t evict_cache_j = cache_lines;
    
    for(size_t j=0;j<cache_lines;j++){
        if(valid[idx][j] && tag == cache[idx][j]){
            //hit case
            //update access time
            timestamp[idx][j] = t;
            num_cache_hit++;
            return;
        }
        else if(timestamp[idx][j]<min_t){
            //find new candidate
            min_t = timestamp[idx][j];
            evict_cache_j = j;
        }
    }
    //miss case
    num_cache_miss ++;

    //evict case
    if(timestamp[idx][evict_cache_j]) num_cache_evict++;

    //update access time and tag and valid
    valid[idx][evict_cache_j] = 1;
    cache[idx][evict_cache_j] = tag;
    timestamp[idx][evict_cache_j] = t;
    return;
}