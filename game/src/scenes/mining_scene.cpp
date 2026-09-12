#include "scenes/mining_scene.h"

// assets
#include "bn_affine_bg_items_laser.h"
#include "bn_bg_palette_items_palette.h"
#include "bn_music_items.h"
#include "bn_regular_bg_items_green_bg.h"
#include "bn_regular_bg_items_headset_bg.h"
#include "bn_sound_items.h"
#include "bn_sprite_items_breaking.h"
#include "bn_sprite_items_crosshair.h"
#include "bn_sprite_items_dev16.h"
#include "bn_sprite_items_dev32.h"
#include "bn_sprite_items_dev8.h"
#include "bn_sprite_items_dropped_items.h"
#include "bn_sprite_items_poof.h"
#include "bn_sprite_items_projectile.h"
#include "bn_sprite_items_ship.h"
#include "bn_sprite_items_ship2x.h"

#include "bn_blending.h"

namespace game {

mining_scene::mining_scene(shared_state &p_shared, mining_state &p_state) :
		scene(p_shared),
		_state(p_state),
		_small_text(common::fixed_8x8_sprite_font),
		_rng(1),
		_tilemap_item(_tilemap_cells[0], bn::size(mining_state::TILEMAP_CELLS_SIZE, mining_state::TILEMAP_CELLS_SIZE)),
		_breaking_sprite(bn::sprite_items::breaking.create_sprite()),
		_crosshair_sprite(bn::sprite_items::crosshair.create_sprite()) {
	_frame = 0;
	_abandon_selected = false;
	_pause_tab = pause_menu_tab::NONE;

	_breaking_sprite.set_camera(_state.get_camera());
	_breaking_sprite.set_visible(false);
	_breaking_sprite.set_bg_priority(0);
	_crosshair_sprite.set_camera(_state.get_camera());
	_crosshair_sprite.set_visible(false);
	_crosshair_sprite.set_bg_priority(0);

	for (int i = 0; i < _ship_thrust_particles.max_size(); i++) {
		auto s = bn::sprite_items::poof.create_sprite();
		s.set_camera(_state.get_camera());
		s.set_visible(false);
		s.set_vertical_flip(i % 2 == 0);
		s.set_horizontal_flip((i + 1) % 4 == 0);
		_ship_thrust_particles.push_back(particle_lifetime{
				.sprite = s,
				.time = 0,
				.velocity = bn::fixed_point(0, 0),
		});
	}

	_small_text.set_alignment(bn::sprite_text_generator::alignment_type::CENTER);
	bn::string<34> text;
	bn::ostringstream text_stream(text);

	_pause_bg = bn::regular_bg_items::headset_bg.create_bg();

	_pause_bg->set_blending_enabled(true);

	bn::blending::set_transparency_alpha(1.0);
	bn::blending::set_fade_alpha(0.01);

	bn::sound_items::dialing.play();

	// sit in a hard loop while we generate the level
	while (!_state.is_generated()) {
		_text_sprites.clear();
		text.clear();
		text_stream.append("DIALING...");
		_small_text.generate(0, -6, text, _text_sprites);
		text.clear();
		text_stream.append(_state.generated_chunks_count());
		text_stream.append("/");
		text_stream.append(mining_state::MAX_CHUNKS);
		_small_text.generate(0, 6, text, _text_sprites);

		bn::core::update();
		bn::blending::set_fade_alpha(bn::min(bn::fixed(0.8), bn::blending::fade_alpha() + 0.05));

		for (int i = 0; i < 32 && !_state.is_generated(); i++) {
			_state.generate_next_chunk();
		}
	}

	_text_sprites.clear();
	text.clear();
	text_stream.append("RINGING...");
	_small_text.generate(0, -6, text, _text_sprites);
	bn::core::update();

	_state.bake_lighting();

	auto handle = bn::sound_items::connection.play();

	_text_sprites.clear();
	text.clear();
	text_stream.append("ESTABLISHING CONNECTION...");
	_small_text.generate(0, -6, text, _text_sprites);
	bn::core::update();
	_state.place_entities();

	while (handle.active()) {
		bn::core::update();
		bn::blending::set_fade_alpha(bn::min(bn::fixed(0.8), bn::blending::fade_alpha() + 0.01));
	}

	_pause_bg->set_blending_enabled(false);

	_text_sprites.clear();

	_unpause();
}

mining_scene::~mining_scene() {
}

bn::optional<scene_type> mining_scene::update() {
	bn::optional<scene_type> result;

	if (_state.ship_health <= 0) {
		_text_sprites.clear();
		_small_text.set_alignment(bn::sprite_text_generator::alignment_type::CENTER);
		_small_text.set_bg_priority(0);
		_small_text.generate(0, -8, "CONNECTION LOST", _text_sprites);
		_small_text.generate(0, 8, "[A] OK", _text_sprites);

		if (bn::keypad::a_pressed()) {
			_state.clear_inventory();
			result = scene_type::SHIP;
			return result;
		}

		return result;
	}

	if (_state.show_leave_confirmation) {
		_text_sprites.clear();
		_small_text.set_alignment(bn::sprite_text_generator::alignment_type::CENTER);
		_small_text.set_bg_priority(0);
		_small_text.generate(0, -8, "RETURN TO SHIP?", _text_sprites);
		_small_text.generate(0, 8, "[B] NO  [A] YES", _text_sprites);

		if (bn::keypad::a_pressed()) {
			_state.leave();
			result = scene_type::SHIP;
			return result;
		} else if (bn::keypad::a_pressed()) {
			_state.leave_canceled();
		}

		return result;
	}

	if (_pause_tab == pause_menu_tab::NONE) {
		// cycle modes
		if (bn::keypad::l_pressed()) {
			unsigned char mode = static_cast<unsigned char>(_state.get_drone_mode());
			_state.set_drone_mode(static_cast<drone_mode>((mode + 1) % (_shared.get_has_rocket_launcher() ? 3 : 2)));
		}

		// overlay selector
		// if (bn::keypad::l_held()) {
		// 	unsigned char mode = static_cast<unsigned char>(_state.get_drone_mode());
		// 	if (bn::keypad::up_released() && mode > 0) {
		// 		_state.set_drone_mode(static_cast<drone_mode>(mode - 1));
		// 	} else if (bn::keypad::down_released() && mode < 1) {
		// 		_state.set_drone_mode(static_cast<drone_mode>(mode + 1));
		// 	}

		// 	_text_sprites.clear();
		// 	_small_text.set_alignment(bn::sprite_text_generator::alignment_type::CENTER);
		// 	_small_text.set_bg_priority(0);
		// 	_small_text.generate(0, -16, "MODE", _text_sprites);
		// 	_small_text.generate(0, 8, "MINING", _text_sprites);
		// 	_small_text.generate(0, 8 + 12, "COMBAT", _text_sprites);

		// 	_small_text.generate(-32, mode * 12 + 8, ">", _text_sprites);

		// 	return result;
		// }

		_update_space();
		_update_overlay_text();

		if (bn::keypad::start_released()) {
			_pause(false);
		} else if (bn::keypad::select_released()) {
			_pause(true);
		}

	} else {
		_update_pause_menu();

		if (bn::keypad::start_released() || bn::keypad::b_released() || bn::keypad::select_released()) {
			_pause_item_sprites.clear();

			_unpause();
		}

		if (_abandon_selected) {
			// todo state should handle this
			_state.clear_inventory();
			result = scene_type::SHIP;
			_abandon_selected = false;
			return result;
		}
	}

	_frame++;

	return result;
}

void mining_scene::_pause(bool p_show_radar) {
	bn::bg_palettes::set_transparent_color(bn::color(0, 0, 0));

	_shared.play_load();

	_ship_laser.reset();
	_bg_bg.reset();
	_tilemap_bg.reset();
	_ship_sprite.reset();
	_crosshair_sprite.set_visible(false);

	for (int i = 0; i < _floating_item_sprites.size(); i++) {
		_floating_item_sprites.at(i).set_visible(false);
	}

	for (int i = 0; i < _proj_sprites.size(); i++) {
		_proj_sprites.at(i).set_visible(false);
	}

	for (int i = 0; i < _ship_thrust_particles.size(); i++) {
		_ship_thrust_particles.at(i).sprite.set_visible(false);
	}

	for (auto it = _state.turrets.begin(); it != _state.turrets.end(); ++it) {
		auto &obj = *it;
		obj.set_visible(false);
	}

	for (auto it = _state.creeps.begin(); it != _state.creeps.end(); ++it) {
		auto &obj = *it;
		obj.set_visible(false);
	}

	_pause_tab = p_show_radar ? pause_menu_tab::SCANNER : pause_menu_tab::INVENTORY;
	_cur_menu_option = menu_item_options::ABANDON_DRONE;
}

void mining_scene::_unpause() {
	// cleanup
	_scan_map_bg.reset();
	_pause_ship_sprite.reset();
	_pause_bg.reset();

	bn::core::update();

	bn::bg_palettes::set_transparent_color(bn::color(0, 1, 3));

	// setup tilemap

	bn::bg_tiles::set_allow_offset(false);

	_tilemap_bg_item = bn::regular_bg_item(
			bn::regular_bg_tiles_items::tiles,
			bn::bg_palette_items::palette,
			this->_tilemap_item);

	bn::bg_tiles::set_allow_offset(true);

	_tilemap_bg = _tilemap_bg_item->create_bg(0, 0);
	_tilemap_bg->set_priority(2);
	_tilemap_bg->set_camera(_state.get_camera());

	_tilemap_loaded_point = _state.point_to_tilemap_pos(_state.get_camera().position());
	_update_tilemap();

	_bg_bg = bn::regular_bg_items::green_bg.create_bg();
	_ship_laser = bn::affine_bg_items::laser.create_bg();

	_bg_bg->set_visible(false);
	_ship_laser->set_visible(false);

	_bg_bg->set_priority(3);
	_ship_laser->set_priority(2);

	_ship_laser->set_camera(_state.get_camera());
	_ship_laser->set_wrapping_enabled(false);
	_ship_laser->set_pivot_position(bn::point(0, 64));

	_ship_sprite = bn::sprite_items::ship.create_sprite();
	_ship_sprite->set_camera(_state.get_camera());
	_ship_sprite->set_bg_priority(0);
	_ship_sprite->set_position(_state.ship_hitbox.position());

	_crosshair_sprite.set_visible(true);
	_bg_bg->set_visible(true);

	for (auto it = _state.turrets.begin(); it != _state.turrets.end(); ++it) {
		auto &obj = *it;
		obj.set_visible(true);
	}

	for (auto it = _state.creeps.begin(); it != _state.creeps.end(); ++it) {
		auto &obj = *it;
		obj.set_visible(true);
	}

	_pause_tab = pause_menu_tab::NONE;
}

inline bool _has_flags(unsigned short value, unsigned short mask) {
	return (value & mask) == mask;
}

void mining_scene::_set_tilemap_tile(int seed, int p_x, int p_y, int p_edge_mask, tile_material p_material, unsigned char p_light_level) {
	const int tileset_columns = 16;

	const int solid_offsets[3] = { tileset_columns, tileset_columns + 1, 0 };

	int corner_ids[4] = { 0, 0, 0, 0 };
	int corner_palette[4] = { p_light_level, p_light_level, p_light_level, p_light_level };
	bool corner_flip_v[4] = { false, false, false, false };
	bool corner_flip_h[4] = { false, false, false, false };

	if (p_material == tile_material::AIR) {
		// leave the index on 0
	} else if (p_light_level == 3) {
		corner_ids[0] = tileset_columns;
		corner_ids[1] = tileset_columns;
		corner_ids[2] = tileset_columns;
		corner_ids[3] = tileset_columns;
	} else {
		_rng.set_seed(seed);

		// TOP = 1 << 0, //          0000 0001
		// RIGHT = 1 << 1, //        0000 0010
		// BOTTOM = 1 << 2, //       0000 0100
		// LEFT = 1 << 3, //         0000 1000
		// TOP_LEFT = 1 << 4, //     0001 0000
		// TOP_RIGHT = 1 << 5, //    0010 0000
		// BOTTOM_RIGHT = 1 << 6, // 0100 0000
		// BOTTOM_LEFT = 1 << 7, //  1000 0000

		if (p_edge_mask != 0xFF) {
			auto top = _has_flags(p_edge_mask, tile_flags::TOP);
			auto left = _has_flags(p_edge_mask, tile_flags::LEFT);
			auto right = _has_flags(p_edge_mask, tile_flags::RIGHT);
			auto bottom = _has_flags(p_edge_mask, tile_flags::BOTTOM);
			auto top_left = _has_flags(p_edge_mask, tile_flags::TOP_LEFT);
			auto top_right = _has_flags(p_edge_mask, tile_flags::TOP_RIGHT);
			auto bottom_left = _has_flags(p_edge_mask, tile_flags::BOTTOM_LEFT);
			auto bottom_right = _has_flags(p_edge_mask, tile_flags::BOTTOM_RIGHT);

			if (!top || !left || !top_left) {
				if (top && !left) {
					corner_ids[0] = 1;
				} else if (top && left) {
					corner_ids[0] = 2;
					corner_flip_h[0] = true;
					corner_palette[0] = 0;
				} else if (!top && left) {
					corner_ids[0] = 3;
				} else {
					corner_ids[0] = 4;
					corner_flip_h[0] = true;
				}
			}
			if (!top || !right || !top_right) {
				if (top && !right) {
					corner_ids[1] = 1;
					corner_flip_h[1] = true;
				} else if (top && right) {
					corner_ids[1] = 2;
					corner_palette[1] = 0;
				} else if (!top && right) {
					corner_ids[1] = 3;
				} else {
					corner_ids[1] = 4;
				}
			}
			if (!bottom || !left || !bottom_left) {
				if (bottom && !left) {
					corner_ids[2] = 1;
				} else if (bottom && left) {
					corner_ids[2] = tileset_columns + 2;
					corner_flip_h[2] = true;
					corner_palette[2] = 0;
				} else if (!bottom && left) {
					corner_ids[2] = tileset_columns + 3;
				} else {
					corner_ids[2] = tileset_columns + 4;
					corner_flip_h[2] = true;
				}
			}
			if (!bottom || !right || !bottom_right) {
				if (bottom && !right) {
					corner_ids[3] = 1;
					corner_flip_h[3] = true;
				} else if (bottom && right) {
					corner_ids[3] = tileset_columns + 2;
					corner_palette[3] = 0;
				} else if (!bottom && right) {
					corner_ids[3] = tileset_columns + 3;
				} else {
					corner_ids[3] = tileset_columns + 4;
				}
			}
		}

		for (size_t i = 0; i < 4; i++) {
			auto s = corner_ids[i];

			// tiles that randomize vertical flip
			if (s == 0 || s == 1) {
				corner_flip_v[i] = _rng.get_bool();
			}

			// tiles that randomize horizontal flip
			if (s == 0 || s % tileset_columns == 3) {
				corner_flip_h[i] = _rng.get_bool();
			}

			// tiles that randomize light level
			if (s == 0) {
				corner_palette[i] = bn::min(3, corner_palette[i] + _rng.get_int() % (corner_palette[i] + 2));
			}

			if (s == 0) {
				auto rand_offset = _rng.get_int(p_material == tile_material::ROCK || p_material == tile_material::BEDROCK ? 2 : 3);
				s = solid_offsets[rand_offset];
			}

			// offset it to match the material
			s += bn::max(0, static_cast<int>(p_material) - 2) * tileset_columns * 2;

			corner_ids[i] = s;
		}
	}

	// write back

	// get references to current tiles
	bn::regular_bg_map_cell &top_left = _tilemap_cells[_tilemap_item.cell_index(p_x * 2, p_y * 2)];
	bn::regular_bg_map_cell &top_right = _tilemap_cells[_tilemap_item.cell_index(p_x * 2 + 1, p_y * 2)];
	bn::regular_bg_map_cell &bottom_left = _tilemap_cells[_tilemap_item.cell_index(p_x * 2, p_y * 2 + 1)];
	bn::regular_bg_map_cell &bottom_right = _tilemap_cells[_tilemap_item.cell_index(p_x * 2 + 1, p_y * 2 + 1)];

	bn::regular_bg_map_cell_info top_left_info(top_left);
	bn::regular_bg_map_cell_info top_right_info(top_right);
	bn::regular_bg_map_cell_info bottom_left_info(bottom_left);
	bn::regular_bg_map_cell_info bottom_right_info(bottom_right);

	if (p_material == tile_material::BEDROCK) {
		top_left_info.set_palette_id(2);
		top_right_info.set_palette_id(2);
		bottom_left_info.set_palette_id(2);
		bottom_right_info.set_palette_id(2);
	} else {
		top_left_info.set_palette_id(corner_palette[0]);
		top_right_info.set_palette_id(corner_palette[1]);
		bottom_left_info.set_palette_id(corner_palette[2]);
		bottom_right_info.set_palette_id(corner_palette[3]);
	}

	top_left_info.set_tile_index(corner_ids[0]);
	top_right_info.set_tile_index(corner_ids[1]);
	bottom_left_info.set_tile_index(corner_ids[2]);
	bottom_right_info.set_tile_index(corner_ids[3]);

	top_left_info.set_horizontal_flip(corner_flip_h[0]);
	top_right_info.set_horizontal_flip(corner_flip_h[1]);
	bottom_left_info.set_horizontal_flip(corner_flip_h[2]);
	bottom_right_info.set_horizontal_flip(corner_flip_h[3]);

	top_left_info.set_vertical_flip(corner_flip_v[0]);
	top_right_info.set_vertical_flip(corner_flip_v[1]);
	bottom_left_info.set_vertical_flip(corner_flip_v[2]);
	bottom_right_info.set_vertical_flip(corner_flip_v[3]);

	top_left = top_left_info.cell();
	top_right = top_right_info.cell();
	bottom_left = bottom_left_info.cell();
	bottom_right = bottom_right_info.cell();
}

void mining_scene::_update_tilemap() {
	_tilemap_bg->set_position(
			_tilemap_loaded_point * mining_state::TILEMAP_LOAD_STRIDE_PX);
	auto top_left_world_point = _tilemap_loaded_point * mining_state::TILEMAP_LOAD_STRIDE_PX;
	top_left_world_point -= bn::point(128, 128);
	auto top_left_tile_point = top_left_world_point / 16;

	// populate current tilemap
	for (int y = 0; y < mining_state::TILEMAP_SIZE; y++) {
		for (int x = 0; x < mining_state::TILEMAP_SIZE; x++) {
			auto px = top_left_tile_point.x() + x;
			auto py = top_left_tile_point.y() + y;

			auto tile = _state.get_tile_at(px, py);

			auto seed = helpers::tile_pos_to_index(
					px,
					py,
					mining_state::SPACE_TILE_WIDTH);

			if (tile.material == tile_material::AIR) {
				_set_tilemap_tile(seed, x, y, 0, tile.material, 0);
				continue;
			}

			auto top = _state.get_tile_at(px, py - 1);
			auto right = _state.get_tile_at(px + 1, py);
			auto bottom = _state.get_tile_at(px, py + 1);
			auto left = _state.get_tile_at(px - 1, py);

			auto top_left = _state.get_tile_at(px - 1, py - 1);
			auto top_right = _state.get_tile_at(px + 1, py - 1);
			auto bottom_left = _state.get_tile_at(px - 1, py + 1);
			auto bottom_right = _state.get_tile_at(px + 1, py + 1);

			unsigned short flag = 0;
			if (top.material != tile_material::AIR) {
				flag |= tile_flags::TOP;
			}
			if (right.material != tile_material::AIR) {
				flag |= tile_flags::RIGHT;
			}
			if (bottom.material != tile_material::AIR) {
				flag |= tile_flags::BOTTOM;
			}
			if (left.material != tile_material::AIR) {
				flag |= tile_flags::LEFT;
			}

			if (top_left.material != tile_material::AIR) {
				flag |= tile_flags::TOP_LEFT;
			}
			if (top_right.material != tile_material::AIR) {
				flag |= tile_flags::TOP_RIGHT;
			}
			if (bottom_left.material != tile_material::AIR) {
				flag |= tile_flags::BOTTOM_LEFT;
			}
			if (bottom_right.material != tile_material::AIR) {
				flag |= tile_flags::BOTTOM_RIGHT;
			}

			_set_tilemap_tile(seed, x, y, flag, tile.material, tile.light_level);
		}
	}

	bn::regular_bg_map_ptr map = _tilemap_bg->map();
	map.reload_cells_ref();
}

void mining_scene::_update_space() {
	_state.update();

	_ship_sprite->set_tiles(bn::sprite_items::ship.tiles_item()
					.create_tiles((((_state.ship_rotation / bn::fixed(360.0)) * 32 - 0.5).integer() + 32) % 32));
	_ship_sprite->set_position(_state.ship_hitbox.position());
	_ship_sprite->set_visible((_state.ship_invincible_timer / 2) % 2 == 0);

	_bg_bg->set_position(-_state.get_camera().position() / 2);

	auto aim_dir = _state.get_aim_direction();
	auto crosshair_target_pos = _state.ship_hitbox.center() + aim_dir;
	if (helpers::distance(crosshair_target_pos, _crosshair_sprite.position()) > 1.0) {
		_crosshair_sprite.set_position(helpers::lerp_fixed_point(_crosshair_sprite.position(), crosshair_target_pos, 0.55));
	} else {
		_crosshair_sprite.set_position(crosshair_target_pos);
	}

	auto target_cell = _state.get_targeting_cell();
	auto mining_progress = _state.get_mining_progress();
	auto is_mining = mining_progress > 0;

	_breaking_sprite.set_visible(is_mining);
	if (!is_mining) {
		_ship_laser->set_visible(false);
	}

	if (target_cell.has_value()) {
		if (_crosshair_frame < 4) {
			_crosshair_frame++;
		}

		if (is_mining) {
			auto hit_point = target_cell.value() * 16 + bn::point(8, 8);
			auto dist = helpers::distance(_state.ship_hitbox.center(), hit_point);

			_breaking_sprite.set_position(hit_point);
			_breaking_sprite.set_tiles(bn::sprite_items::breaking.tiles_item()
							.create_tiles(helpers::remap_fixed(mining_progress, 0, 1, 0, 4).integer()));

			_ship_laser->set_position(_state.ship_hitbox.position());
			_ship_laser->set_rotation_angle(bn::degrees_atan2(aim_dir.x().integer(), aim_dir.y().integer()) + 180);

			// flicker
			_ship_laser->set_horizontal_scale(_frame % 4 < 2 ? 0.05 : 0.06);

			if (dist > 1.0) {
				_ship_laser->set_visible(is_mining);
				_ship_laser->set_vertical_scale(dist / 128.0);
			} else {
				_ship_laser->set_visible(false);
			}
		}

	} else if (_crosshair_frame > 0) {
		_crosshair_frame--;
	}
	_crosshair_sprite.set_tiles(bn::sprite_items::crosshair.tiles_item().create_tiles(_crosshair_frame / 2));

	bn::point tilemap_pos = _state.point_to_tilemap_pos(_state.ship_hitbox.position());

	if (tilemap_pos != _tilemap_loaded_point || _state.is_tileset_dirty) {
		_tilemap_loaded_point = tilemap_pos;
		_update_tilemap();
		_state.is_tileset_dirty = false;
	}

	if (_state.is_thrusting() && _thrust_particle_time == 0) {
		auto &p = _ship_thrust_particles.at(_next_thrust_particle);
		p.time = 60;
		auto dir = helpers::angle_to_dir(-_state.ship_rotation);
		p.sprite.set_position(_state.ship_hitbox.center() + helpers::set_length(dir, 8.0));
		p.velocity = helpers::set_length(dir, 0.8);
		_thrust_particle_time = 4;
		_next_thrust_particle = (_next_thrust_particle + 1) % _ship_thrust_particles.max_size();
	}

	if (_thrust_particle_time > 0) {
		_thrust_particle_time--;
	}

	for (auto &obj : _ship_thrust_particles) {
		// obj.tiles
		if (obj.time == 0) {
			obj.sprite.set_visible(false);
		} else {
			obj.sprite.set_visible(true);
			obj.time--;
			obj.sprite.set_tiles(bn::sprite_items::poof.tiles_item()
							.create_tiles(bn::clamp((70 - obj.time) / 10, 1, 3)));
			obj.sprite.set_position(obj.sprite.position() + obj.velocity);
		}
	}

	// render the floating_items

	int sprite_index = 0;
	int sprite_flicker_index = -1;

	int flicker_frame = _obj_flicker_frame / 10;

	for (auto obj : _state.floating_items) {
		if (sprite_index >= mining_state::MAX_VISIBLE_FLOATING_ITEMS) {
			break;
		}

		sprite_flicker_index++;

		if (!helpers::is_point_in_view(_state.get_camera().position(), obj.get_position(), 8)) {
			continue;
		}

		BN_ASSERT(sprite_index <= _floating_item_sprites.size());

		if (sprite_flicker_index >= mining_state::MAX_VISIBLE_FLOATING_ITEMS / 2 && flicker_frame % 2 == sprite_flicker_index % 2) {
			continue;
		}

		if (sprite_index == _floating_item_sprites.size()) {
			// add sprite
			_floating_item_sprites.push_back(bn::sprite_items::dropped_items.create_sprite());
		}
		bn::sprite_ptr &existing = _floating_item_sprites.at(sprite_index);
		existing.set_tiles(bn::sprite_items::dropped_items.tiles_item()
						.create_tiles(obj.get_sprite_index()));
		existing.set_position(obj.get_position());
		existing.set_visible(true);
		existing.set_camera(_state.get_camera());
		sprite_index++;
	}

	_obj_flicker_frame = (_obj_flicker_frame + 1) % 60;

	// hide unused
	for (; sprite_index < _floating_item_sprites.size(); sprite_index++) {
		_floating_item_sprites.at(sprite_index).set_visible(false);
	}

	sprite_index = 0;

	for (auto proj : _state.projectiles) {
		if (sprite_index >= mining_state::MAX_VISIBLE_PROJECTILES) {
			break;
		}

		sprite_flicker_index++;

		if (!helpers::is_point_in_view(_state.get_camera().position(), proj.get_position(), 8)) {
			continue;
		}

		BN_ASSERT(sprite_index <= _proj_sprites.size());

		if (sprite_index == _proj_sprites.size()) {
			// add sprite
			_proj_sprites.push_back(bn::sprite_items::projectile.create_sprite());
		}
		bn::sprite_ptr &existing = _proj_sprites.at(sprite_index);
		existing.set_tiles(bn::sprite_items::projectile.tiles_item()
						.create_tiles(proj.get_frame()));
		existing.set_position(proj.get_position());
		existing.set_visible(true);
		existing.set_camera(_state.get_camera());
		sprite_index++;
	}

	// hide unused
	for (; sprite_index < _proj_sprites.size(); sprite_index++) {
		_proj_sprites.at(sprite_index).set_visible(false);
	}
}

void mining_scene::_update_pause_menu() {
	int type_index = static_cast<int>(_pause_tab);
	constexpr int max_type = static_cast<int>(pause_menu_tab::SYSTEM);

	if (type_index > 1 && bn::keypad::l_released()) {
		_pause_tab = static_cast<pause_menu_tab>(type_index - 1);
		_shared.play_click();
	} else if (type_index < max_type && bn::keypad::r_released()) {
		_pause_tab = static_cast<pause_menu_tab>(type_index + 1);
		_shared.play_click();
	}

	bn::string<40> text;
	bn::ostringstream text_stream(text);

	_text_sprites.clear();
	_pause_item_sprites.clear();

	if (_pause_tab == pause_menu_tab::INVENTORY) {
		if (!_pause_ship_sprite.has_value()) {
			_pause_ship_sprite = bn::sprite_items::ship2x.create_sprite();
		}

		_pause_ship_sprite->set_position(48, 0);
		_pause_ship_sprite->set_tiles(bn::sprite_items::ship2x.tiles_item()
						.create_tiles((_frame / 6) % 32));

		for (int i = 0; i < ITEM_TYPE_COUNT; i++) {
			auto item = items[i];
			auto y = -20 + i * 9;

			text.clear();
			helpers::append_with_padding(text_stream, _state.item_inventory[i], 0, ' ');
			_small_text.set_alignment(bn::sprite_text_generator::alignment_type::RIGHT);
			_small_text.generate(-80, y, text, _text_sprites);

			auto sprite = bn::sprite_items::dropped_items.create_sprite(-70, y);
			sprite.set_tiles(bn::sprite_items::dropped_items.tiles_item()
							.create_tiles(item.sprite_index + _shared.get_frame_count() / 30 % 2));
			_pause_item_sprites.push_back(sprite);

			_small_text.set_alignment(bn::sprite_text_generator::alignment_type::LEFT);
			_small_text.generate(-64, y, item.display_name, _text_sprites);
		}
	} else {
		if (_pause_ship_sprite.has_value()) {
			_pause_ship_sprite.reset();
		}
	}

	if (_pause_tab == pause_menu_tab::BOUNTIES) {
		auto bounties = _shared.get_bounties();

		for (int i = 0; i < bounties.size(); i++) {
			auto b = bounties[i];

			auto y = -32 + i * 10;

			if (b.collected) {
				_small_text.generate(-60, y, "[COLLECTED]", _text_sprites);
			} else {
				_small_text.set_alignment(bn::sprite_text_generator::alignment_type::RIGHT);

				text.clear();
				helpers::append_with_padding(text_stream, _state.item_inventory[static_cast<int>(b.resource)] + _shared.get_inventory_count(b.resource), 3, ' ');
				text.append("  ");
				helpers::append_with_padding(text_stream, b.amount, 2, ' ');
				_small_text.generate(-30, y, text, _text_sprites);

				_small_text.generate(-50, y, "/", _text_sprites);

				_small_text.set_alignment(bn::sprite_text_generator::alignment_type::LEFT);

				auto item = get_item_info(b.resource);
				auto sprite = bn::sprite_items::dropped_items.create_sprite(-18, y);
				sprite.set_tiles(bn::sprite_items::dropped_items.tiles_item()
								.create_tiles(item.sprite_index + _shared.get_frame_count() / 30 % 2));
				_pause_item_sprites.push_back(sprite);

				_small_text.generate(-12, y, item.display_name, _text_sprites);

				text.clear();
				text_stream.append("$");
				helpers::append_with_padding(text_stream, b.price, 3, ' ');
				_small_text.generate(48, y, text, _text_sprites);
			}
		}

		_small_text.set_alignment(bn::sprite_text_generator::alignment_type::CENTER);
		_small_text.generate(0, 70, "RETURN TO SHIP TO COLLECT", _text_sprites);
	}

	if (_pause_tab == pause_menu_tab::SCANNER) {
		if (_pause_bg.has_value()) {
			_pause_bg.reset();
			bn::core::update();
		}

		bn::bg_palettes::set_transparent_color(bn::color(0, 0, 0));

		if (!_scan_map_bg.has_value()) {
			text.clear();
			_small_text.set_alignment(bn::sprite_text_generator::alignment_type::CENTER);
			text_stream.append("SCANNING...");
			_small_text.generate(0, 70, text, _text_sprites);

			bn::core::update();

			bn::sound_items::scanner.play(0.8);

			_scan_map_bg = bn::dp_direct_bitmap_bg_ptr::create();
			bn::dp_direct_bitmap_bg_painter painter(_scan_map_bg.value());

			painter.fill(bn::color(0, 0, 0));
			painter.flip_page_now();
			bn::core::update();
			painter.flip_page_now();

			bn::point size = bn::point(160, 128);
			bn::point ship_pos = _state.space_point_to_tile_point(_state.ship_hitbox.center());

			// https://stackoverflow.com/a/1555236/9473815

			int x, y, dx, dy;
			x = y = dx = 0;
			dy = -1;
			int t = bn::max(size.x(), size.y());
			int max_i = t * t;

			auto level = _shared.get_upgrade_level(upgrade_type::SCANNER);
			int range = 16 + level * 12;
			bool show_ore = level == MAX_UPGRADE_LEVEL;

			for (int i = 0; i < max_i; i++) {
				if (
						(-size.x() / 2 <= x) && (x <= size.x() / 2) //
						&& (-size.y() / 2 <= y && y <= size.y() / 2)) {
					// draw

					auto tile_point = bn::point(ship_pos.x() + x, ship_pos.y() + y);
					auto plot_point = bn::point(size.x() / 2 + x * 2, size.y() / 2 + y * 2);

					if (plot_point.x() < size.x() && plot_point.y() < size.y() && plot_point.x() >= 0 && plot_point.y() >= 0) {
						auto dist = helpers::distance(ship_pos, tile_point);

						bn::fixed noise = 0;
						if ((helpers::posmod(tile_point.x(), 2) == 0) != (helpers::posmod(tile_point.y(), 2) == 0)) {
							noise = _rng.get_bool() ? 0.3 : 0.2;
						} else {
							noise = _rng.get_bool() ? 0.6 : 0.0;
						}

						auto opacity = bn::clamp(helpers::remap_fixed(dist, range / 3, range, 0.1, 1) + noise, bn::fixed(0), bn::fixed(1));

						if (i == 0) {
							painter.rectangle(plot_point.x(), plot_point.y(), plot_point.x() + 1, plot_point.y() + 1,
									bn::color(4, 31, 4));
						} else if (_state.is_solid_tile(tile_point)) {
							auto data = _state.get_tile(tile_point);

							bn::color c;
							if (show_ore) {
								c = get_tile_color(data.material);
							} else {
								if (data.material == tile_material::BEDROCK) {
									c = get_tile_color(tile_material::BEDROCK);
								} else {
									c = get_tile_color(tile_material::ROCK);
								}
							}

							auto raw_color = bn::color(
									bn::min(31, c.red()),
									bn::min(31, c.green()),
									bn::min(31, c.blue()));
							auto raw_highlight = bn::color(
									bn::min(31, c.red() * 3),
									bn::min(31, c.green() * 3),
									bn::min(31, c.blue() * 3));

							auto color = helpers::lerp_color(raw_color, bn::color(0, 0, 0), opacity);
							auto highlight = helpers::lerp_color(raw_highlight, bn::color(0, 0, 0), opacity);

							bool top = _state.is_solid_tile(tile_point + bn::point(0, -1));
							bool left = _state.is_solid_tile(tile_point + bn::point(-1, 0));
							bool right = _state.is_solid_tile(tile_point + bn::point(1, 0));
							bool bottom = _state.is_solid_tile(tile_point + bn::point(0, 1));

							painter.plot(plot_point.x(), plot_point.y(), !top || !left ? highlight : color);
							painter.plot(plot_point.x() + 1, plot_point.y(), !top || !right ? highlight : color);
							painter.plot(plot_point.x(), plot_point.y() + 1, !bottom || !left ? highlight : color);
							painter.plot(plot_point.x() + 1, plot_point.y() + 1, !bottom || !right ? highlight : color);

						} else {
							painter.rectangle(plot_point.x(), plot_point.y(), plot_point.x() + 1, plot_point.y() + 1,
									helpers::lerp_color(bn::color(1, 1, 3), bn::color(0, 0, 0), opacity));
						}
					}
				}

				if (x == y) {
					// todo i can't get this to work...
					// if (bn::keypad::any_released()) {
					// 	bn::core::update();
					// 	_unpause();
					// 	return;
					// }

					painter.flip_page_now();
					bn::core::update();
					painter.flip_page_now();
				}

				if (x == y || ((x < 0 && (x == -y)) || ((x > 0) && (x == 1 - y)))) {
					t = dx;
					dx = -dy;
					dy = t;
				}
				x += dx;
				y += dy;
			}

			painter.flip_page_now();
		} else {
			_scan_map_bg->set_visible(true);
		}
	} else {
		bn::bg_palettes::set_transparent_color(bn::color(0, 1, 3));

		if (_scan_map_bg.has_value()) {
			_scan_map_bg.reset();
			bn::core::update();
		}

		if (!_pause_bg.has_value()) {
			_pause_bg = bn::regular_bg_items::headset_bg.create_bg();
		}
	}

	text.clear();
	if (type_index > 1) {
		_small_text.set_alignment(bn::sprite_text_generator::alignment_type::LEFT);
		text.append("L<");
		_small_text.generate(-100, -72, text, _text_sprites);
	}
	if (type_index < max_type) {
		text.clear();
		_small_text.set_alignment(bn::sprite_text_generator::alignment_type::RIGHT);
		text.append(">R");
		_small_text.generate(100, -72, text, _text_sprites);
	}

	text.clear();
	_small_text.set_alignment(bn::sprite_text_generator::alignment_type::CENTER);
	append_pause_menu_name(text_stream, _pause_tab);
	_small_text.generate(0, -74, text, _text_sprites);

	text.clear();
	for (int i = 1; i <= max_type; i++) {
		if (i == type_index) {
			text.append("@");
		} else {
			text.append("*");
		}
	}
	_small_text.generate(0, -64, text, _text_sprites);

	if (_pause_tab == pause_menu_tab::SYSTEM) {
		_small_text.set_alignment(bn::sprite_text_generator::alignment_type::LEFT);

		// text.clear();
		// text.append("> CONTROLS");
		// _small_text.generate(-64, -40, text, _text_sprites);

		// text.clear();
		// text.append("> MUSIC [OFF]");
		// _small_text.generate(-64, -30, text, _text_sprites);

		text.clear();
		text.append("> ABANDON DRONE");
		_small_text.generate(-64, -10, text, _text_sprites);

		if (bn::keypad::a_pressed()) {
			// todo confirmation
			_abandon_selected = true;
		}
	}
}

void mining_scene::_update_overlay_text() {
	_small_text.set_bg_priority(0);
	_text_sprites.clear();

	_small_text.set_alignment(bn::sprite_text_generator::alignment_type::LEFT);

	if (_state.get_drone_mode() == drone_mode::COMBAT) {
		_small_text.generate(-118, -74, "BLASTER", _text_sprites);
	} else if (_state.get_drone_mode() == drone_mode::MINING) {
		_small_text.generate(-118, -74, "MINING LASER", _text_sprites);
	} else {
		_small_text.generate(-118, -74, "ROCKET", _text_sprites);
	}

	bn::string<15> text;
	bn::ostringstream text_stream(text);
	text_stream.append(_state.ship_health);
	text_stream.append("/");
	text_stream.append("100");
	_small_text.generate(-118, 75, text, _text_sprites);

	_small_text.set_alignment(bn::sprite_text_generator::alignment_type::RIGHT);

	int index = 0;
	int item_frame = _state.item_queue_frame();

	for (auto it = _state.item_pickup_queue.begin(); it != _state.item_pickup_queue.end(); ++it) {
		auto &obj = *it;

		text.clear();
		text_stream.append("+");
		text_stream.append(obj.amount);
		text_stream.append(" ");
		text_stream.append(get_item_info(obj.object_type).display_name);

		auto y = bn::clamp((mining_state::ITEM_QUEUE_FRAMES_TIME - (obj.frame - item_frame)) / 4, 0, 4);

		_small_text.generate(118, 79 - index * 9 - y, text, _text_sprites);

		index++;
	}
}

} //namespace game