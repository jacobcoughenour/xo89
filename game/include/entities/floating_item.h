#pragma once

#include "entity.h"
#include "helpers.h"
#include "items.h"

#include "bn_fixed.h"
#include "bn_fixed_point.h"

namespace game {

class mining_state;

class floating_item : public entity {
public:
	explicit floating_item(
			mining_state &p_state,
			item_type p_type,
			unsigned char p_sprite_index,
			bn::fixed_point p_init_pos,
			bn::fixed_point p_init_velocity);
	bool update() override;

private:
	item_type _type;
	unsigned char _sprite_index;
	bn::fixed_point _position;
	bn::fixed_point _velocity;

public:
	item_type get_type() { return _type; }
	unsigned char get_sprite_index() { return _sprite_index; }
	bn::fixed_point get_position() { return _position; }
	bn::fixed_point get_velocity() { return _velocity; }
	void apply_impulse(bn::fixed_point p_impulse) {
		_velocity += p_impulse;
	}
};

} //namespace game
