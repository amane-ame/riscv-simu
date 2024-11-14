编译直接 `make release`, 然后 `build/simulator elf_file`.

单步模式加个 `-s`, 命令都和 gdb 几乎一样. 输出所有寄存器用 `p $`, 输出内存用 `x addr_hex`.

写的 RV64IM, 五级流水线, 分支预测 never taken.

内存配置和 cache 配置之类的可以到 `config.hpp` 里面改.

附带了 `test_fibonacci` 测试, 它会计算第 20 个斐波那契数并且用 write 输出.

附带了 `test_readwrite` 测试, 它会读入一行字符串然后原样输出.

测试程序用 `riscv64-unkonwing-elf-gcc -Wa, -march=rv64i -o xxx xxx.c` 编译.
