#include "entities/creep.h"

#include "state/mining_state.h"

// assets
#include "bn_sprite_items_dev8.h"

namespace game {

creep::creep(
		mining_state &p_state,
		bn::fixed_point p_spawn_position) :
		combat_entity(p_state, 15),
		_sprite(bn::sprite_items::dev8.create_sprite()) {
	_sprite.set_camera(_state.get_camera());
	_sprite.set_position(p_spawn_position);
	_hitbox.set_width(8);
	_hitbox.set_height(8);
	_hitbox.set_position(p_spawn_position);
}

bool creep::update() {
	if (!combat_entity::update()) {
		return false;
	}

	auto target = _state.ship_hitbox.center();
	auto dist = target - _hitbox.position();
	auto dir = helpers::set_length(dist, 0.4);
	auto desired_pos = _hitbox.position() + dir;

	if (!_state.is_solid_tile(_state.space_point_to_tile_point(desired_pos))) {
		_hitbox.set_position(desired_pos);
		_sprite.set_position(desired_pos);
	}

	return true;
}

} //namespace game
