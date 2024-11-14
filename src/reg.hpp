#pragma once

#include <map>

#define REG_NUM    32

#define REG_RA      1
#define REG_SP      2
#define REG_A0     10
#define REG_A1     11
#define REG_A2     12
#define REG_A3     13
#define REG_A4     14
#define REG_A5     15
#define REG_A6     16
#define REG_A7     17

#define OP_RR      0x33
#define OP_RRW     0x3b
#define OP_LOAD    0x03
#define OP_RI      0x13
#define OP_RIW     0x1b
#define OP_JALR    0x67
#define OP_ECALL   0x73
#define OP_STORE   0x23
#define OP_BRANCH  0x63
#define OP_AUIPC   0x17
#define OP_LUI     0x37
#define OP_JAL     0x6f

#define ALU_ADD     0
#define ALU_SUB     1
#define ALU_MUL     2
#define ALU_MULH    3
#define ALU_MULHSU  4
#define ALU_MULHU   5
#define ALU_DIV     8
#define ALU_DIVU    9
#define ALU_REM     6
#define ALU_REMU    7
#define ALU_SLL    10
#define ALU_SRA    11
#define ALU_SRL    12
#define ALU_XOR    13
#define ALU_OR     14
#define ALU_AND    15
#define ALU_SLT    16
#define ALU_SLTU   17

#define SYS_CLOSE  57
#define SYS_READ   63
#define SYS_WRITE  64
#define SYS_EXIT   93

const char *const reg_name[32] =
{
    "zero", "ra", "sp", "gp", "tp", "t0", "t1", "t2",
    "s0", "s1", "a0", "a1", "a2", "a3", "a4", "a5",
    "a6", "a7", "s2", "s3", "s4", "s5", "s6", "s7",
    "s8", "s9", "s10", "s11", "t3", "t4", "t5", "t6",
};

static const std::map<int, std::map<int, int>> mapR =
{
    {0x00, {{0x00, ALU_ADD},
            {0x01, ALU_MUL},
            {0x20, ALU_SUB}}},
    {0x01, {{0x00, ALU_SLL},
            {0x01, ALU_MULH}}},
    {0x02, {{0x00, ALU_SLT},
            {0x01, ALU_MULHSU}}},
    {0x03, {{0x00, ALU_SLTU},
            {0x01, ALU_MULHU}}},
    {0x04, {{0x00, ALU_XOR},
            {0x01, ALU_DIV}}},
    {0x05, {{0x00, ALU_SRL},
            {0x01, ALU_DIVU},
            {0x20, ALU_SRA}}},
    {0x06, {{0x00, ALU_OR},
            {0x01, ALU_REM}}},
    {0x07, {{0x00, ALU_AND},
            {0X01, ALU_REMU}}},
};

static const std::map<int, std::map<int, int>> mapI =
{
    {0x00, {{0x00, ALU_ADD}}},
    {0x01, {{0x00, ALU_SLL}}},
    {0x02, {{0x00, ALU_SLT}}},
    {0x03, {{0x00, ALU_SLTU}}},
    {0x04, {{0x00, ALU_XOR}}},
    {0x05, {{0x00, ALU_SRL},
            {0x10, ALU_SRA}}},
    {0x06, {{0x00, ALU_OR}}},
    {0x07, {{0x00, ALU_AND}}},
};

class reg_base
{
public:
    bool stall;
    bool bubble;

    reg_base() = default;
    ~reg_base() = default;
};

class reg_if : public reg_base
{
public:
    int pred_pc;
};

class reg_id : public reg_base
{
public:
    int inst;
    int valP;
    int pred_pc;
};

class reg_ex : public reg_base
{
public:
    int opcode, funct3;
    int valP;
    unsigned long val1, val2, imm;
    int alu;
    int rs1, rs2, rd;
    int pred_pc;
};

class reg_mem : public reg_base
{
public:
    int opcode, funct3;
    bool cond;
    unsigned long val2, valE;
    int rd;
    int pred_pc;
};

class reg_wb : public reg_base
{
public:
    int opcode;
    unsigned long val;
    int rd;
};

template<typename T>
void reg_set(T &l, const T &r)
{
    if(l.stall)
        l.stall = false;
    else if(r.bubble)
    {
        l = {};
        l.bubble = true;
    }
    else
        l = r;

    return;
}

unsigned long sign_extend(unsigned long x, int len);
unsigned long zero_extend(unsigned long x, int len);
static inline int bits(int x, int start, int len);

static inline void parseR (int x, reg_ex &e, int &funct7);
static inline void parseI (int x, reg_ex &e, int &funct7);
static inline void parseS (int x, reg_ex &e);
static inline void parseSB(int x, reg_ex &e);
static inline void parseU (int x, reg_ex &e);
static inline void parseUJ(int x, reg_ex &e);

void parse_inst(int x, reg_ex &e);
