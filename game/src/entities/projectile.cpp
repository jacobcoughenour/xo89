#include "entities/projectile.h"

#include "state/mining_state.h"

namespace game {

projectile::projectile(
		mining_state &p_state,
		bn::fixed_point p_init_pos,
		bn::fixed_point p_init_velocity) :
		entity(p_state),
		_position(p_init_pos),
		_velocity(p_init_velocity) {}

bool projectile::update() {
	// move by velocity
	auto obj_desired_pos = _position + _velocity;

	// todo proper ray casting?

	if (_state.is_solid_tile(_state.space_point_to_tile_point(obj_desired_pos))) {
		return false;
	} else {
		_position = obj_desired_pos;
	}

	return true;
}

} //namespace game
