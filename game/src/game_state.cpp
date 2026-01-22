#include "game_state.h"

#include "common_fixed_8x8_sprite_font.h"
#include "common_variable_8x8_sprite_font.h"

namespace game {

game_state::game_state() :
		small_fixed_text_generator(common::fixed_8x8_sprite_font),
		small_variable_text_generator(common::variable_8x8_sprite_font) {
}

void game_state::update() {
}

} //namespace game