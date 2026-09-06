#include "scenes/ship_scene.h"

#include "fonts/common_fixed_8x8_sprite_font.h"

#include "bn_direct_bitmap_items_ship_interior.h"
#include "bn_sprite_items_dev16.h"

#include "helpers.h"
#include "upgrades.h"

namespace game {

ship_scene::ship_scene(shared_state &p_shared) :
		scene(p_shared),
		_camera(bn::camera_ptr::create(0, 0)),
		_small_text(common::fixed_8x8_sprite_font),
		_pano_bg(bn::sp_direct_bitmap_bg_ptr::create()) {
	bn::bg_palettes::set_transparent_color(bn::color(0, 1, 0));

	_shared.ensure_loaded();
}

ship_scene::~ship_scene() {
	_text_sprites.clear();
	_pano_bg.set_visible(false);
}

inline int _get_menu_rotation(ship_menu p_menu) {
	switch (p_menu) {
		case ship_menu::BOUNTIES:
			return 390;
		case ship_menu::INVENTORY:
			return 390;
		case ship_menu::UPGRADE:
			return 390;
		case ship_menu::DEPLOY:
			return 800;
		default:
			return 0;
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
			stream.append("DEPLOY");
			break;
		default:
			break;
	}
}

bn::optional<scene_type> ship_scene::update() {
	bn::optional<scene_type> result;

	_pano_bg.set_visible(!_viewing_menu.has_value());

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
		if (bn::keypad::a_pressed()) {
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
		_rotation = helpers::lerp_fixed(_rotation, target_rotation, 0.5);
		_rotation = helpers::fposmod(_rotation, 1024);

		_camera.set_position(_rotation, 0);

		bn::sp_direct_bitmap_bg_painter painter(_pano_bg);
		painter.blit(-_rotation.integer(), 0, bn::direct_bitmap_items::ship_interior);

		_text_sprites.clear();

		bn::string<40> text;
		bn::ostringstream text_stream(text);

		text.clear();
		_small_text.set_alignment(bn::sprite_text_generator::alignment_type::CENTER);
		_append_ship_menu_name(text_stream, _selected_ship_menu);
		_small_text.generate(0, -20, text, _text_sprites);
	}

	return result;
}

void ship_scene::_update_bounties_screen() {
	if (bn::keypad::b_released()) {
		_viewing_menu.reset();
		_selected_bounty_index = 0;
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
		text.clear();

		if (_selected_bounty_index == i) {
			text_stream.append("> ");
		} else {
			text_stream.append("  ");
		}

		if (b.collected) {
			text_stream.append("[COLLECTED]");
		} else {
			helpers::append_with_padding(text_stream, b.amount, 3, ' ');
			text_stream.append(" ");
			text_stream.append(get_item_info(b.resource).display_name);
			text_stream.append("  $");
			helpers::append_with_padding(text_stream, b.price, 3, ' ');
		}

		_small_text.generate(-80, -40 + i * 9, text, _text_sprites);
	}
}

void ship_scene::_update_inventory_screen() {
	if (bn::keypad::b_released()) {
		_viewing_menu.reset();
		return;
	}

	_text_sprites.clear();
	_small_text.set_alignment(bn::sprite_text_generator::alignment_type::CENTER);
	_small_text.generate(0, -72, "INVENTORY", _text_sprites);

	bn::string<40> text;
	bn::ostringstream text_stream(text);

	_small_text.set_alignment(bn::sprite_text_generator::alignment_type::LEFT);

	for (int i = 0; i < ITEM_TYPE_COUNT; i++) {
		auto typ = static_cast<item_type>(i);

		text.clear();
		helpers::append_with_padding(text_stream, _shared.get_inventory_count(typ), 3, ' ');
		text_stream.append(" ");
		text_stream.append(get_item_info(typ).display_name);

		_small_text.generate(-80, -40 + i * 9, text, _text_sprites);
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

	bn::string<40> text;
	bn::ostringstream text_stream(text);

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
			_small_text.generate(30 + (MAX_UPGRADE_LEVEL + 2) * 4, y, "MAX", _text_sprites);
		}
	}

	if (bn::keypad::up_released() && _selected_upgrade_index > 0) {
		_selected_upgrade_index--;
	} else if (bn::keypad::down_released() && _selected_upgrade_index < UPGRADE_COUNT) {
		_selected_upgrade_index++;
	}

	auto selected_buy = _selected_upgrade_index == UPGRADE_COUNT;

	text.clear();
	text_stream.append(_shared.get_remaining_upgrade_slot_count());
	text_stream.append(" SLOTS LEFT");
	_small_text.generate(-80, 30, text, _text_sprites);

	text.clear();
	text_stream.append("BUY UPGRADE SLOT $");
	text_stream.append(UPGRADE_PRICE);
	_small_text.generate(-80, 40, text, _text_sprites);

	if (selected_buy) {
		_small_text.generate(-90, 40, ">", _text_sprites);

		if (bn::keypad::a_released()) {
			_shared.buy_upgrade_slot();
		}
	} else {
		_small_text.generate(-90, -40 + _selected_upgrade_index * 10, ">", _text_sprites);

		auto selected_upgrade = static_cast<upgrade_type>(_selected_upgrade_index);

		if (bn::keypad::right_released()) {
			_shared.upgrade_module(selected_upgrade);
		} else if (bn::keypad::left_released()) {
			_shared.downgrade_module(selected_upgrade);
		}
	}
}

} // namespace game
