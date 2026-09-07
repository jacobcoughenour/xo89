#include "entities/turret.h"

#include "state/mining_state.h"

// assets
#include "bn_sprite_items_dev16.h"

namespace game {

turret::turret(
		mining_state &p_state,
		bn::fixed_point p_position) :
		combat_entity(p_state, 15, bn::sprite_items::dev16.create_sprite()),
		_fire_cooldown(0) {
	p_position += bn::fixed_point(8, 8);
	_sprite.set_camera(_state.get_camera());
	_sprite.set_position(p_position);
	_hitbox.set_width(16);
	_hitbox.set_height(16);
	_hitbox.set_position(p_position);
}

bool turret::update() {
	if (!combat_entity::update()) {
		return false;
	}

	if (_fire_cooldown == 0) {
		auto target = _state.ship_hitbox.center();
		_state.spawn_projectile(
				false,
				5,
				0,
				_hitbox.center(),
				helpers::set_length(target - _hitbox.center(), 1));
		_fire_cooldown = 90;
	}
	if (_fire_cooldown > 0) {
		_fire_cooldown--;
	}

	return true;
}

} //namespace game
