#include "scenes/ship_scene.h"

#include "fonts/common_fixed_8x8_sprite_font.h"

#include "bn_direct_bitmap_items_ship_interior.h"
#include "bn_regular_bg_items_screen_bg.h"
#include "bn_sound_items.h"
#include "bn_sprite_items_buttons.h"
#include "bn_sprite_items_combat.h"
#include "bn_sprite_items_dpad.h"
#include "bn_sprite_items_dropped_items.h"
#include "bn_sprite_items_mining.h"
#include "bn_sprite_items_scanner.h"
#include "bn_sprite_items_scanner_exit.h"
#include "bn_sprite_items_screen_menu_overlay.h"
#include "bn_sprite_items_ship_green.h"

#include "helpers.h"
#include "upgrades.h"

namespace game {

ship_scene::ship_scene(shared_state &p_shared) :
		scene(p_shared),
		_camera(bn::camera_ptr::create(0, 0)),
		_small_text(common::fixed_8x8_sprite_font) {
	bn::bg_palettes::set_transparent_color(bn::color(0, 1, 0));

	_shared.ensure_loaded();
	_viewing_menu.reset();
	_tutorial_page_index = 0;
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
		case ship_menu::TUTORIAL:
			stream.append("TUTORIAL");
			break;
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

	bn::music::stop();

	if (_hum_timer == 0) {
		_hum_sound = bn::sound_items::ship_hum.play(0.3);
		_hum_timer = 34;
	}
	_hum_timer--;

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
		if (_viewing_menu == ship_menu::TUTORIAL) {
			_update_tutorial_screen();
		} else if (_viewing_menu == ship_menu::BOUNTIES) {
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
			_shared.play_load();
			return result;
		}

		int menu_index = static_cast<int>(_selected_ship_menu);
		if (bn::keypad::right_released() || bn::keypad::r_released()) {
			menu_index++;
			if (menu_index < static_cast<int>(ship_menu::DEPLOY)) {
				_shared.play_click();
			} else {
				_shared.play_whoosh();
			}
		}
		if (bn::keypad::left_released() || bn::keypad::l_released()) {
			menu_index--;
			if (menu_index >= 0 && menu_index < static_cast<int>(ship_menu::DEPLOY) - 1) {
				_shared.play_click();
			} else {
				_shared.play_whoosh();
			}
		}
		menu_index = helpers::posmod(menu_index, 5);
		_selected_ship_menu = static_cast<ship_menu>(menu_index);

		bn::sp_direct_bitmap_bg_painter painter(_pano_bg.value());
		painter.blit(-_rotation.integer(), 0, bn::direct_bitmap_items::ship_interior);

		int target_rotation = _get_menu_rotation(_selected_ship_menu);
		_rotation = helpers::lerp_fixed(_rotation, target_rotation, 0.4);
		_rotation = helpers::fposmod(_rotation, 512);

		_camera.set_position(_rotation, 0);

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
				_small_text.generate(45, 32, text, _text_sprites);
			} else {
				_small_text.generate(-40, -22, text, _text_sprites);
			}
		}
	}

	return result;
}

void ship_scene::_update_tutorial_screen() {
	if (bn::keypad::b_released()) {
		_tutorial_sprites.clear();
		_viewing_menu.reset();
		_shared.play_whoosh();
		return;
	}

	constexpr int tutorial_pages = 7;

	if (bn::keypad::right_released() || bn::keypad::r_released()) {
		_tutorial_page_index = bn::min(_tutorial_page_index + 1, tutorial_pages - 1);
		_shared.play_click();
	}
	if (bn::keypad::left_released() || bn::keypad::l_released()) {
		_tutorial_page_index = bn::max(_tutorial_page_index - 1, 0);
		_shared.play_click();
	}

	_tutorial_sprites.clear();
	_text_sprites.clear();

	bn::string<60> text;
	bn::ostringstream text_stream(text);

	text.clear();
	if (_tutorial_page_index > 0) {
		_small_text.set_alignment(bn::sprite_text_generator::alignment_type::LEFT);
		text.append("L<");
		_small_text.generate(-100, -72, text, _text_sprites);
	}
	if (_tutorial_page_index < tutorial_pages - 1) {
		text.clear();
		_small_text.set_alignment(bn::sprite_text_generator::alignment_type::RIGHT);
		text.append(">R");
		_small_text.generate(100, -72, text, _text_sprites);
	}

	_small_text.set_alignment(bn::sprite_text_generator::alignment_type::CENTER);
	text.clear();
	for (int i = 0; i < tutorial_pages; i++) {
		if (i == _tutorial_page_index) {
			text.append("@");
		} else {
			text.append("*");
		}
	}
	_small_text.generate(0, -72, text, _text_sprites);

	auto frame = _shared.get_frame_count() / 10;

	if (_tutorial_page_index == 0) {
		_small_text.set_alignment(bn::sprite_text_generator::alignment_type::CENTER);
		_small_text.generate(0, -60, "MOVEMENT", _text_sprites);

		auto x = -72;

		auto spin_index = frame % 28;
		auto dpad_index = 0;
		auto a_pressed = false;
		auto b_pressed = false;
		auto ship_pos = 0;

		if (spin_index < 7) {
			spin_index = helpers::posmod(-4 + spin_index, 32);
			dpad_index = 2;
		} else if (spin_index < 14) {
			ship_pos = spin_index - 7;
			spin_index = 3;
			a_pressed = true;
		} else if (spin_index < 21) {
			spin_index = helpers::posmod(3 - (spin_index - 14), 32);
			dpad_index = 1;
			ship_pos = 7;
		} else {
			ship_pos = 28 - spin_index;
			spin_index = helpers::posmod(-4, 32);
			b_pressed = true;
		}

		auto ship = bn::sprite_items::ship_green.create_sprite(x + (ship_pos * 4), -18, 7);
		_tutorial_sprites.push_back(ship);

		auto a_button = bn::sprite_items::buttons.create_sprite(x + 56 + 16, -20, a_pressed ? 1 : 0);
		_tutorial_sprites.push_back(a_button);
		auto b_button = bn::sprite_items::buttons.create_sprite(x + 56, -4, b_pressed ? 3 : 2);
		_tutorial_sprites.push_back(b_button);

		_small_text.set_alignment(bn::sprite_text_generator::alignment_type::LEFT);

		_small_text.generate(x + 72 + 16, -20, "FORWARD", _text_sprites);
		_small_text.generate(x + 72, -4, "BACK", _text_sprites);
		if (a_pressed) {
			_small_text.generate(x, -8, "-->", _text_sprites);
		} else if (b_pressed) {
			_small_text.generate(x, -8, "<--", _text_sprites);
		}

		auto ship_spin = bn::sprite_items::ship_green.create_sprite(x + 24, 24, spin_index);
		_tutorial_sprites.push_back(ship_spin);
		auto dpad = bn::sprite_items::dpad.create_sprite(x + 68, 24, dpad_index);
		_tutorial_sprites.push_back(dpad);
		_small_text.generate(12, 24, "TURN", _text_sprites);

	} else if (_tutorial_page_index == 1) {
		_small_text.set_alignment(bn::sprite_text_generator::alignment_type::CENTER);
		_small_text.generate(0, -60, "MINING", _text_sprites);

		auto x = -40;
		_small_text.set_alignment(bn::sprite_text_generator::alignment_type::LEFT);
		_small_text.generate(x, -30, "HOLD    TO MINE", _text_sprites);

		auto r_button = bn::sprite_items::buttons.create_sprite(x + 48, -32, (frame % 12) < 3 ? 4 : 5);
		_tutorial_sprites.push_back(r_button);

		_small_text.generate(x, -10, "FLY CLOSE TO PICKUP", _text_sprites);
		_small_text.generate(x, 0, "THE DROPPED ITEMS", _text_sprites);

		_small_text.set_alignment(bn::sprite_text_generator::alignment_type::CENTER);

		_small_text.generate(0, 28, "-- CAUTION --", _text_sprites);
		_small_text.generate(0, 40, "HARD COLLISIONS WILL", _text_sprites);
		_small_text.generate(0, 50, "DAMAGE THE DRONE", _text_sprites);

		auto mining = bn::sprite_items::mining.create_sprite(-80, -20, 0);
		_tutorial_sprites.push_back(mining);

	} else if (_tutorial_page_index == 2) {
		_small_text.set_alignment(bn::sprite_text_generator::alignment_type::CENTER);
		_small_text.generate(0, -60, "COMBAT", _text_sprites);

		auto x = -40;

		_small_text.set_alignment(bn::sprite_text_generator::alignment_type::LEFT);

		auto l_button = bn::sprite_items::buttons.create_sprite(x + 56, -32, (frame % 12) < 6 ? 6 : 7);
		_tutorial_sprites.push_back(l_button);

		_small_text.generate(x, -30, "PRESS    TO SWITCH", _text_sprites);
		_small_text.generate(x, -20, "TO THE BLASTER", _text_sprites);
		_small_text.generate(x, 0, "AND FIRE WITH", _text_sprites);

		auto r_button = bn::sprite_items::buttons.create_sprite(x + 120, -2, (frame % 12) < 6 ? 4 : 5);
		_tutorial_sprites.push_back(r_button);

		auto combat = bn::sprite_items::combat.create_sprite(-80, -20, 0);
		combat.set_horizontal_flip(true);
		_tutorial_sprites.push_back(combat);

	} else if (_tutorial_page_index == 3) {
		_small_text.set_alignment(bn::sprite_text_generator::alignment_type::CENTER);
		_small_text.generate(0, -60, "SCANNER", _text_sprites);

		auto scanner = bn::sprite_items::scanner.create_sprite(0, -20);
		_tutorial_sprites.push_back(scanner);

		_small_text.set_alignment(bn::sprite_text_generator::alignment_type::CENTER);
		_small_text.generate(0, 30, "PRESS [SELECT] TO SCAN", _text_sprites);
		_small_text.generate(0, 40, "YOUR SURROUNDINGS", _text_sprites);

	} else if (_tutorial_page_index == 4) {
		_small_text.set_alignment(bn::sprite_text_generator::alignment_type::CENTER);
		_small_text.generate(0, -60, "SAVE", _text_sprites);

		auto scanner = bn::sprite_items::scanner_exit.create_sprite(0, -20);
		_tutorial_sprites.push_back(scanner);

		_small_text.set_alignment(bn::sprite_text_generator::alignment_type::CENTER);
		_small_text.generate(0, 20, "COLLECT ENOUGH RESOURCES", _text_sprites);
		_small_text.generate(0, 30, "THEN RETURN TO THE SHIP", _text_sprites);
		_small_text.generate(0, 40, "TO SAVE THEM", _text_sprites);

		_small_text.set_alignment(bn::sprite_text_generator::alignment_type::CENTER);
		_small_text.generate(0, 55, "IF YOUR DRONE IS DESTROYED", _text_sprites);
		_small_text.generate(0, 65, "YOU LOSE THOSE ITEMS", _text_sprites);
	} else if (_tutorial_page_index == 5) {
		_small_text.set_alignment(bn::sprite_text_generator::alignment_type::CENTER);
		_small_text.generate(0, -60, "SHIP COMPUTER", _text_sprites);

		auto screens = bn::sprite_items::screen_menu_overlay.create_sprite(0, -16, (frame / 6) % 4);
		_tutorial_sprites.push_back(screens);

		_small_text.set_alignment(bn::sprite_text_generator::alignment_type::CENTER);
		_small_text.generate(0, 20, "SELL RESOURCES ON", _text_sprites);
		_small_text.generate(0, 30, "THE BOUNTY SCREEN", _text_sprites);
		_small_text.generate(0, 45, "UPGRADE YOUR DRONE", _text_sprites);
		_small_text.generate(0, 55, "ON THE MODULES SCREEN", _text_sprites);

	} else if (_tutorial_page_index == 6) {
		_small_text.set_alignment(bn::sprite_text_generator::alignment_type::CENTER);
		_small_text.generate(0, 0, "GOOD LUCK", _text_sprites);
		_small_text.generate(0, 10, ":)", _text_sprites);
	}
}

void ship_scene::_update_bounties_screen() {
	if (bn::keypad::b_released()) {
		_viewing_menu.reset();
		_shared.play_whoosh();
		_selected_bounty_index = 0;
		_shared.clear_collected_bounties();
		_shared.save();
		_item_sprites.clear();
		return;
	}

	auto bounties = _shared.get_bounties();

	if (bn::keypad::up_released()) {
		_selected_bounty_index = bn::max(0, _selected_bounty_index - 1);
		_shared.play_click();
	} else if (bn::keypad::down_released()) {
		_selected_bounty_index = bn::min(_selected_bounty_index + 1, bounties.size() - 1);
		_shared.play_click();
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
			_small_text.generate(-96, y, ">", _text_sprites);
		}

		if (b.collected) {
			_small_text.generate(-60, y, "[COLLECTED]", _text_sprites);
		} else {
			_small_text.set_alignment(bn::sprite_text_generator::alignment_type::RIGHT);

			text.clear();
			helpers::append_with_padding(text_stream, _shared.get_inventory_count(b.resource), 3, ' ');
			text.append("  ");
			helpers::append_with_padding(text_stream, b.amount, 2, ' ');
			_small_text.generate(-30, y, text, _text_sprites);

			_small_text.generate(-50, y, "/", _text_sprites);

			_small_text.set_alignment(bn::sprite_text_generator::alignment_type::LEFT);

			auto item = get_item_info(b.resource);
			auto sprite = bn::sprite_items::dropped_items.create_sprite(-18, y);
			sprite.set_tiles(bn::sprite_items::dropped_items.tiles_item()
							.create_tiles(item.sprite_index + _shared.get_frame_count() / 30 % 2));
			_item_sprites.push_back(sprite);

			_small_text.generate(-12, y, item.display_name, _text_sprites);

			text.clear();
			text_stream.append("$");
			helpers::append_with_padding(text_stream, b.price, 3, ' ');
			_small_text.generate(48, y, text, _text_sprites);
		}
	}
}

void ship_scene::_update_inventory_screen() {
	if (bn::keypad::b_released()) {
		_viewing_menu.reset();
		_shared.play_whoosh();
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
		_shared.save();
		_shared.play_whoosh();
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
		_shared.play_click();
	} else if (bn::keypad::down_released() && _selected_upgrade_index < UPGRADE_COUNT + 1) {
		_selected_upgrade_index++;
		_shared.play_click();
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
			if (_shared.buy_upgrade_slot()) {
				_shared.play_select();
			} else {
				_shared.play_deny();
			}
		}
	} else if (selected_rocket) {
		_small_text.generate(-96, 50, ">", _text_sprites);

		if (bn::keypad::a_released()) {
			if (_shared.buy_rocket_launcher()) {
				_shared.play_select();
			} else {
				_shared.play_deny();
			}
		}
	} else {
		_small_text.generate(-96, -40 + _selected_upgrade_index * 10, ">", _text_sprites);

		auto selected_upgrade = static_cast<upgrade_type>(_selected_upgrade_index);

		if (bn::keypad::right_released()) {
			if (_shared.upgrade_module(selected_upgrade)) {
				_shared.play_tick(true);
			} else {
				_shared.play_deny();
			}
		} else if (bn::keypad::left_released()) {
			if (_shared.downgrade_module(selected_upgrade)) {
				_shared.play_tick(false);
			} else {
				_shared.play_deny();
			}
		}
	}
}

} // namespace game
