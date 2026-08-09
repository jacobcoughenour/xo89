#pragma once

#include "bn_optional.h"

#include "scenes/scene_type.h"
#include "state/shared_state.h"

namespace game {

class state {
protected:
	state() = default;

public:
	virtual ~state() = default;

	virtual void update() = 0;
};

} //namespace game