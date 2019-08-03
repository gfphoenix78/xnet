//
// Created by Hao Wu on 8/2/19.
//
#include <stdio.h>
#include "library.h"

int main(int ac, char *av[])
{
    int fds[128];
    int n = Listen(av[1], av[2], fds, 128);
    printf("n = %d\n", n);

    return 0;
}
