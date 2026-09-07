#include "entities/turret.h"

#include "state/mining_state.h"

// assets
#include "bn_sprite_items_turret.h"

namespace game {

turret::turret(
		mining_state &p_state,
		bn::fixed_point p_position,
		unsigned char p_direction) :
		combat_entity(p_state, 15, bn::sprite_items::turret.create_sprite()),
		_fire_cooldown(0),
		_direction(p_direction) {
	p_position += bn::fixed_point(8, 8);

	_sprite.set_camera(_state.get_camera());
	_sprite.set_position(p_position);
	_sprite.set_vertical_flip(p_direction == 2);
	_sprite.set_horizontal_flip(p_direction == 1);

	_hitbox.set_width(16);
	_hitbox.set_height(16);

	if (_direction == 0) {
		p_position.set_y(p_position.y() + 6);
	} else if (_direction == 1) {
		p_position.set_x(p_position.x() - 6);
	} else if (_direction == 2) {
		p_position.set_y(p_position.y() - 6);
	} else {
		p_position.set_x(p_position.x() + 6);
	}

	_hitbox.set_position(p_position);
}

bool turret::update() {
	if (!combat_entity::update()) {
		return false;
	}

	auto target = _state.ship_hitbox.center();
	auto dir = target - _hitbox.center();

	int tile_index = _direction % 2 == 0 ? 0 : 17;

	bool is_in_view = false;

	if (_direction == 0 && dir.y() < 0) {
		_aim_angle = helpers::dir_to_angle_deg(dir);
		is_in_view = true;
	} else if (_direction == 1 && dir.x() > 0) {
		_aim_angle = helpers::dir_to_angle_deg(dir);
		is_in_view = true;
	} else if (_direction == 2 && dir.y() > 0) {
		_aim_angle = helpers::dir_to_angle_deg(dir);
		is_in_view = true;
	} else if (_direction == 3 && dir.x() < 0) {
		_aim_angle = helpers::dir_to_angle_deg(dir);
		is_in_view = true;
	}

	if (_direction == 0) {
		tile_index += helpers::remap_fixed(helpers::dir_to_angle_deg(bn::fixed_point(dir.x(), -dir.y())), -90, 90, 0, 16).round_integer();
	} else if (_direction == 1) {
		tile_index += helpers::remap_fixed(_aim_angle, 0, 180, 0, 16).round_integer();
	} else if (_direction == 2) {
		tile_index += helpers::remap_fixed(_aim_angle, -90, 90, 0, 16).round_integer();
	} else if (_direction == 3) {
		tile_index += helpers::remap_fixed(_aim_angle, 0, -180, 0, 16).round_integer();
	}

	_sprite.set_tiles(bn::sprite_items::turret.tiles_item().create_tiles(tile_index % 34));

	if (_fire_cooldown == 0 && is_in_view) {
		dir = helpers::angle_to_dir(_aim_angle);
		_state.spawn_projectile(
				false,
				5,
				0,
				_hitbox.center() + dir * 8.0,
				helpers::angle_to_dir(_aim_angle));
		_fire_cooldown = 90;
	}
	if (_fire_cooldown > 0) {
		_fire_cooldown--;
	}

	return true;
}

} //namespace game
