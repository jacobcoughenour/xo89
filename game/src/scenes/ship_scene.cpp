#include "scenes/ship_scene.h"

#include "fonts/common_fixed_8x8_sprite_font.h"

#include "bn_direct_bitmap_items_ship_interior.h"
#include "bn_sprite_items_dev16.h"

#include "helpers.h"

namespace game {

ship_scene::ship_scene(shared_state &p_shared) :
		scene(p_shared),
		_camera(bn::camera_ptr::create(0, 0)),
		_small_text(common::fixed_8x8_sprite_font),
		_pano_bg(bn::sp_direct_bitmap_bg_ptr::create()),
		_test_sprite(bn::sprite_items::dev16.create_sprite()) {
	bn::bg_palettes::set_transparent_color(bn::color(16, 16, 16));

	_test_sprite.set_camera(_camera);
	_test_sprite.set_position(1024 / 2, 4);
}

ship_scene::~ship_scene() {
}

bn::optional<scene_type> ship_scene::update() {
	bn::optional<scene_type> result;

	if (bn::keypad::start_released()) {
		result = scene_type::MINING;
		return result;
	}

	if (bn::keypad::right_held()) {
		_rotation += 8;
	}
	if (bn::keypad::left_held()) {
		_rotation -= 8;
	}

	_rotation += 2;

	_rotation = helpers::posmod(_rotation, 1024);

	_camera.set_position(_rotation, 0);

	bn::sp_direct_bitmap_bg_painter painter(_pano_bg);
	painter.blit(-_rotation, 0, bn::direct_bitmap_items::ship_interior);

	return result;
}

} // namespace game
