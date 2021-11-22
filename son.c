#include <stdio.h>
#include <unistd.h>

void foo(int val) {
    if (val > 0) {
        printf("val: %d\n", val);
    } else {
        printf("val: %d\n", val + 1);
    }
}

void bar(int cnt) {
    int i;

    for (i = 0; i < cnt; i++) {
        foo(i);
    }
}

int main()
{
    bar(5);

    return 0;
}
