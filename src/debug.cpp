#include <cstdlib>
#include <iostream>
#include <string>
#include <vector>
#include "config.hpp"
#include "debug.hpp"
#include "reg.hpp"
#include "simu.hpp"

static int scan(void)
{
    while(std::cin)
    {
        int x;
        if(std::cin >> std::hex >> x)
            return x;
        else
        {
            std::cin.clear();
            std::cin.ignore(32767, '\n');
        }
    }

    throw std::runtime_error("Error: stdin stream was closed");
}

debug::debug(simu *simulator)
{
    simulator-> pc = 0;

    return;
}

void debug::interactive(simu *simulator)
{
    static bool first = true;

    while(true)
    {
        std::cout << "(sim) ";
        if(!first)
        {
            std::cin.clear();
            std::cin.ignore(32767, '\n');
        }
        first = false;

        std::string op;
        std::cin >> op;
        if(op == "r" || op == "run")
        {
            if(simulator->pc)
                std::cout << "error: program is already running" << std::endl;
            else
            {
                std::string file = simulator->file;
                delete simulator;
                simulator = new simu(file);
                simulator->debugger = this;
                simulator->run();
                simulator->pc = 0;
            }
        }
        else if(op == "k" || op == "kill")
        {
            if(!simulator->pc)
                std::cout << "error: program is not running" << std::endl;
            else
                simulator->pc = 0;
        }
        else if(op == "c" || op == "continue")
        {
            if(!simulator->pc)
                std::cout << "error: program is not running" << std::endl;
            else
            {
                simulator->step_mode = simu::NONE;
                return;
            }
        }
        else if(op == "p" || op == "print")
        {
            if(!simulator->pc)
            {
                std::cout << "error: program is not running" << std::endl;
                continue;
            }

            std::vector<std::string> name(reg_name, reg_name + REG_NUM);
            std::string reg;
            std::cin >> reg;
            if(reg == "$")
                simulator->print_reg(false);
            else
            {
                int pos = std::find(name.begin(), name.end(), reg) - name.begin();
                if(pos == REG_NUM)
                    std::cout << "error: unknown register " << reg << std::endl;
                else
                    printf("%s=0x%lx(%ld)\n", reg.c_str(), simulator->reg[pos], simulator->reg[pos]);
            }
        }
        else if(op == "x" || op == "examine")
        {
            if(!simulator->pc)
            {
                std::cout << "error: program is not running" << std::endl;
                continue;
            }

            int pos = scan();
            if(pos < 0 || pos >= MEMORY_SIZE)
                std::cout << "error: out of memory address " << std::hex << pos << std::endl;
            else
            {
                int mem_cycle;
                unsigned char data;
                simulator->l1.request(pos, sizeof(char), 0, &data, mem_cycle);
                printf("%x:%02x\n", pos, data);
            }
        }
        else if(op == "b" || op == "break")
        {
            int pos = scan();
            if(breakpoints.count(pos))
                std::cout << "error: already breakpoint at " << std::hex << pos << std::endl;
            else
                breakpoints.insert(pos);
        }
        else if(op == "clear")
            breakpoints.clear();
        else if(op == "s" || op == "n" || op == "step" || op == "next")
        {
            if(!simulator->pc)
            {
                std::cout << "error: program is not running" << std::endl;
                continue;
            }
            simulator->step_mode = op[0] == 's' ? simu::TICK : simu::INSTRUCTION;

            return;
        }
        else if(op == "q" || op == "quit")
            exit(0);
        else
            std::cout << "error: unknown command " << op << std::endl;
    }

    return;
}
