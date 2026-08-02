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

namespace game {

flying_scene::flying_scene(game_state &state) :
		_state(state),
		_camera(bn::camera_ptr::create(0, 0)),
		_rng(1),
		_space(_camera),
		_bg_bg(bn::regular_bg_items::green_bg.create_bg()),
		_ship_sprite(bn::sprite_items::ship.create_sprite()),
		_ship_laser(bn::affine_bg_items::laser.create_bg()),
		_breaking_sprite(bn::sprite_items::breaking.create_sprite()),
		_crosshair_sprite(bn::sprite_items::crosshair.create_sprite()) {
	bn::bg_palettes::set_transparent_color(bn::color(2, 3, 4));

	_bg_bg.set_visible(false);
	_ship_sprite.set_visible(false);
	_ship_laser.set_visible(false);

	_bg_bg.set_priority(3);
	_ship_laser.set_priority(2);

	_ship_hitbox.set_width(8);
	_ship_hitbox.set_height(8);
	_ship_hitbox.set_position(_space.spawn_point());

	_ship_sprite.set_camera(_camera);
	_ship_sprite.set_bg_priority(0);
	_ship_sprite.set_position(_ship_hitbox.position());

	_breaking_sprite.set_camera(_camera);
	_crosshair_sprite.set_camera(_camera);

	_ship_laser.set_camera(_camera);
	_ship_laser.set_wrapping_enabled(false);
	_ship_laser.set_pivot_position(bn::point(0, 64));

	// bn::music_items::demo_1.play();

	while (!_space.is_generated()) {
		auto total = chunked_space::MAX_CHUNKS;

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
	_bg_bg.set_visible(true);
	_ship_sprite.set_visible(true);
}

flying_scene::~flying_scene() {
}

bn::optional<scene_type> flying_scene::update() {
	bn::optional<scene_type> result;

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
		_ship_velocity -= helpers::angle_to_dir(-_ship_rotation) * bn::fixed(0.04);
	}
	// if (bn::keypad::b_held()) {
	// 	_ship_velocity += helpers::angle_to_dir(-_ship_rotation) * bn::fixed(0.032);
	// }
	_ship_velocity *= bn::fixed(0.99);

	bn::fixed_point cur_pos = _ship_hitbox.position();
	bn::fixed_point desired_pos = cur_pos + _ship_velocity;

	// bn::fixed_rect last_aabb = _ship_hitbox;
	bn::fixed_rect final_aabb = _ship_hitbox;
	final_aabb.set_position(desired_pos);

	bool hit = false;

	if (
			_space.is_solid_tile(_space.space_point_to_tile_point(final_aabb.top_left())) || _space.is_solid_tile(_space.space_point_to_tile_point(final_aabb.top_right())) || _space.is_solid_tile(_space.space_point_to_tile_point(final_aabb.bottom_left())) || _space.is_solid_tile(_space.space_point_to_tile_point(final_aabb.bottom_right()))) {
		hit = true;

		_ship_velocity = _ship_velocity * bn::fixed(-0.6);
	}

	_ship_hitbox = hit ? _ship_hitbox : final_aabb;

	_ship_sprite.set_position(_ship_hitbox.position());
	_ship_laser.set_position(_ship_hitbox.position());

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

		if (bn::keypad::b_held()) {
			_ship_laser.set_visible(true);
			_ship_laser.set_rotation_angle(bn::degrees_atan2(target_dir.x().integer(), target_dir.y().integer()) + 180);

			// flicker
			_ship_laser.set_horizontal_scale(_frame % 4 < 2 ? 0.05 : 0.06);

			if (dist > 1.0) {
				_ship_laser.set_vertical_scale(dist / 128.0);
			} else {
				_ship_laser.set_visible(false);
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
			_ship_laser.set_visible(false);
			_ship_laser.set_vertical_scale(1.4);
			_mining_timer = 0;
		}
	} else {
		_ship_laser.set_visible(false);
		_mining_timer = 0;
	}
	_bg_bg.set_position(-_camera.position() / 2);

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

	// update_text();

	_frame++;

	return result;
}

void flying_scene::update_text() {
	_state.small_fixed_text_generator.set_alignment(bn::sprite_text_generator::alignment_type::LEFT);

	_text_sprites.clear();
	bn::string<36> text;
	bn::ostringstream text_stream(text);

	text_stream.append(_ship_velocity.y());

	_state.small_fixed_text_generator.generate(16, 0, text, _text_sprites);
}

} //namespace game