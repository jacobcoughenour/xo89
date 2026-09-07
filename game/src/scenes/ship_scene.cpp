#include "scenes/ship_scene.h"

#include "fonts/common_fixed_8x8_sprite_font.h"

#include "bn_direct_bitmap_items_ship_interior.h"
#include "bn_regular_bg_items_screen_bg.h"
#include "bn_sprite_items_dropped_items.h"
#include "bn_sprite_items_screen_menu_overlay.h"

#include "helpers.h"
#include "upgrades.h"

namespace game {

ship_scene::ship_scene(shared_state &p_shared) :
		scene(p_shared),
		_camera(bn::camera_ptr::create(0, 0)),
		_small_text(common::fixed_8x8_sprite_font) {
	bn::bg_palettes::set_transparent_color(bn::color(0, 1, 0));

	_shared.ensure_loaded();
}

ship_scene::~ship_scene() {
	_text_sprites.clear();
	_pano_bg.reset();
	_screen_bg.reset();
}

inline int _get_menu_rotation(ship_menu p_menu) {
	switch (p_menu) {
		case ship_menu::DEPLOY:
			return 240;
		default:
			return 68;
	}
}

inline void _append_ship_menu_name(bn::ostringstream &stream, ship_menu p_menu) {
	switch (p_menu) {
		case ship_menu::BOUNTIES:
			stream.append("BOUNTIES");
			break;
		case ship_menu::INVENTORY:
			stream.append("INVENTORY");
			break;
		case ship_menu::UPGRADE:
			stream.append("MODULES");
			break;
		case ship_menu::DEPLOY:
			stream.append("DEPLOY DRONE");
			break;
		default:
			break;
	}
}

bn::optional<scene_type> ship_scene::update() {
	bn::optional<scene_type> result;

	if (_viewing_menu.has_value()) {
		if (_pano_bg.has_value()) {
			_pano_bg.reset();
			bn::core::update();
		}
		if (!_screen_bg.has_value()) {
			_screen_bg = bn::regular_bg_items::screen_bg.create_bg();
		}
	} else {
		if (_screen_bg.has_value()) {
			_screen_bg.reset();
			bn::core::update();
		}
		if (!_pano_bg.has_value()) {
			_pano_bg = bn::sp_direct_bitmap_bg_ptr::create();
		}
	}

	if (_viewing_menu.has_value() && _screen_overlay.has_value()) {
		_screen_overlay.reset();
	}

	if (_viewing_menu.has_value()) {
		if (_viewing_menu == ship_menu::BOUNTIES) {
			_update_bounties_screen();
		} else if (_viewing_menu == ship_menu::INVENTORY) {
			_update_inventory_screen();
		} else if (_viewing_menu == ship_menu::UPGRADE) {
			_update_upgrades_screen();
		} else {
			result = scene_type::MINING;
		}
		return result;
	} else {
		if (bn::keypad::a_released()) {
			_viewing_menu = _selected_ship_menu;
			return result;
		}

		int menu_index = static_cast<int>(_selected_ship_menu);
		if (bn::keypad::right_released()) {
			menu_index++;
		}
		if (bn::keypad::left_released()) {
			menu_index--;
		}
		menu_index = helpers::posmod(menu_index, 4);
		_selected_ship_menu = static_cast<ship_menu>(menu_index);

		int target_rotation = _get_menu_rotation(_selected_ship_menu);
		_rotation = helpers::lerp_fixed(_rotation, target_rotation, 0.4);
		_rotation = helpers::fposmod(_rotation, 512);

		_camera.set_position(_rotation, 0);

		bn::sp_direct_bitmap_bg_painter painter(_pano_bg.value());
		painter.blit(-_rotation.integer(), 0, bn::direct_bitmap_items::ship_interior);

		if (_selected_ship_menu != ship_menu::DEPLOY) {
			_screen_overlay = bn::sprite_items::screen_menu_overlay.create_sprite(30, 14, _selected_ship_menu);
			_screen_overlay->set_camera(_camera);
		}

		_text_sprites.clear();

		if (_rotation == target_rotation) {
			bn::string<40> text;
			bn::ostringstream text_stream(text);

			text.clear();
			_small_text.set_alignment(bn::sprite_text_generator::alignment_type::CENTER);
			_append_ship_menu_name(text_stream, _selected_ship_menu);
			if (_selected_ship_menu == ship_menu::DEPLOY) {
				_small_text.generate(38, 0, text, _text_sprites);
			} else {
				_small_text.generate(-40, -22, text, _text_sprites);
			}
		}
	}

	return result;
}

void ship_scene::_update_bounties_screen() {
	if (bn::keypad::b_released()) {
		_viewing_menu.reset();
		_selected_bounty_index = 0;
		_shared.clear_collected_bounties();
		_item_sprites.clear();
		return;
	}

	auto bounties = _shared.get_bounties();

	if (bn::keypad::up_released()) {
		_selected_bounty_index = bn::max(0, _selected_bounty_index - 1);
	} else if (bn::keypad::down_released()) {
		_selected_bounty_index = bn::min(_selected_bounty_index + 1, bounties.size());
	}

	if (bn::keypad::a_released()) {
		_shared.collect_bounty(_selected_bounty_index);
	}

	_item_sprites.clear();

	_text_sprites.clear();
	bn::string<40> text;
	bn::ostringstream text_stream(text);

	_small_text.set_alignment(bn::sprite_text_generator::alignment_type::RIGHT);
	text_stream.append("$");
	helpers::append_with_padding(text_stream, _shared.get_balance(), 1, '0');
	_small_text.generate(100, -64, text, _text_sprites);

	_small_text.set_alignment(bn::sprite_text_generator::alignment_type::CENTER);
	_small_text.generate(0, -72, "BOUNTIES", _text_sprites);

	_small_text.set_alignment(bn::sprite_text_generator::alignment_type::LEFT);

	for (int i = 0; i < bounties.size(); i++) {
		auto b = bounties[i];

		auto y = -32 + i * 10;

		if (_selected_bounty_index == i) {
			_small_text.generate(-90, y, ">", _text_sprites);
		}

		if (b.collected) {
			_small_text.generate(-60, y, "[COLLECTED]", _text_sprites);
		} else {
			text.clear();
			helpers::append_with_padding(text_stream, b.amount, 3, ' ');
			_small_text.generate(-76, y, text, _text_sprites);

			auto item = get_item_info(b.resource);
			auto sprite = bn::sprite_items::dropped_items.create_sprite(-40, y);
			sprite.set_tiles(bn::sprite_items::dropped_items.tiles_item()
							.create_tiles(item.sprite_index + _shared.get_frame_count() / 30 % 2));
			_item_sprites.push_back(sprite);

			_small_text.generate(-34, y, item.display_name, _text_sprites);
			text.clear();
			text_stream.append("$");
			helpers::append_with_padding(text_stream, b.price, 3, ' ');
			_small_text.generate(44, y, text, _text_sprites);
		}
	}
}

void ship_scene::_update_inventory_screen() {
	if (bn::keypad::b_released()) {
		_viewing_menu.reset();
		_item_sprites.clear();
		return;
	}

	_item_sprites.clear();
	_text_sprites.clear();
	_small_text.set_alignment(bn::sprite_text_generator::alignment_type::CENTER);
	_small_text.generate(0, -72, "INVENTORY", _text_sprites);

	bn::string<40> text;
	bn::ostringstream text_stream(text);

	for (int i = 0; i < ITEM_TYPE_COUNT; i++) {
		auto typ = static_cast<item_type>(i);
		int y = -32 + i * 10;

		text.clear();
		helpers::append_with_padding(text_stream, _shared.get_inventory_count(typ), 3, ' ');
		_small_text.set_alignment(bn::sprite_text_generator::alignment_type::RIGHT);
		_small_text.generate(-38, y, text, _text_sprites);

		auto item = get_item_info(typ);
		auto sprite = bn::sprite_items::dropped_items.create_sprite(-30, y);
		sprite.set_tiles(bn::sprite_items::dropped_items.tiles_item()
						.create_tiles(item.sprite_index + _shared.get_frame_count() / 30 % 2));
		_item_sprites.push_back(sprite);

		_small_text.set_alignment(bn::sprite_text_generator::alignment_type::LEFT);
		_small_text.generate(-24, y, item.display_name, _text_sprites);
	}
}

void ship_scene::_update_upgrades_screen() {
	if (bn::keypad::b_released()) {
		_viewing_menu.reset();
		return;
	}
	_text_sprites.clear();
	_small_text.set_alignment(bn::sprite_text_generator::alignment_type::CENTER);
	_small_text.generate(0, -72, "MODULES", _text_sprites);

	bn::string<64> text;
	bn::ostringstream text_stream(text);

	_small_text.set_alignment(bn::sprite_text_generator::alignment_type::RIGHT);
	text_stream.append("$");
	helpers::append_with_padding(text_stream, _shared.get_balance(), 1, '0');
	_small_text.generate(100, -64, text, _text_sprites);

	_small_text.set_alignment(bn::sprite_text_generator::alignment_type::LEFT);

	for (int i = 0; i < UPGRADE_COUNT; i++) {
		auto info = upgrades[i];
		auto y = bn::fixed(-40 + i * 10);

		text.clear();
		text_stream.append(info.display_name);

		_small_text.generate(-80, y, text, _text_sprites);

		_small_text.generate(28 - 4, y, "[", _text_sprites);
		_small_text.generate(30 + MAX_UPGRADE_LEVEL * 4, y, "]", _text_sprites);
		auto level = _shared.get_upgrade_level(static_cast<upgrade_type>(i));
		for (unsigned int x = 0; x < level; x++) {
			_small_text.generate(29 + x * 4, y, "|", _text_sprites);
		}
		if (level == 3) {
			_small_text.generate(34 + (MAX_UPGRADE_LEVEL + 2) * 4, y, "MAX", _text_sprites);
		}
	}

	auto can_buy_more_slots = _shared.get_total_upgrade_slot_count() < MAX_UPGRADE_LEVEL * UPGRADE_COUNT;

	if (bn::keypad::up_released() && _selected_upgrade_index > 0) {
		_selected_upgrade_index--;
	} else if (bn::keypad::down_released() && _selected_upgrade_index < UPGRADE_COUNT + 1) {
		_selected_upgrade_index++;
	}

	auto selected_buy = _selected_upgrade_index == UPGRADE_COUNT;
	auto selected_rocket = _selected_upgrade_index == (UPGRADE_COUNT + 1);

	_small_text.set_alignment(bn::sprite_text_generator::alignment_type::RIGHT);
	text.clear();
	text_stream.append(_shared.get_remaining_upgrade_slot_count());
	text_stream.append(" SLOTS LEFT");
	_small_text.generate(80, 20, text, _text_sprites);

	_small_text.set_alignment(bn::sprite_text_generator::alignment_type::LEFT);
	if (can_buy_more_slots) {
		text.clear();
		text_stream.append("BUY UPGRADE SLOT $");
		text_stream.append(UPGRADE_PRICE);
		_small_text.generate(-80, 30, text, _text_sprites);
	} else {
		_small_text.generate(-80, 30, "---", _text_sprites);
	}

	text.clear();
	_small_text.set_alignment(bn::sprite_text_generator::alignment_type::LEFT);
	if (_shared.get_has_rocket_launcher()) {
		_small_text.generate(-80, 50, "ROCKET LAUNCHER [|]", _text_sprites);
	} else {
		text_stream.append("ROCKET LAUNCHER $");
		text_stream.append(ROCKET_LAUNCHER_PRICE);
		_small_text.generate(-80, 50, text, _text_sprites);
	}

	if (selected_buy) {
		_small_text.generate(-96, 30, ">", _text_sprites);

		if (bn::keypad::a_released()) {
			_shared.buy_upgrade_slot();
		}
	} else if (selected_rocket) {
		_small_text.generate(-96, 50, ">", _text_sprites);

		if (bn::keypad::a_released()) {
			_shared.buy_rocket_launcher();
		}
	} else {
		_small_text.generate(-96, -40 + _selected_upgrade_index * 10, ">", _text_sprites);

		auto selected_upgrade = static_cast<upgrade_type>(_selected_upgrade_index);

		if (bn::keypad::right_released()) {
			_shared.upgrade_module(selected_upgrade);
		} else if (bn::keypad::left_released()) {
			_shared.downgrade_module(selected_upgrade);
		}
	}
}

} // namespace game
