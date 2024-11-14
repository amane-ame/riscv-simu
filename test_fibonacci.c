#include <stdio.h>

int to_str(char *s, int n)
{
    int c;

    if(!n)
        return 0;
    
    c = to_str(s, n / 10);
    s[c] = n % 10 + 48;

    return c + 1;
}

int main()
{
    int f[20], i, c;
    char s[20];

    f[0] = f[1] = 1;
    for(i = 2; i < 20; i ++)
        f[i] = f[i - 1] + f[i - 2];

    c = to_str(s, f[19]);
    s[c ++] = '\n';
    s[c] = 0;
    write(1, s, c);

    return 0;
}
