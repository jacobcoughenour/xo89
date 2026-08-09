#pragma once

#include "state/state.h"

namespace game {

class shared_state : public state {
public:
	shared_state() = default;
	void update() {}

	int balance = 0;

	// todo do save load here
};
} //namespace game