#include <nano/lib/assert.hpp>

#include <gtest/gtest.h>

TEST (assert_DeathTest, debug_assert)
{
	debug_assert (true);
	ASSERT_DEATH (debug_assert (false), ".*Assertion \\(false\\) failed.*");
}

TEST (assert_DeathTest, release_assert)
{
	release_assert (true);
	ASSERT_DEATH (release_assert (false), ".*Assertion \\(false\\) failed.*");
}