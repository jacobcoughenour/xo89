#pragma once

#include "bn_fixed_rect.h"

namespace game {

class mining_state;

class targetable {
public:
	virtual ~targetable() {}
	virtual void take_damage(unsigned int p_damage_amount) = 0;
	virtual bn::fixed_rect get_hitbox() = 0;
};

} //namespace game