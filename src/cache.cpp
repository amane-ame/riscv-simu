#include <cstring>
#include "cache.hpp"
#include "config.hpp"

void storage::set_latency(StorageLatency sl)
{
    latency = sl;

    return;
}

block::block(void) : valid(0), write(0), tag(0), bits(NULL)
{
    return;
}

block::~block(void)
{
    if(bits)
        delete[] bits;

    return;
}

void block::resize(int block_size)
{
    if(bits)
        delete[] bits;
    bits = new unsigned char[block_size]();

    return;
}

cache::cache(void) : blocks(NULL)
{
    return;
}

cache::~cache(void)
{
    if(blocks)
    {
        for(int i = 0; i < config.set_num; i ++)
            delete[] blocks[i];
        delete[] blocks;
    }

    return;
}

void cache::set_config(CacheConfig cc)
{
    config = cc;
    block_size = cc.size / cc.associativity / cc.set_num;
    blocks = new class block *[cc.set_num];
    for(int i = 0; i < cc.set_num; i ++)
    {
        blocks[i] = new class block[cc.associativity];
        for(int j = 0; j < cc.associativity; j ++)
            blocks[i][j].resize(block_size);
    }

    return;
}

void cache::set_lower(storage *ll)
{
    lower = ll;

    return;
}

void cache::request(unsigned long addr, int bytes, int read, unsigned char *content, int &time)
{
    unsigned long cache_addr, write_addr;
    int s = 32 - __builtin_clz(config.set_num - 1);
    int b = 32 - __builtin_clz(block_size - 1);

    set = (addr << (64 - s - b)) >> (64 - s);
    block = (addr << (64 - b)) >> (64 - b);
    tag = addr >> (s + b);

    time = 0;
    if(replace_decision())
        replace_algorithm();
    else
    {
        time += latency.bus_latency + latency.hit_latency;
        if(read)
            memcpy(content, blocks[set][line].bits + block, bytes);
        else
        {
            memcpy(blocks[set][line].bits + block, content, bytes);
            if(!config.write_through)
                blocks[set][line].write = 1;
            else
            {
                int lower_time;
                lower->request(addr, bytes, 0, content, lower_time);
                time += lower_time;
            }
        }
        
        return;
    }

    cache_addr = (tag << (s + b)) | (set << b);
    write_addr = (blocks[set][line].tag << (s + b)) | (set << b);
    time += latency.bus_latency;

    unsigned char *tmp_content = new unsigned char[block_size];
    int lower_time;

    if(read)
    {
        lower->request(cache_addr, block_size, read, tmp_content, lower_time);
        time += lower_time;

        memcpy(content, tmp_content + block, bytes);
        if(!config.write_through && blocks[set][line].write)
        {
            lower->request(write_addr, block_size, 0, blocks[set][line].bits, lower_time);
            time += lower_time;
        }

        memcpy(blocks[set][line].bits, tmp_content, block_size);
        int tmp_valid = blocks[set][line].valid;
        for(int i = 0; i < config.associativity; i ++)
            if(blocks[set][i].valid > tmp_valid)
                blocks[set][i].valid -= 1;
        blocks[set][line].valid = config.associativity;
        blocks[set][line].tag = tag;
    }
    else
    {
        if(config.write_allocate)
        {
            if(!config.write_through && blocks[set][line].write)
            {
                lower->request(write_addr, block_size, 0, blocks[set][line].bits, lower_time);
                time += lower_time;
            }

            lower->request(cache_addr, block_size, 1, blocks[set][line].bits, lower_time);
            time += lower_time;

            memcpy(blocks[set][line].bits + block, content, bytes);
            int tmp_valid = blocks[set][line].valid;
            for(int i = 0; i < config.associativity; i ++)
                if(blocks[set][i].valid > tmp_valid)
                    blocks[set][i].valid -= 1;
            blocks[set][line].valid = config.associativity;
            blocks[set][line].tag = tag;

            if(!config.write_through)
                blocks[set][line].write = 1;
            else
            {
                lower->request(addr, bytes, 0, blocks[set][line].bits, lower_time);
                time += lower_time;
            }
        }
        else
        {
            lower->request(addr, bytes, 0, content, lower_time);
            time += lower_time;
        }
    }

    delete[] tmp_content;
    return;
}

bool cache::replace_decision(void)
{
    for(line = 0; line < config.associativity; line ++)
        if(blocks[set][line].valid && blocks[set][line].tag == tag)
            return false;

    return true;
}

void cache::replace_algorithm(void)
{
    int victim = -1;

    for(line = 0; line < config.associativity; line ++)
    {
        if(!blocks[set][line].valid)
            return;
        if(blocks[set][line].valid == 1)
            victim = line;
    }
    line = victim;

    return;
}

memory::memory(void) : data(new unsigned char[MEMORY_SIZE])
{
    return;
}

memory::~memory(void)
{
    delete[] data;

    return;
}

void memory::request(unsigned long addr, int bytes, int read, unsigned char *content, int &time)
{
    time = latency.hit_latency + latency.bus_latency;
    if(read)
        memcpy(content, data + addr, bytes);
    else
        memcpy(data + addr, content, bytes);

    return;
}
