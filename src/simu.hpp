#pragma once

#include <string>
#include "cache.hpp"
#include "elf.hpp"
#include "reg.hpp"

class debug;

class simu
{
friend class debug;

private:
    std::string file;
    debug *debugger;

    memory mem;
    cache l1, l2, l3;
    unsigned long reg[REG_NUM];
    int pc;
    elf elf;
    enum
    {
        NONE,
        TICK,
        INSTRUCTION
    } step_mode;

    reg_if  F, f;
    reg_id  D, d;
    reg_ex  E, e;
    reg_mem M, m;
    reg_wb  W, w;

    int IF (void);
    int ID (void);
    int EX (void);
    int MEM(void);
    int WB (void);

    void print_reg(bool indent);
    void syscall(void);
    void tick(unsigned long &instructions, unsigned long &pipelined_ticks, unsigned long &multicycle_ticks);
public:
    simu(const std::string &filename);
    ~simu();

    void run(void);
};
