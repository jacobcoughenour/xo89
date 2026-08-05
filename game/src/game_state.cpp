#include "game_state.h"

#include "common_fixed_8x8_sprite_font.h"
#include "common_variable_8x8_sprite_font.h"

namespace game {

game_state::game_state() :
		small_fixed_text_generator(common::fixed_8x8_sprite_font),
		small_variable_text_generator(common::variable_8x8_sprite_font) {
	clear_inventory();
}

void game_state::update() {
	for (auto it = item_pickup_queue.begin(); it != item_pickup_queue.end(); ++it) {
		auto &obj = *it;
		if (obj.frame < _item_queue_frame) {
			item_pickup_queue.erase(it);
		}
	}

	if (item_pickup_queue.size() > 0) {
		_item_queue_frame++;
	} else {
		_item_queue_frame = 0;
	}
}

void game_state::clear_inventory() {
	for (size_t i = 0; i < static_cast<unsigned long>(obj_type::OBJ_TYPE_MAX); i++) {
		item_inventory[i] = 0;
	}
}

void game_state::pickup_resource(obj_type p_type, int p_amount) {
	item_inventory[static_cast<unsigned long>(p_type)] += p_amount;

	int frame = _item_queue_frame + ITEM_QUEUE_FRAMES_TIME;

	for (auto it = item_pickup_queue.begin(); it != item_pickup_queue.end(); ++it) {
		auto &obj = *it;
		if (obj.object_type == p_type) {
			obj.amount += p_amount;
			obj.frame = frame;
			return;
		}
	}

	if (item_pickup_queue.size() >= item_pickup_queue.max_size()) {
		item_pickup_queue.pop_front();
	}
	item_pickup_queue.push_back({
			p_type,
			p_amount,
			frame,
	});
}

} //namespace game