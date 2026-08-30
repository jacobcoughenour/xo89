#pragma once

#include "bn_bitset.h"
#include "bn_optional.h"

namespace game {

class mining_state;

enum class entity_type {
	FLOATING_ITEM,
	PROJECTILE,
	TURRET,
	CREEP
};

class entity {
protected:
	entity(mining_state &p_state) :
			_state(p_state) {}
	mining_state &_state;

public:
	virtual ~entity() = default;
	virtual bool update() = 0;
};

} //namespace game