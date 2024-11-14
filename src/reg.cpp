#include <cstdio>
#include <stdexcept>
#include <string>
#include "reg.hpp"

unsigned long sign_extend(unsigned long x, int len)
{
    x = (-1ULL << len) | x;
    x += (((x >> (len - 1)) & 1) ^ 1) << len;

    return x;
}

unsigned long zero_extend(unsigned long x, int len)
{
    return x & (-1ULL >> (64 - len));
}


static inline int bits(int x, int start, int len)
{
    return ((x >> start) & ((1 << len) - 1));
}

static inline void parseR(int x, reg_ex &e, int &funct7)
{
    e.rd = bits(x, 7, 5);
    e.funct3 = bits(x, 12, 3);
    e.rs1 = bits(x, 15, 5);
    e.rs2 = bits(x, 20, 5);
    funct7 = bits(x, 25, 7);

    return;
}

static inline void parseI(int x, reg_ex &e, int &funct7)
{
    e.rd = bits(x, 7, 5);
    e.funct3 = bits(x, 12, 3);
    e.rs1 = bits(x, 15, 5);
    e.imm = bits(x, 20, 12);
    funct7 = bits(x, 25, 7);

    return;
}

static inline void parseS(int x, reg_ex &e)
{
    e.funct3 = bits(x, 12, 3);
    e.rs1 = bits(x, 15, 5);
    e.rs2 = bits(x, 20, 5);
    e.imm = bits(x, 25, 7) << 5 | bits(x, 7, 5);

    return;
}

static inline void parseSB(int x, reg_ex &e)
{
    e.funct3 = bits(x, 12, 3);
    e.rs1 = bits(x, 15, 5);
    e.rs2 = bits(x, 20, 5);
    e.imm = bits(x, 31, 1) << 12 | bits(x, 25, 6) << 5 | bits(x, 8, 4) << 1 | bits(x, 7, 1) << 11;

    return;
}

static inline void parseU(int x, reg_ex &e)
{
    e.rd = bits(x, 7, 5);
    e.imm = bits(x, 12, 20) << 12;

    return;
}

static inline void parseUJ(int x, reg_ex &e)
{
    e.rd = bits(x, 7, 5);
    e.imm = bits(x, 31, 1) << 20 | bits(x, 21, 10) << 1 | bits(x, 20, 1) << 11 | bits(x, 12, 8) << 12;

    return;
}

void parse_inst(int x, reg_ex &e)
{
    e.opcode = bits(x, 0, 7);
    int funct7 = 0;

    switch(e.opcode)
    {
    case OP_RR:
    case OP_RRW:
        parseR(x, e, funct7);
        e.alu = mapR.at(e.funct3).at(funct7);
        break;
    case OP_LOAD:
        parseI(x, e, funct7);
        e.imm = sign_extend(e.imm, 12);
        break;
    case OP_RI:
        parseI(x, e, funct7);
        if(e.funct3 == 0x01 || e.funct3 == 0x05)
        {
            funct7 >>= 1;
            e.imm &= 0x3F;
        }
        else
        {
            funct7 = 0;
            e.imm = sign_extend(e.imm, 12);
        }
        e.alu = mapI.at(e.funct3).at(funct7);
        break;
    case OP_RIW:
        parseI(x, e, funct7);
        if(e.funct3 == 0x01 || e.funct3 == 0x05)
        {
            funct7 >>= 1;
            e.imm &= 0x1F;
        }
        else
        {
            funct7 = 0;
            e.imm = sign_extend(e.imm, 12);
        }
        e.alu = mapI.at(e.funct3).at(funct7);
        break;
    case OP_JALR:
        parseI(x, e, funct7);
        e.imm = sign_extend(e.imm, 12);
        break;
    case OP_ECALL:
        break;
    case OP_STORE:
        parseS(x, e);
        e.imm = sign_extend(e.imm, 12);
        break;
    case OP_BRANCH:
        parseSB(x, e);
        e.imm = sign_extend(e.imm, 12);
        break;
    case OP_AUIPC:
    case OP_LUI:
        parseU(x, e);
        e.imm = sign_extend(e.imm, 32);
        break;
    case OP_JAL:
        parseUJ(x, e);
        e.imm = sign_extend(e.imm, 21);
        break;
    default:
        char str[11];
        snprintf(str, 11, "%08x", x);
        throw std::runtime_error("unknown instruction " + std::string(str));
    }

    return;
}
