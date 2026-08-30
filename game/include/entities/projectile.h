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
			bool p_from_player,
			unsigned int p_damage_amount,
			bn::fixed_point p_init_pos,
			bn::fixed_point p_init_velocity);
	bool update() override;

private:
	bool _from_player;
	unsigned int _damage_amount;
	bn::fixed_point _position;
	bn::fixed_point _velocity;
	int _frame;

public:
	bn::fixed_point get_position() { return _position; }
	bn::fixed_point get_velocity() { return _velocity; }
	int get_frame() { return _frame; }
};

} //namespace game
