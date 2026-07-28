#include "flying_scene.h"

// assets
#include "bn_affine_bg_items_laser.h"
#include "bn_regular_bg_items_green_bg.h"
#include "bn_sprite_items_dev16.h"
#include "bn_sprite_items_dev32.h"
#include "bn_sprite_items_dev8.h"
#include "bn_sprite_items_ship.h"

#include "bn_music_items.h"

namespace game {

flying_scene::flying_scene(game_state &state) :
		_state(state),
		_camera(bn::camera_ptr::create(0, 0)),
		_rng(1),
		_bg_bg(bn::regular_bg_items::green_bg.create_bg()),
		_space(_camera),
		_ship_sprite(bn::sprite_items::ship.create_sprite()),
		_ship_laser(bn::affine_bg_items::laser.create_bg()) {
	bn::bg_palettes::set_transparent_color(bn::color(2, 3, 4));

	_ship_sprite.set_camera(_camera);
	_ship_sprite.set_bg_priority(0);
	_ship_sprite.set_position(_space.spawn_point());

	_ship_laser.set_wrapping_enabled(false);
	_ship_laser.set_pivot_position(bn::point(0, 64));
	// bn::music_items::demo_1.play();
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
					.create_tiles(((_ship_rotation / helpers::PI_2) * 32.0).floor_integer() % 32));

	if (bn::keypad::a_held()) {
		_ship_velocity -= helpers::rad_to_dir(-_ship_rotation) * bn::fixed(0.05);
	}

	_ship_velocity *= bn::fixed(0.99);

	bn::fixed_point pos = _ship_sprite.position();
	pos += _ship_velocity;
	_ship_sprite.set_position(pos);

	_camera.set_position(_ship_sprite.position());

	if (bn::keypad::b_held()) {
		_ship_laser.set_visible(true);

		_ship_laser.set_rotation_angle(helpers::remap_fixed(_ship_rotation, helpers::PI_2, 0, 0, 360));
		_ship_laser.set_horizontal_scale(_frame % 4 < 2 ? 0.05 : 0.06);

		// todo need to hit test to determine laser length
		_ship_laser.set_vertical_scale(1.2);

	} else {
		_ship_laser.set_visible(false);
	}

	_bg_bg.set_position(-_camera.position() / 2);

	_space.update();

	_frame++;

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