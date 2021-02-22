#include <stdio.h>
#include <limits.h>

int main(int argc, char *argv[])
{
    printf("%d\n", INT_MIN);
    printf("%d\n", -1-INT_MIN);
    printf("%d\n", -1+INT_MIN);
    printf("%d\n", 1+(-INT_MIN));
    printf("%d\n", 0-INT_MIN);
    return 0;
}
