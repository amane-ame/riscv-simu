#pragma once

#include <set>
#include "simu.hpp"

class debug
{
public:
    std::set<int> breakpoints;

    debug(simu *simulator);

    bool insert(int pc);
    void interactive(simu *simulator);
};
