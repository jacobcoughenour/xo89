#pragma once

#include "bn_optional.h"

#include "state/shared_state.h"

namespace game {

enum class scene_type;

class scene {
protected:
	scene(shared_state &p_shared) : _shared(p_shared) {}

	shared_state &_shared;

public:
	virtual ~scene() = default;
	// return the scene you want to switch to
	[[nodiscard]] virtual bn::optional<scene_type> update() = 0;
};

} //namespace game