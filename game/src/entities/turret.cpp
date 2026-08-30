#include "entities/turret.h"

#include "state/mining_state.h"

// assets
#include "bn_sprite_items_dev16.h"

#include "bn_log.h"

namespace game {

turret::turret(
		mining_state &p_state,
		bn::fixed_point p_position) :
		entity(p_state),
		_sprite(bn::sprite_items::dev16.create_sprite()),
		_health(15),
		_fire_cooldown(0) {
	p_position += bn::fixed_point(8, 8);
	_sprite.set_camera(_state.get_camera());
	_sprite.set_position(p_position);
	_hitbox.set_width(16);
	_hitbox.set_height(16);
	_hitbox.set_position(p_position);
}

void turret::take_damage(unsigned int p_damage_amount) {
	if (p_damage_amount >= _health) {
		_health = 0;
	} else {
		_health -= p_damage_amount;
	}
}

bool turret::update() {
	if (_health == 0) {
		return false;
	}

	if (_fire_cooldown == 0) {
		auto target = _state.ship_hitbox.center();
		_state.spawn_projectile(
				false,
				5,
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
