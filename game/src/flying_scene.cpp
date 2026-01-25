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
		_ship_rotation -= 0.04;
	}
	if (bn::keypad::right_held()) {
		_ship_rotation += 0.04;
	}
	_ship_rotation = helpers::fposmod(_ship_rotation, helpers::PI_2);

	_ship_sprite.set_tiles(bn::sprite_items::ship.tiles_item()
					.create_tiles(((_ship_rotation / helpers::PI_2) * 16.0).floor_integer() % 16));

	if (bn::keypad::a_held()) {
		_ship_velocity -= helpers::rad_to_dir(-_ship_rotation) * bn::fixed(0.05);
	}

	_ship_velocity *= bn::fixed(0.99);

	bn::fixed_point pos = _ship_sprite.position();
	pos += _ship_velocity;
	_ship_sprite.set_position(pos);

	update_text();

	return result;
}

void flying_scene::update_text() {
	_state.small_fixed_text_generator.set_alignment(bn::sprite_text_generator::alignment_type::LEFT);

	_text_sprites.clear();
	bn::string<36> text;
	bn::ostringstream text_stream(text);
	text_stream.append(_ship_velocity.y());
	_state.small_fixed_text_generator.generate(16, 0, text, _text_sprites);
}

} //namespace game