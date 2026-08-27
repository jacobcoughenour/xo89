#pragma once

#include "entity.h"
#include "helpers.h"
#include "items.h"

#include "bn_fixed.h"
#include "bn_fixed_point.h"

namespace game {

class mining_state;

class projectile : public entity {
public:
	explicit projectile(
			mining_state &p_state,
			bn::fixed_point p_init_pos,
			bn::fixed_point p_init_velocity);
	bool update() override;

private:
	bn::fixed_point _position;
	bn::fixed_point _velocity;

public:
	bn::fixed_point get_position() { return _position; }
	bn::fixed_point get_velocity() { return _velocity; }
};

} //namespace game
