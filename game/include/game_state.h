#pragma once

#include "obj_type.h"

#include "bn_array.h"
#include "bn_bg_palettes.h"
#include "bn_bg_tiles.h"
#include "bn_common.h"
#include "bn_list.h"
#include "bn_memory.h"
#include "bn_regular_bg_item.h"
#include "bn_regular_bg_map_ptr.h"
#include "bn_regular_bg_ptr.h"
#include "bn_sprite_ptr.h"
#include "bn_sprite_text_generator.h"
#include "bn_vector.h"

namespace game {

class game_state {
public:
	bn::sprite_text_generator small_fixed_text_generator;
	bn::sprite_text_generator small_variable_text_generator;

	int item_inventory[ITEM_TYPE_COUNT];

	game_state();
	void update();

	void clear_inventory();
	void pickup_resource(obj_type p_type, int amount);

	struct item_queue_entry {
		obj_type object_type;
		int amount;
		int frame;
	};

	static constexpr int ITEM_QUEUE_FRAMES_TIME = 90;

private:
	int _item_queue_frame;

public:
	bn::list<item_queue_entry, 5> item_pickup_queue;
	int item_queue_frame() { return _item_queue_frame; }
};
} //namespace game