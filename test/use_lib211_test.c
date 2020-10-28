#include <211.h>

static void passing_checks(void)
{
    CHECK( 1 );
    CHECK_POINTER("a", "a");
}

static void failing_checks(void)
{
    CHECK_CHAR('A', 'B');
    CHECK_INT('A', 'B');
    CHECK_SIZE(-3, -5);
    CHECK_POINTER("a", "b");
}

int main(void)
{
    RUN_TEST(passing_checks);
    RUN_TEST(failing_checks);
}
