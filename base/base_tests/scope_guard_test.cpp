#include "../../base/SRC_FIRST.hpp"
#include "../../testing/testing.hpp"

#include "../scope_guard.hpp"

static bool b = false;

void SetB()
{
    b = true;
}

UNIT_TEST(ScopeGuard)
{
    {
        b = false;
        MY_SCOPE_GUARD(guard, &SetB);
        TEST_EQUAL(b, false, ("Start test condition"));
    }
    TEST_EQUAL(b, true, ("scope_guard works in destructor"));
}

UNIT_TEST(ScopeGuardRelease)
{
    {
        b = false;
        MY_SCOPE_GUARD(guard, &SetB);
        TEST_EQUAL(b, false, ("Start test condition"));
        guard.release();
    }
    TEST_EQUAL(b, false, ("If relese() was called then scope_guard shouldn't work"));
}
