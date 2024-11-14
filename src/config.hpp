#pragma once

#define MEMORY_SIZE (1 << 25)

#define ALU_CYCLE_INT  1
#define ALU_CYCLE_MUL  2
#define ALU_CYCLE_DIV 40

#define L1_LATENCY {1, 0}
#define L2_LATENCY {8, 6}
#define L3_LATENCY {20, 20}
#define MEMORY_LATENCY {70, 30}

#define L1_CONFIG {32 * 1024, 8, 32 * 1024 / 8 / 64, 0, 1}
#define L2_CONFIG {256 * 1024, 8, 256 * 1024 / 8 / 64, 0, 1}
#define L3_CONFIG {8192 * 1024, 8, 8192 * 1024 / 8 / 64, 0, 1}
