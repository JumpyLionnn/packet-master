#include <stdlib.h>
#include <stdio.h>
#include <packet_master.h>

int expect_int_eq(int a, int b) {
    if (a == b) {
        return 0;
    }
    else {
        printf("Test failed: expected %i == %i.\n", a, b);
        return 1;
    }
}

int main()
{
    int failed = 0;

    failed += expect_int_eq(value(), 2);

    if (failed == 0)
    {
        printf("All tests passed\n");
        return EXIT_SUCCESS;
    }
    else
    {
        printf("%i tests failed\n", failed);
        return EXIT_FAILURE;
    }
}