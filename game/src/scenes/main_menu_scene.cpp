#include "scenes/main_menu_scene.h"

#include "helpers.h"
#include "scenes/scene_type.h"

#include "bn_bg_palettes.h"
#include "bn_blending.h"
#include "bn_keypad.h"

#include "bn_regular_bg_items_jam.h"
#include "bn_sound_items.h"
#include "bn_sprite_items_nostabyte.h"
#include "fonts/common_fixed_8x8_sprite_font.h"
#include "fonts/common_variable_16x16_sprite_font.h"

#include "bn_music_items.h"

namespace game {

main_menu_scene::main_menu_scene(shared_state &p_shared) :
		scene(p_shared),
		_big_text(common::variable_16x16_sprite_font),
		_small_text(common::fixed_8x8_sprite_font) {
	bn::bg_palettes::set_transparent_color(bn::color(7, 5, 11));

	bn::music_items::title_loop.play(0.5, true);
}

main_menu_scene::~main_menu_scene() {
}

bn::optional<scene_type> main_menu_scene::update() {
	bn::optional<scene_type> result;

	_text_sprites.clear();

	_big_text.set_alignment(bn::sprite_text_generator::alignment_type::CENTER);
	_big_text.generate(0, -30, "xo89", _text_sprites);

	_small_text.set_alignment(bn::sprite_text_generator::alignment_type::LEFT);
	_small_text.generate(-80, 25, "NEW GAME", _text_sprites);
	_small_text.generate(-80, 15, "LOAD GAME", _text_sprites);
	_small_text.generate(-80, 35, "CREDITS", _text_sprites);

	_small_text.generate(-88, 15, ">", _text_sprites);

	// _frame += 1;

	return result;
}

} //namespace game