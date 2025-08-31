#include <unity.h>

#include "../common/test_max2871.cpp"

extern void runAllTests();  // from test/common/test_max2871.cpp

int main(int argc, char **argv) {
    UNITY_BEGIN();
    runAllTests();
    UNITY_END();
    return 0;
}
