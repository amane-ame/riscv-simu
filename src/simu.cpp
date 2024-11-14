#include <algorithm>
#include <string>
#include <unistd.h>
#include <vector>
#include "config.hpp"
#include "debug.hpp"
#include "reg.hpp"
#include "simu.hpp"

simu::simu(const std::string &filename) : file(filename), debugger(NULL), elf(filename.c_str(), mem.data, pc), step_mode(NONE)
{
    std::fill(reg, reg + REG_NUM, 0);
    F = {};
    D = {};
    E = {};
    M = {};
    W = {};

    l1.set_lower(&l2);
    l2.set_lower(&l3);
    l3.set_lower(&mem);

    l1.set_latency(L1_LATENCY);
    l2.set_latency(L2_LATENCY);
    l3.set_latency(L3_LATENCY);
    mem.set_latency(MEMORY_LATENCY);

    l1.set_config(L1_CONFIG);
    l2.set_config(L2_CONFIG);
    l3.set_config(L3_CONFIG);

    return;
}

simu::~simu(void)
{
    return;
}

void simu::print_reg(bool indent)
{
    if(indent)
        printf("    ");
    printf("registers:");
    for(int i = 0; i < REG_NUM; i ++)
    {
        if(indent)
            printf("    ");
        if(!(i % 8))
            printf("\n");

        printf("%4s=", reg_name[i]);
        if(reg[i] == ~0UL)
            printf("~0UL    ");
        else
            printf("%-8lx", reg[i]);
    }
    printf("\n");

    return;
}

int simu::IF(void)
{
    int pc = F.pred_pc;

    if(((M.opcode == OP_BRANCH && M.cond) || M.opcode == OP_JALR || M.opcode == OP_JAL) && (unsigned long)M.pred_pc != M.valE)
        pc = M.valE;
    else if(M.opcode == OP_BRANCH && !M.cond && (unsigned long)M.pred_pc != M.val2)
        pc = M.val2;

    int if_cycle = 1;
    l1.request(pc, sizeof(int), 1, (unsigned char *)&d.inst, if_cycle);
    d.valP = pc;

    // never taken
    d.pred_pc = f.pred_pc = pc + 4;
    // if(step_mode != NONE)
    //     printf("pc=%x M.opcode(%x) pred_pc(%x) valE(%lx) f.pred_pc=%x(%d %d)\n", pc, M.opcode, M.pred_pc, M.valE, f.pred_pc, (int)F.stall, (int)f.bubble);

    return if_cycle;
}

int simu::ID(void)
{
    if(D.bubble)
        return 0;

    e.valP = D.valP;
    e.pred_pc = D.pred_pc;

    parse_inst(D.inst, e);

    e.val1 = reg[e.rs1];
    e.val2 = reg[e.rs2];

    return 1;
}

int simu::EX(void)
{
    if(E.bubble)
        return 0;

    m.opcode = E.opcode;
    m.funct3 = E.funct3;
    m.rd = E.rd;
    m.pred_pc = E.pred_pc;

    if(E.opcode == OP_BRANCH || E.opcode == OP_JALR || E.opcode == OP_JAL)
        m.val2 = E.valP + 4;
    else
        m.val2 = E.val2;

    unsigned long valA, valB;
    switch(E.opcode)
    {
    case OP_RR:
    case OP_RRW:
        valA = E.val1;
        valB = E.val2;
        break;
    case OP_BRANCH:
    case OP_AUIPC:
    case OP_JAL:
        valA = E.valP;
        valB = E.imm;
        break;
    default:
        valA = E.val1;
        valB = E.imm;
        break;
    }

    int ex_cycle = ALU_CYCLE_INT;
    switch(E.alu)
    {
    case ALU_ADD   : m.valE = valA + valB; break;
    case ALU_SUB   : m.valE = valA - valB; break;
    case ALU_MUL   : m.valE = valA * valB; ex_cycle = ALU_CYCLE_MUL; break;
    case ALU_MULH  : m.valE = ((__int128_t) valA * (__int128_t) valB) >> 64; ex_cycle = ALU_CYCLE_MUL; break;
    case ALU_MULHSU: m.valE = ((__int128_t) valA * (__uint128_t)valB) >> 64; ex_cycle = ALU_CYCLE_MUL; break;
    case ALU_MULHU : m.valE = ((__uint128_t)valA * (__uint128_t)valB) >> 64; ex_cycle = ALU_CYCLE_MUL; break;
    case ALU_DIV   : m.valE = (long)valA / (long)valB; ex_cycle = ALU_CYCLE_DIV; break;
    case ALU_DIVU  : m.valE =       valA /       valB; ex_cycle = ALU_CYCLE_DIV; break;
    case ALU_REM   : m.valE = (long)valA % (long)valB; ex_cycle = ALU_CYCLE_DIV; break;
    case ALU_REMU  : m.valE =       valA %       valB; ex_cycle = ALU_CYCLE_DIV; break;
    case ALU_SLL   : m.valE =       valA << valB; break;
    case ALU_SRA   : m.valE = (long)valA >> valB; break;
    case ALU_SRL   : m.valE =       valA >> valB; break;
    case ALU_XOR   : m.valE = valA ^ valB; break;
    case ALU_OR    : m.valE = valA | valB; break;
    case ALU_AND   : m.valE = valA & valB; break;
    case ALU_SLT   : m.valE = (long)valA < (long)valB; break;
    case ALU_SLTU  : m.valE =       valA <       valB; break;
    }

    if(E.opcode == OP_RRW || E.opcode == OP_RIW)
    {
        m.valE = sign_extend(m.valE, 32);
        if(E.alu == ALU_MUL || E.alu == ALU_MULH || E.alu == ALU_MULHSU || E.alu == ALU_MULHU)
            ex_cycle = ALU_CYCLE_INT;
    }

    if(E.opcode == OP_BRANCH)
        switch(E.funct3)
        {
        case 0x00: m.cond = E.val1 == E.val2; break;
        case 0x01: m.cond = E.val1 != E.val2; break;
        case 0x04: m.cond = (long)E.val1 <  (long)E.val2; break;
        case 0x05: m.cond = (long)E.val1 >= (long)E.val2; break;
        case 0x06: m.cond =       E.val1 <        E.val2; break;
        case 0x07: m.cond =       E.val1 >=       E.val2; break;
        }

    return ex_cycle;
}

int simu::MEM(void)
{
    if(M.bubble)
        return 0;

    w.opcode = M.opcode;
    w.rd = M.rd;

    int mem_cycle = 1;
    switch(M.opcode)
    {
    case OP_LOAD:
        switch(M.funct3)
        {
        case 0:
        case 4:
            l1.request(M.valE, sizeof(char), 1, (unsigned char *)&w.val, mem_cycle);
            break;
        case 1:
        case 5:
            l1.request(M.valE, sizeof(short), 1, (unsigned char *)&w.val, mem_cycle);
            break;
        case 2:
        case 6:
            l1.request(M.valE, sizeof(int), 1, (unsigned char *)&w.val, mem_cycle);
            break;
        case 3:
        case 7:
            l1.request(M.valE, sizeof(long), 1, (unsigned char *)&w.val, mem_cycle);
            break;
        }

        switch(M.funct3)
        {
        case 0:
        case 1:
        case 2:
            w.val = sign_extend(w.val, 8 << M.funct3);
            break;
        case 4:
        case 5:
        case 6:
            w.val = zero_extend(w.val, 8 << (M.funct3 - 4));
            break;
        }
        break;
    case OP_STORE:
        switch(M.funct3)
        {
        case 0:
            l1.request(M.valE, sizeof(char), 0, (unsigned char *)&M.val2, mem_cycle);
            break;
        case 1:
            l1.request(M.valE, sizeof(short), 0, (unsigned char *)&M.val2, mem_cycle);
            break;
        case 2:
            l1.request(M.valE, sizeof(int), 0, (unsigned char *)&M.val2, mem_cycle);
            break;
        case 3:
            l1.request(M.valE, sizeof(long), 0, (unsigned char *)&M.val2, mem_cycle);
            break;
        }
        break;
    case OP_JALR:
    case OP_JAL:
    case OP_BRANCH:
        w.val = M.val2;
        break;
    default:
        w.val = M.valE;
    }

    return mem_cycle;
}

int simu::WB(void)
{
    if(W.bubble)
        return 0;

    if(W.rd != 0)
    {
        reg[W.rd] = W.val;
        return 1;
    }

    return 0;
}

void simu::syscall(void)
{
    int sys_cycle;
    unsigned char *data = NULL;

    // printf("meet syscall %lu (%lu %s)\n", reg[REG_A7], reg[REG_A0], memory + reg[REG_A1]);
    switch(reg[REG_A7])
    {
    case SYS_CLOSE:
        reg[REG_A0] = close((unsigned int)reg[REG_A0]);
    case SYS_READ:
        data = new unsigned char[reg[REG_A2]];
        reg[REG_A0] = read((unsigned int)reg[REG_A0], data, (unsigned int)reg[REG_A2]);
        l1.request(reg[REG_A1], reg[REG_A2], 0, data, sys_cycle);
        break;
    case SYS_WRITE:
        data = new unsigned char[reg[REG_A2]];
        l1.request(reg[REG_A1], reg[REG_A2], 1, data, sys_cycle);
        reg[REG_A0] = write((unsigned int)reg[REG_A0], data, (unsigned int)reg[REG_A2]);
        break;
    case SYS_EXIT:
        throw std::runtime_error("exited");
    default:
        throw std::runtime_error("unknown syscall " + std::to_string(reg[REG_A7]));
    }

    if(data)
        delete[] data;
    return;
}

void simu::tick(unsigned long &instructions, unsigned long &pipelined_ticks, unsigned long &multicycle_ticks)
{
    static std::vector<int> if_cycle, id_cycle, ex_cycle, mem_cycle, wb_cycle;

    f = {};
    d = {};
    e = {};
    m = {};
    w = {};

    wb_cycle.push_back(WB());
    mem_cycle.push_back(MEM());
    ex_cycle.push_back(EX());
    id_cycle.push_back(ID());
    if_cycle.push_back(IF());
    if(wb_cycle.size() >= 6)
        wb_cycle.erase(wb_cycle.begin());
    if(mem_cycle.size() >= 6)
        mem_cycle.erase(mem_cycle.begin());
    if(ex_cycle.size() >= 6)
        ex_cycle.erase(ex_cycle.begin());
    if(id_cycle.size() >= 6)
        id_cycle.erase(id_cycle.begin());
    if(if_cycle.size() >= 6)
        if_cycle.erase(if_cycle.begin());

    if(W.opcode == OP_ECALL)
        syscall();

    if(step_mode == TICK || (step_mode == INSTRUCTION && E.valP) || (debugger && debugger->breakpoints.count(E.valP)))
    {
        if(!E.valP)
            printf("PC: bubble");
        else
            printf("PC: %06x", E.valP);

        int pc_cycle, in;
        l1.request(E.valP, sizeof(int), 0, (unsigned char *)&in, pc_cycle);
        printf("(%08x), pipelined_ticks: %lu, instructions: %lu, stopped\n", in, pipelined_ticks, instructions);
        debugger->interactive(this);
    }

    bool has_ecall = (E.opcode == OP_ECALL || M.opcode == OP_ECALL || W.opcode == OP_ECALL);
    bool has_jump  = (E.opcode == OP_JALR  || E.opcode == OP_JAL);
    // never taken
    bool mispredicted = (((E.opcode == OP_BRANCH && m.cond) || E.opcode == OP_JALR || E.opcode == OP_JAL) && (unsigned long)E.pred_pc != m.valE) ||
                          (E.opcode == OP_BRANCH && !m.cond && (unsigned long)E.pred_pc != m.val2);
    bool data_dependent = (e.rs1 && (E.rd == e.rs1 || M.rd == e.rs1 || W.rd == e.rs1)) ||
                          (e.rs2 && (E.rd == e.rs2 || M.rd == e.rs2 || W.rd == e.rs2));

    // if(step_mode != NONE)
    //     printf("mispredicted: %d\n", (int)mispredicted);
    d.bubble |= mispredicted;
    e.bubble |= mispredicted;

    F.stall  |= data_dependent || has_ecall;
    D.stall  |= data_dependent || has_ecall;
    D.stall  &= !has_jump;
    e.bubble |= data_dependent || has_ecall;

    e.bubble |= D.bubble;
    m.bubble |= E.bubble;
    w.bubble |= M.bubble;

    pipelined_ticks += std::max({1, if_cycle.back(), ex_cycle.back(), mem_cycle.back()});
    if(!W.stall && !W.bubble)
    {
        instructions ++;
        multicycle_ticks += wb_cycle[4] + mem_cycle[3] + ex_cycle[2] + id_cycle[1] + if_cycle[0];
    }

    // if(step_mode != NONE)
    //     printf("F.stall=%d D.stall=%d d.bubble=%d e.bubble=%d\n", (int)F.stall, (int)D.stall, (int)d.bubble, (int)e.bubble);
    reg_set(F, f);
    // if(step_mode != NONE)
    //     printf("after f=%x F=%x\n", f.pred_pc, F.pred_pc);
    reg_set(D, d);
    reg_set(E, e);
    reg_set(M, m);
    reg_set(W, w);

    return;
}

void simu::run(void)
{
    unsigned long instructions, pipelined_ticks, multicycle_ticks;

    try
    {
        reg[REG_SP] = MEMORY_SIZE / 2;
        D.bubble = E.bubble = M.bubble = W.bubble = true;
        F.pred_pc = pc;

        instructions = pipelined_ticks = multicycle_ticks = 0;
        while(true)
            tick(instructions, pipelined_ticks, multicycle_ticks);
    }
    catch(std::exception &e)
    {
        printf("========== above are user output ==========\n");
        if(std::string(e.what()) != "exited")
        {
            printf("program runtime error: %s\n", e.what());
            print_reg(true);
        }
        else
            printf("program exited normally\n");
        printf("    instructions:     %lu\n", instructions);
        printf("    pipelined ticks:  %lu, CPI: %06f\n", pipelined_ticks, (double)pipelined_ticks / instructions);
        printf("    multicycle ticks: %lu, CPI: %06f\n", multicycle_ticks, (double)multicycle_ticks / instructions);
    }

    return;
}
