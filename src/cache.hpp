#pragma once

#include <stdint.h>

// Storage basic config
struct StorageLatency
{
    int hit_latency; // In nanoseconds
    int bus_latency; // Added to each request
};

class storage
{
protected:
    StorageLatency latency;

public:
    void set_latency(StorageLatency sl);

    // Main access process
    // [in]  addr: access address
    // [in]  bytes: target number of bytes
    // [in]  read: 0|1 for write|read
    // [i|o] content: in|out data
    // [out] hit: 0|1 for miss|hit
    // [out] time: total access time
    virtual void request(unsigned long addr, int bytes, int read, unsigned char *content, int &time) = 0;
};

class block
{
friend class cache;

private:
    unsigned char valid;
    unsigned char write;
    unsigned long tag;
    unsigned char *bits;

public:
    block(void);
    ~block(void);

    void resize(int block_size);
};

struct CacheConfig
{
    int size;
    int associativity;
    int set_num; // Number of cache sets
    int write_through; // 0|1 for back|through
    int write_allocate; // 0|1 for no-alc|alc
};

class cache: public storage
{
private:
    bool replace_decision(void);
    void replace_algorithm(void);

    block **blocks;
    int block_size;
    int set, line, block;
    unsigned long tag;

    CacheConfig config;
    storage *lower;

public:
    cache(void);
    ~cache(void);

    void set_config(CacheConfig cc);
    void set_lower(storage *ll);
    void request(unsigned long addr, int bytes, int read, unsigned char *content, int &time);
};

class memory : public storage
{
friend class simu;

private:
    unsigned char *data;

public:
    memory(void);
    ~memory(void);

    void request(unsigned long addr, int bytes, int read, unsigned char *content, int &time);
};
