#include "entities/floating_item.h"

#include "state/mining_state.h"

namespace game {

floating_item::floating_item(
		mining_state &p_state,
		item_type p_type,
		unsigned char p_sprite_index,
		bn::fixed_point p_init_pos,
		bn::fixed_point p_init_velocity) :
		entity(p_state),
		_type(p_type),
		_sprite_index(p_sprite_index),
		_position(p_init_pos),
		_velocity(p_init_velocity) {
}

bool floating_item::update() {
	auto ship_pos = _state.ship_hitbox.center();
	bn::fixed len = helpers::distance(ship_pos, _position);

	if (len < 2) {
		_state.pickup_resource(_type, 1);
		return false;
	}

	bn::fixed_point dir = helpers::normalize_point(ship_pos - _position);

	auto level = _state.get_shared().get_upgrade_level(upgrade_type::MAGNET);
	// attractor influence distance
	int d = 30 + level * 20;
	// push object towards ship
	_velocity += dir * bn::min(bn::fixed(10 + level * 3), bn::max(d - len, bn::fixed(0.2))) * bn::fixed(0.03);

	// move by velocity
	auto obj_desired_pos = _position + _velocity;

	if (_state.is_solid_tile(_state.space_point_to_tile_point(obj_desired_pos))) {
		// bounce
		_velocity *= bn::fixed(-0.94);
	} else {
		_position = obj_desired_pos;
		// velocity damping
		_velocity *= bn::fixed(0.98);
		// gravity
		_velocity += bn::fixed_point(0.0, 0.08);
	}

	return true;
}

} //namespace game
