#include "scenes/main_menu_scene.h"

#include "helpers.h"
#include "scenes/scene_type.h"

#include "bn_bg_palettes.h"
#include "bn_blending.h"
#include "bn_keypad.h"

#include "bn_regular_bg_items_logo.h"
#include "bn_regular_bg_items_main_menu_bg.h"
#include "bn_sound_items.h"
#include "bn_sprite_items_nostabyte.h"
#include "fonts/common_fixed_8x8_sprite_font.h"
#include "fonts/common_variable_8x8_sprite_font.h"

#include "bn_sprite_items_main_menu_bg_ship.h"

#include "bn_music.h"
#include "bn_music_items.h"

namespace game {

main_menu_scene::main_menu_scene(shared_state &p_shared) :
		scene(p_shared),
		_main_bg(bn::regular_bg_items::main_menu_bg.create_bg(0, 30)),
		_logo_bg(bn::regular_bg_items::logo.create_bg()),
		_small_text(common::fixed_8x8_sprite_font),
		_small_var_text(common::variable_8x8_sprite_font),
		_ship_sprite(bn::sprite_items::main_menu_bg_ship.create_sprite(56, 18 + 45)) {
	bn::bg_palettes::set_transparent_color(bn::color(1, 0, 1));
	bn::music_items::title_loop.play(0.5, true);
	_logo_bg.set_visible(false);
	_main_bg.set_visible(false);
	_ship_sprite.set_visible(false);
	bn::blending::set_transparency_alpha(1.0);
	bn::blending::set_fade_alpha(1.0);
}

main_menu_scene::~main_menu_scene() {
}

bn::optional<scene_type> main_menu_scene::update() {
	bn::optional<scene_type> result;

	_text_sprites.clear();

	_frame++;

	if (_frame < 240) {
		if (bn::keypad::any_released()) {
			_frame = 240;
			return result;
		}

		_main_bg.set_y(helpers::lerp_fixed(_main_bg.y(), 0.0, 0.023));
		_ship_sprite.set_y(helpers::lerp_fixed(_ship_sprite.y(), 18, 0.02));

		if (_frame < 160) {
			_logo_bg.set_visible(false);
			_main_bg.set_blending_enabled(true);
			_ship_sprite.set_blending_enabled(true);
			bn::blending::set_fade_alpha(helpers::remap_fixed(_frame, 0, 120, 1.0, 0.0));
			_main_bg.set_visible(true);
			_ship_sprite.set_visible(true);
		} else {
			_logo_bg.set_visible(true);
			_main_bg.set_blending_enabled(false);
			_ship_sprite.set_blending_enabled(false);
			_logo_bg.set_blending_enabled(true);
			bn::blending::set_transparency_alpha(helpers::remap_fixed(_frame, 160, 200, 0.0, 1.0));
		}

		return result;
	}

	bn::blending::set_transparency_alpha(1.0);
	bn::blending::set_fade_alpha(0.0);
	_main_bg.set_visible(true);
	_ship_sprite.set_visible(true);
	_main_bg.set_blending_enabled(false);
	_ship_sprite.set_blending_enabled(false);
	_logo_bg.set_blending_enabled(false);

	_main_bg.set_y(0);
	_ship_sprite.set_y(18);

	_logo_bg.set_visible(!_option_selected);

	if (_option_selected) {
		if (bn::keypad::b_released()) {
			_shared.play_select();
			_option_selected = false;
		}

		_small_text.set_alignment(bn::sprite_text_generator::alignment_type::CENTER);

		if (_selected_menu == main_menu_option::NEW_GAME) {
			if (_shared.has_save()) {
				_small_text.generate(0, -10, "OVERWRITE EXISTING", _text_sprites);
				_small_text.generate(0, 0, "SAVE DATA?", _text_sprites);
				_small_text.generate(0, 15, "[B] NO  [A] YES", _text_sprites);
			}
			if (!_shared.has_save() || bn::keypad::a_released()) {
				_shared.play_select();
				_shared.new_game();
				result = scene_type::SHIP;
				bn::music::stop();
				return result;
			}
		} else if (_selected_menu == main_menu_option::LOAD_GAME) {
			if (!_shared.has_save()) {
				_small_text.generate(0, -5, "NO SAVE AVAILABLE", _text_sprites);
			} else {
				_shared.load();
				result = scene_type::SHIP;
				bn::music::stop();

				return result;
			}
		} else if (_selected_menu == main_menu_option::CREDITS) {
			_small_text.set_alignment(bn::sprite_text_generator::alignment_type::CENTER);
			_small_var_text.set_alignment(bn::sprite_text_generator::alignment_type::CENTER);

			_small_text.generate(0, -32, "CREDITS", _text_sprites);

			constexpr bn::string_view credits_text_lines[] = {
				"3D assets by PIZZA DOGGY",
				"Everything else by Jacob Coughenour,",
				"Nostabyte Interactive, 12th Sep 2026.",
				"Made with Butano Engine v21.7.1",
			};

			for (int i = 0; i < 4; i++) {
				_small_var_text.generate(0, i * 10, credits_text_lines[i], _text_sprites);
			}
		}

	} else {
		int option = static_cast<int>(_selected_menu);
		if (bn::keypad::up_released() && option > 0) {
			option--;
			_shared.play_click();
		} else if (bn::keypad::down_released() && option < 2) {
			option++;
			_shared.play_click();
		}
		_selected_menu = static_cast<main_menu_option>(option);

		_small_text.set_alignment(bn::sprite_text_generator::alignment_type::LEFT);
		_small_text.generate(-96, 5, "NEW GAME", _text_sprites);
		_small_text.generate(-96, 15, "LOAD GAME", _text_sprites);
		_small_text.generate(-96, 25, "CREDITS", _text_sprites);

		_small_text.generate(-96 - 10, 5 + option * 10, ">", _text_sprites);

		_small_text.set_alignment(bn::sprite_text_generator::alignment_type::RIGHT);
		_small_text.generate(120, 76, "v1.0 JAM EDITION", _text_sprites);

		if (bn::keypad::a_released()) {
			_shared.play_select();
			_option_selected = true;
		}
	}

	return result;
}

} //namespace game