#include "flying_scene.h"

// assets
#include "bn_sprite_items_blue_sprite.h"

namespace game {

flying_scene::flying_scene(game_state &state) :
	_state(state),
	_ship_sprite(bn::sprite_items::blue_sprite.create_sprite()) {
}

flying_scene::~flying_scene() {
}

bn::optional<scene_type> flying_scene::update() {
	bn::optional<scene_type> result;
	return result;
}
}