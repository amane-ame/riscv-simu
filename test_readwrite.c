#include <stdio.h>

int main()
{
    int c;
    char s[233];

    read(0, s, 233);
    for(c = 0; s[c]; c ++)
        ;
    write(1, s, c);

    return 0;
}
