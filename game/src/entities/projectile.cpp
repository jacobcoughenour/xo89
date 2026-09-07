#include "entities/projectile.h"

#include "state/mining_state.h"

namespace game {

projectile::projectile(
		mining_state &p_state,
		bool p_from_player,
		unsigned int p_damage_amount,
		unsigned int p_explosion_radius,
		bn::fixed_point p_init_pos,
		bn::fixed_point p_init_velocity) :
		entity(p_state),
		_from_player(p_from_player),
		_damage_amount(p_damage_amount),
		_explosion_radius(p_explosion_radius),
		_position(p_init_pos),
		_velocity(p_init_velocity) {
	// todo if velocity changes this should also update
	_frame = (((360 - helpers::dir_to_angle_deg(_velocity * 10.0) / bn::fixed(360.0)) * 16 - 0.5).integer() + 16) % 16;
}

bool projectile::update() {
	// move
	_position += _velocity;

	if (_state.is_solid_tile(_state.space_point_to_tile_point(_position))) {
		_explode();
		return false;
	}

	if (_from_player) {
		for (auto &t : _state.creeps) {
			if (t.get_hitbox().contains(_position)) {
				t.take_damage(_damage_amount);
				_explode();
				return false;
			}
		}
		for (auto &t : _state.turrets) {
			if (t.get_hitbox().contains(_position)) {
				t.take_damage(_damage_amount);
				_explode();
				return false;
			}
		}
	}

	if (!_from_player && _state.ship_hitbox.contains(_position)) {
		_state.take_damage(_damage_amount);
		_explode();
		return false;
	}

	return true;
}

void projectile::_explode() {
	if (_explosion_radius != 0) {
		_state.explode(_position, _explosion_radius);
	}
}

} //namespace game
