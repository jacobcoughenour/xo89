#include "flying_scene.h"

// assets
#include "bn_affine_bg_items_laser.h"
#include "bn_regular_bg_items_green_bg.h"
#include "bn_sprite_items_breaking.h"
#include "bn_sprite_items_crosshair.h"
#include "bn_sprite_items_dev16.h"
#include "bn_sprite_items_dev32.h"
#include "bn_sprite_items_dev8.h"
#include "bn_sprite_items_ship.h"

#include "bn_music_items.h"
#include "bn_sound_items.h"

namespace game {

void flying_scene::_rebuild_bgs() {
	bn::bg_palettes::set_transparent_color(bn::color(2, 3, 4));

	_bg_bg = bn::regular_bg_items::green_bg.create_bg();
	_ship_laser = bn::affine_bg_items::laser.create_bg();

	_bg_bg->set_visible(false);
	_ship_laser->set_visible(false);

	_bg_bg->set_priority(3);
	_ship_laser->set_priority(2);

	_ship_laser->set_camera(_camera);
	_ship_laser->set_wrapping_enabled(false);
	_ship_laser->set_pivot_position(bn::point(0, 64));
}

void flying_scene::_destroy_bgs() {
	_bg_bg.reset();
	_ship_laser.reset();
	_space.set_visible(false);
}

flying_scene::flying_scene(game_state &state) :
		_state(state),
		_camera(bn::camera_ptr::create(0, 0)),
		_rng(1),
		_bg_bg(bn::regular_bg_items::green_bg.create_bg()),
		_space(state, _camera),
		_ship_sprite(bn::sprite_items::ship.create_sprite()),
		_ship_laser(bn::affine_bg_items::laser.create_bg()),
		_breaking_sprite(bn::sprite_items::breaking.create_sprite()),
		_crosshair_sprite(bn::sprite_items::crosshair.create_sprite()) {
	_rebuild_bgs();

	_ship_sprite.set_visible(false);

	_ship_hitbox.set_width(8);
	_ship_hitbox.set_height(8);
	_ship_hitbox.set_position(_space.spawn_point());

	_ship_sprite.set_camera(_camera);
	_ship_sprite.set_bg_priority(0);
	_ship_sprite.set_position(_ship_hitbox.position());

	_breaking_sprite.set_camera(_camera);
	_breaking_sprite.set_visible(false);
	_breaking_sprite.set_bg_priority(0);
	_crosshair_sprite.set_camera(_camera);
	_crosshair_sprite.set_visible(false);
	_crosshair_sprite.set_bg_priority(0);

	// bn::music_items::milkypack01.play();

	while (!_space.is_generated()) {
		_state.small_fixed_text_generator.set_alignment(bn::sprite_text_generator::alignment_type::CENTER);
		_text_sprites.clear();
		bn::string<34> text;
		bn::ostringstream text_stream(text);
		text_stream.append("Generating...");
		_state.small_fixed_text_generator.generate(0, -6, text, _text_sprites);
		text.clear();
		text_stream.append(_space.generated_chunks_count());
		text_stream.append("/");
		text_stream.append(chunked_space::MAX_CHUNKS);
		_state.small_fixed_text_generator.generate(0, 6, text, _text_sprites);

		bn::core::update();

		for (int i = 0; i < 32 && !_space.is_generated(); i++) {
			_space.generate_next_chunk();
		}
	}

	_text_sprites.clear();
	_bg_bg->set_visible(true);
	_ship_sprite.set_visible(true);
	_crosshair_sprite.set_visible(true);
}

flying_scene::~flying_scene() {
}

bn::optional<scene_type> flying_scene::update() {
	bn::optional<scene_type> result;

	if (_is_paused) {
		_update_pause_menu();

		if (bn::keypad::start_released()) {
			// unpause

			_scan_map_bg.reset();

			_rebuild_bgs();

			_ship_sprite.set_visible(true);
			_crosshair_sprite.set_visible(true);
			_space.set_visible(true);
			_bg_bg->set_visible(true);

			_is_paused = false;
		}
	} else {
		_update_space();
		_update_overlay_text();

		if (bn::keypad::start_released()) {
			//  pause

			bn::bg_palettes::set_transparent_color(bn::color(0, 0, 0));

			_destroy_bgs();
			_ship_sprite.set_visible(false);
			_crosshair_sprite.set_visible(false);

			_is_paused = true;
		}
	}

	_frame++;

	return result;
}

void flying_scene::_update_space() {
	if (bn::keypad::left_held()) {
		_ship_rotation -= 2.5;
	}
	if (bn::keypad::right_held()) {
		_ship_rotation += 2.5;
	}
	_ship_rotation = helpers::fposmod(_ship_rotation, 360);

	_ship_sprite.set_tiles(bn::sprite_items::ship.tiles_item()
					.create_tiles(((_ship_rotation / bn::fixed(360.0)) * 32).integer() % 32));

	if (bn::keypad::a_held()) {
		_ship_velocity -= helpers::angle_to_dir(-_ship_rotation) * bn::fixed(0.045);
	}
	if (bn::keypad::b_held()) {
		_ship_velocity += helpers::angle_to_dir(-_ship_rotation) * bn::fixed(0.035);
	}
	_ship_velocity *= bn::fixed(0.99);

	bn::fixed_point cur_pos = _ship_hitbox.position();
	bn::fixed_point desired_pos = cur_pos + _ship_velocity;

	bn::fixed_rect final_aabb = _ship_hitbox;
	final_aabb.set_position(desired_pos);

	bool hit = false;

	if (
			_space.is_solid_tile(_space.space_point_to_tile_point(final_aabb.top_left())) //
			|| _space.is_solid_tile(_space.space_point_to_tile_point(final_aabb.top_right())) //
			|| _space.is_solid_tile(_space.space_point_to_tile_point(final_aabb.bottom_left())) //
			|| _space.is_solid_tile(_space.space_point_to_tile_point(final_aabb.bottom_right()))) {
		hit = true;

		_ship_velocity = _ship_velocity * bn::fixed(-0.6);
	}

	_ship_hitbox = hit ? _ship_hitbox : final_aabb;

	_ship_sprite.set_position(_ship_hitbox.position());
	_ship_laser->set_position(_ship_hitbox.position());

	const bn::fixed max_dist = 64;

	auto targetting_hit = _space.raycast(_ship_hitbox.center(), -helpers::angle_to_dir(-_ship_rotation + 8), max_dist);
	auto target_dir = -helpers::set_length(helpers::angle_to_dir(-_ship_rotation), max_dist - 4.0);

	if (targetting_hit.has_value()) {
		auto dist = helpers::distance(_ship_hitbox.center(), targetting_hit.value().intersection_pos);
		target_dir = targetting_hit.value().intersection_pos - _ship_hitbox.position();

		auto new_tile = targetting_hit.value().tile_pos;
		if (new_tile != _laser_target_cell) {
			_mining_timer = 0;
			_breaking_sprite.set_position(targetting_hit.value().intersection_pos);
		}
		_laser_target_cell = new_tile;

		if (bn::keypad::r_held()) {
			_ship_laser->set_visible(true);
			_ship_laser->set_rotation_angle(bn::degrees_atan2(target_dir.x().integer(), target_dir.y().integer()) + 180);

			// flicker
			_ship_laser->set_horizontal_scale(_frame % 4 < 2 ? 0.05 : 0.06);

			if (dist > 1.0) {
				_ship_laser->set_vertical_scale(dist / 128.0);
			} else {
				_ship_laser->set_visible(false);
			}

			_mining_timer++;

			if (_mining_timer >= 30) {
				// mine the cell
				_space.mine_tile(_laser_target_cell);
				_mining_timer = 0;
			} else {
				_breaking_sprite.set_tiles(bn::sprite_items::breaking.tiles_item()
								.create_tiles(_mining_timer / (30 / 4) % 4));
			}
		} else {
			_ship_laser->set_visible(false);
			_ship_laser->set_vertical_scale(1.4);
			_mining_timer = 0;
		}
	} else {
		_ship_laser->set_visible(false);
		_mining_timer = 0;
	}
	_bg_bg->set_position(-_camera.position() / 2);

	auto crosshair_target_pos = _ship_hitbox.center() + target_dir;
	if (helpers::distance(crosshair_target_pos, _crosshair_sprite.position()) > 1.0) {
		_crosshair_sprite.set_position(helpers::lerp_fixed_point(_crosshair_sprite.position(), crosshair_target_pos, 0.55));
	} else {
		_crosshair_sprite.set_position(crosshair_target_pos);
	}

	if (targetting_hit.has_value()) {
		if (_crosshair_frame < 4) {
			_crosshair_frame++;
		}
	} else if (_crosshair_frame > 0) {
		_crosshair_frame--;
	}
	_crosshair_sprite.set_tiles(bn::sprite_items::crosshair.tiles_item().create_tiles(_crosshair_frame / 2));

	_breaking_sprite.set_visible(_mining_timer > 0);

	// keep camera on the ship
	_camera.set_position(_ship_hitbox.position());

	_space.update();
}

void flying_scene::_update_pause_menu() {
	_state.small_fixed_text_generator.set_alignment(bn::sprite_text_generator::alignment_type::CENTER);
	_text_sprites.clear();

	bn::string<32> text;
	bn::ostringstream text_stream(text);
	// text.append("<<  [ RESOURCES ]  >>");
	text.append("<<  [ RADAR ]  >>");
	_state.small_fixed_text_generator.generate(0, -72, text, _text_sprites);

	// _state.small_fixed_text_generator.set_alignment(bn::sprite_text_generator::alignment_type::LEFT);

	// for (int i = 0; i < ITEM_TYPE_COUNT; i++) {
	// 	auto typ = static_cast<obj_type>(i);

	// 	text.clear();
	// 	helpers::append_with_padding(text_stream, _state.item_inventory[i], 3, ' ');
	// 	text_stream.append(" ");
	// 	append_item_name(text_stream, typ);

	// 	_state.small_fixed_text_generator.generate(-80, -40 + i * 9, text, _text_sprites);
	// }

	bn::core::update();

	if (!_scan_map_bg.has_value()) {
		_scan_map_bg = bn::dp_direct_bitmap_bg_ptr::create();
		bn::dp_direct_bitmap_bg_painter painter(_scan_map_bg.value());

		painter.fill(bn::color(0, 0, 0));
		painter.flip_page_now();
		bn::core::update();
		painter.flip_page_now();

		bn::point size = bn::point(160, 128);
		bn::point ship_pos = _space.space_point_to_tile_point(_ship_hitbox.center());

		// https://stackoverflow.com/a/1555236/9473815

		int x, y, dx, dy;
		x = y = dx = 0;
		dy = -1;
		int t = bn::max(size.x(), size.y());
		int max_i = t * t;

		int range = 32;

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
					if (helpers::posmod(tile_point.x(), 2) == 0 != helpers::posmod(tile_point.y(), 2) == 0) {
						noise = _rng.get_bool() ? 0.3 : 0.2;
					} else {
						noise = _rng.get_bool() ? 0.6 : 0.0;
					}

					auto opacity = bn::clamp(helpers::remap_fixed(dist, range / 3, range, 0.1, 1) + noise, bn::fixed(0), bn::fixed(1));

					if (i == 0) {
						painter.rectangle(plot_point.x(), plot_point.y(), plot_point.x() + 1, plot_point.y() + 1,
								bn::color(4, 31, 4));
					} else if (_space.is_solid_tile(tile_point)) {
						auto color = helpers::lerp_color(bn::color(10, 0, 10), bn::color(0, 0, 0), opacity);
						auto highlight = helpers::lerp_color(bn::color(31, 8, 31), bn::color(0, 0, 0), opacity);

						bool top = _space.is_solid_tile(tile_point + bn::point(0, -1));
						bool left = _space.is_solid_tile(tile_point + bn::point(-1, 0));
						bool right = _space.is_solid_tile(tile_point + bn::point(1, 0));
						bool bottom = _space.is_solid_tile(tile_point + bn::point(0, 1));

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
				painter.flip_page_now();
				bn::core::update();
				if (bn::keypad::start_released()) {
					break;
				}
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
	}
}

void flying_scene::_update_overlay_text() {
	_state.small_fixed_text_generator.set_alignment(bn::sprite_text_generator::alignment_type::RIGHT);
	_state.small_fixed_text_generator.set_bg_priority(0);
	_text_sprites.clear();

	int index = 0;
	int item_frame = _state.item_queue_frame();

	bn::string<15> text;
	bn::ostringstream text_stream(text);

	for (auto it = _state.item_pickup_queue.begin(); it != _state.item_pickup_queue.end(); ++it) {
		auto &obj = *it;

		text.clear();
		text_stream.append("+");
		text_stream.append(obj.amount);
		text_stream.append(" ");
		append_item_name(text_stream, obj.object_type);

		auto y = bn::clamp((game_state::ITEM_QUEUE_FRAMES_TIME - (obj.frame - item_frame)) / 4, 0, 4);

		_state.small_fixed_text_generator.generate(118, 80 - index * 9 - y, text, _text_sprites);

		index++;
	}
}

} //namespace game