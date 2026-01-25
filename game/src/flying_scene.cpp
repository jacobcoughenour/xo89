#include "flying_scene.h"

// assets
#include "bn_sprite_items_dev16.h"
#include "bn_sprite_items_dev32.h"
#include "bn_sprite_items_dev8.h"
#include "bn_sprite_items_ship.h"

namespace game {

flying_scene::flying_scene(game_state &state) :
		_state(state),
		_ship_sprite(bn::sprite_items::ship.create_sprite()) {
}

flying_scene::~flying_scene() {
}

bn::optional<scene_type> flying_scene::update() {
	bn::optional<scene_type> result;

	if (bn::keypad::left_held()) {
		_ship_rotation -= 0.01;
	}
	if (bn::keypad::right_held()) {
		_ship_rotation += 0.01;
	}
	_ship_rotation = helpers::fposmod1(_ship_rotation);

	_ship_sprite.set_tiles(bn::sprite_items::ship.tiles_item()
					.create_tiles((_ship_rotation * 16.0).floor_integer()));

	return result;
}
} //namespace game