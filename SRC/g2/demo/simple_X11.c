#include <stdio.h>
#include <g2.h>
#include <g2_X11.h>

int main()
{
    int d;
    d=g2_open_X11(100, 100);
    g2_line(d, 10, 10, 90, 90);
    getchar();
    return 0;
}

