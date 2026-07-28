#pragma once

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
		BN_ASSERT(helpers::truncate(4.22) == 4);
		BN_ASSERT(helpers::truncate(-4.22) == -4);

		BN_ASSERT(helpers::fmod(1.0, 1.0) == 0.0);
		BN_ASSERT(helpers::fmod(1.2, 1.0) == 0.2);
		BN_ASSERT(helpers::fmod(-0.25, 1.5) == -0.25);

		BN_ASSERT(helpers::fmod1(-0.2) == -0.2);
		BN_ASSERT(helpers::fmod1(0.5) == 0.5);
		BN_ASSERT(helpers::fmod1(-0.25) == -0.25);
		BN_ASSERT(helpers::fmod1(-1.25) == -0.25);

		BN_ASSERT(helpers::fposmod(0.01, 1.0) == 0.01);
		BN_ASSERT(helpers::fposmod(5.0, helpers::PI_2) == 5.0);
		BN_ASSERT(helpers::fposmod(0.0, helpers::PI_2) == 0.0);
		BN_ASSERT(helpers::fposmod(-0.0, helpers::PI_2) == 0.0);
		BN_ASSERT(helpers::fposmod(-0.2, helpers::PI_2) == helpers::PI_2 - 0.2);
		BN_ASSERT(helpers::fposmod(-0.1, helpers::PI_2) == helpers::PI_2 - 0.1);

		BN_ASSERT(helpers::fposmod1(-0.25) == 0.75);
	}
};
} //namespace game