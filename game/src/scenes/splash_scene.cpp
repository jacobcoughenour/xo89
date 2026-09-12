#include "scenes/splash_scene.h"

#include "helpers.h"
#include "scenes/scene_type.h"

#include "bn_bg_palettes.h"
#include "bn_blending.h"
#include "bn_keypad.h"

#include "bn_regular_bg_items_jam.h"
#include "bn_sound_items.h"
#include "bn_sprite_items_nostabyte.h"

namespace game {

splash_scene::splash_scene(shared_state &p_shared) :
		scene(p_shared),
		_nos_sprite(bn::sprite_items::nostabyte.create_sprite(0, 0)),
		_ta_sprite(bn::sprite_items::nostabyte.create_sprite(0, 0)),
		_byt_sprite(bn::sprite_items::nostabyte.create_sprite(0, 0)),
		_e_sprite(bn::sprite_items::nostabyte.create_sprite(0, 0)),
		_interac_sprite(bn::sprite_items::nostabyte.create_sprite(0, 0)),
		_tive_sprite(bn::sprite_items::nostabyte.create_sprite(0, 0)),
		_splash_sound(bn::sound_items::splash.play()),
		_jam_bg(bn::regular_bg_items::jam.create_bg()) {
	_jam_bg.set_visible(false);
	bn::bg_palettes::set_transparent_color(bn::color(7, 5, 11));

	bn::sprite_tiles_item tiles = bn::sprite_items::nostabyte.tiles_item();

	_nos_sprite.set_position(bn::fixed_point(-58, 0));
	_nos_sprite.set_visible(false);
	_nos_sprite.set_blending_enabled(true);

	_ta_sprite.set_tiles(tiles.create_tiles(1));
	_ta_sprite.set_position(bn::fixed_point(4, 0));
	_ta_sprite.set_visible(false);
	_ta_sprite.set_blending_enabled(true);

	_byt_sprite.set_tiles(tiles.create_tiles(2));
	_byt_sprite.set_position(bn::fixed_point(44, 0));
	_byt_sprite.set_visible(false);
	_byt_sprite.set_blending_enabled(true);

	_e_sprite.set_tiles(tiles.create_tiles(3));
	_e_sprite.set_position(bn::fixed_point(44 + 64, 0));
	_e_sprite.set_visible(false);
	_e_sprite.set_blending_enabled(true);

	_interac_sprite.set_tiles(tiles.create_tiles(4));
	_interac_sprite.set_visible(false);
	_interac_sprite.set_blending_enabled(true);

	_tive_sprite.set_tiles(tiles.create_tiles(5));
	_tive_sprite.set_visible(false);
	_tive_sprite.set_blending_enabled(true);
}

splash_scene::~splash_scene() {
}

void splash_scene::_animate_up(bn::sprite_ptr sprite, int end_frame, int duration_frames) {
	if (_frame > end_frame) {
		return;
	}
	sprite.set_visible(_frame >= (end_frame - duration_frames));
	bn::fixed_point p = sprite.position();
	bn::fixed y = bn::max(end_frame - _frame, 0);
	y *= y;
	p.set_y(y);
	sprite.set_position(p);
}

void splash_scene::_animate_right(bn::sprite_ptr sprite, int end_frame, int duration_frames, int end_x) {
	if (_frame > end_frame) {
		return;
	}
	sprite.set_visible(_frame >= (end_frame - duration_frames));
	bn::fixed_point p = sprite.position();
	p.set_x(bn::min(end_x - (end_frame - _frame), end_x));
	sprite.set_position(p);
}

bn::optional<scene_type> splash_scene::update() {
	bn::optional<scene_type> result;

	if (_frame >= 200 && _frame <= 275) {
		bn::fixed a = helpers::remap_fixed(_frame, 240, 275, 1, 0);
		bn::blending::set_transparency_alpha(a);
		bn::bg_palettes::set_transparent_color(helpers::lerp_color(bn::color(0, 0, 0), bn::color(7, 5, 11), a));
	} else if (_frame <= 80) {
		bn::fixed a = helpers::remap_fixed(_frame, 0, 80, 0, 1);
		bn::bg_palettes::set_transparent_color(helpers::lerp_color(bn::color(31, 31, 31), bn::color(7, 5, 11), a));
	}

	if (_frame == 300) {
		_nos_sprite.set_blending_enabled(false);
		_nos_sprite.set_visible(false);
		_ta_sprite.set_blending_enabled(false);
		_ta_sprite.set_visible(false);
		_byt_sprite.set_blending_enabled(false);
		_byt_sprite.set_visible(false);
		_e_sprite.set_blending_enabled(false);
		_e_sprite.set_visible(false);
		_interac_sprite.set_blending_enabled(false);
		_interac_sprite.set_visible(false);
		_tive_sprite.set_blending_enabled(false);
		_tive_sprite.set_visible(false);

		bn::bg_palettes::set_transparent_color(bn::color(0, 0, 0));
		_jam_bg.set_blending_enabled(true);
		_jam_bg.set_visible(true);
	}

	if (_frame >= 300 && _frame < 320) {
		bn::fixed a = helpers::remap_fixed(_frame, 300, 320, 0, 1);
		bn::blending::set_transparency_alpha(a);
	}
	if (_frame >= 480) {
		bn::fixed a = helpers::remap_fixed(_frame, 480, 500, 1, 0);
		bn::blending::set_transparency_alpha(a);
	}

	if (bn::keypad::any_released() || _frame == 530) {
		result = scene_type::MAIN_MENU;
		_splash_sound.stop();
		_jam_bg.set_visible(false);

		bn::blending::set_transparency_alpha(1.0);
		bn::blending::set_fade_alpha(0.0);

		return result;
	}

	_animate_up(_nos_sprite, 51 * 2, 3);
	_animate_up(_ta_sprite, 60 * 2, 3);
	_animate_up(_byt_sprite, 68 * 2, 3);
	_animate_up(_e_sprite, 68 * 2, 3);

	_animate_right(_interac_sprite, 78 * 2, 4, 9);
	_animate_right(_tive_sprite, 78 * 2, 4, 9 + 64);

	_frame += 1;

	return result;
}

} //namespace game