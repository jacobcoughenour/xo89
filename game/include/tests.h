#ifndef TESTS_H
#define TESTS_H

#include "bn_assert.h"
#include "helpers.h"

namespace game {

class tests {
public:
	static void run_tests() {
		test_helpers();
	}

private:
	static void test_helpers() {
		BN_ASSERT(helpers::fmod(1.0, 1.0) == 0.0);
		BN_ASSERT(helpers::fmod(1.2, 1.0) == 0.2);
		BN_ASSERT(helpers::fmod1(0.5) == 0.5);
		BN_ASSERT(helpers::fmod1(-0.25) == 0.75);

		BN_ASSERT(helpers::fposmod1(-0.25) == 0.75);
	}
};
} //namespace game
#endif