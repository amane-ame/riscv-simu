#include <cstdio>
#include <string>
#include <vector>
#include "debug.hpp"
#include "simu.hpp"

int main(int argc, char *argv[])
{
    // without getopt

    if(sizeof(char) != 1)
    {
        fprintf(stderr, "error: sizeof(char) is not 1\n");
        return 2;
    }
    if(sizeof(short) != 2)
    {
        fprintf(stderr, "error: sizeof(short) is not 2\n");
        return 2;
    }
    if(sizeof(int) != 4)
    {
        fprintf(stderr, "error: sizeof(int) is not 4\n");
        return 2;
    }
    if(sizeof(long) != 8)
    {
        fprintf(stderr, "error: sizeof(long) is not 8, you may need to change all 'long' to 'long long' manually\n");
        return 2;
    }

    std::vector<std::string> args(argv, argv + argc);
    bool step = false;

    if(args.size() >= 2 && (args[1] == "-s" || args[1] == "--step"))
    {
        step = true;
        args.erase(args.begin() + 1);
    }

    if(args.size() < 2 || args[1] == "-h" || args[1] == "--help")
    {
        fprintf(stderr, "Usage: %s [options] elf_file\n\n"
                        "Options:\n"
                        "    -s, --step          Single step mode\n"
                        "    -h, --help          Print this help\n", args[0].c_str());

        return 1;
    }

    try
    {
        simu *simulator = new simu(args[1]);
        if(!step)
            simulator->run();
        else
        {
            debug debugger(simulator);
            debugger.interactive(simulator);
        }
    }
    catch(std::exception &e)
    {
        fputs(e.what(), stderr);
        return 2;
    }

    return 0;
}
