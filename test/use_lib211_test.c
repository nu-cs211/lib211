#include <211.h>

int main()
{
    CHECK( 1 );

    CHECK_CHAR('A', 'B');
    CHECK_INT('A', 'B');
    CHECK_SIZE(-3, -5);
    CHECK_POINTER("a", "a");
    CHECK_POINTER("a", "b");

}
