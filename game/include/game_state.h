#ifndef GAME_STATE_H
#define GAME_STATE_H

#include "chunked_space.h"

#include "bn_array.h"
#include "bn_bg_palettes.h"
#include "bn_bg_tiles.h"
#include "bn_common.h"
#include "bn_memory.h"
#include "bn_regular_bg_item.h"
#include "bn_regular_bg_map_ptr.h"
#include "bn_regular_bg_ptr.h"
#include "bn_sprite_ptr.h"
#include "bn_sprite_text_generator.h"
#include "bn_vector.h"

namespace game {

// this is the state shared between rendered scenes.
class game_state {
public:
	bn::sprite_text_generator small_fixed_text_generator;
	bn::sprite_text_generator small_variable_text_generator;

	// todo maybe this is where we save/load?

	game_state();
	void update();
};
} //namespace game
#endif