#include "chunked_space.h"

#include "stb_perlin.h"

namespace game {

inline bn::point _tile_index_to_pos(int p_index, int p_columns) {
	return bn::point(p_index % p_columns, p_index / p_columns);
}

inline int _tile_pos_to_index(int p_x, int p_y, int p_columns) {
	return p_x + p_y * p_columns;
}

inline int _relative_tile_index(int p_from_index, int p_x, int p_y, int p_tileset_columns) {
	return p_from_index + p_x + p_y * p_tileset_columns;
}

chunked_space::chunked_space(game_state &state, bn::camera_ptr camera) :
		_state(state),
		_tilemap_item(_tilemap_cells[0], bn::size(TILEMAP_CELLS_SIZE, TILEMAP_CELLS_SIZE)),
		_camera(camera),
		_rng(),
		_objects(),
		_obj_sprites() {
	// setup tilemap

	bn::bg_tiles::set_allow_offset(false);

	_tilemap_bg_item = bn::regular_bg_item(
			bn::regular_bg_tiles_items::tiles,
			bn::bg_palette_items::palette,
			this->_tilemap_item);

	set_visible(true);

	bn::bg_tiles::set_allow_offset(true);

	BN_ASSERT(_is_point_in_view(bn::point(0, 0), bn::point(-120, 0), 0));
	BN_ASSERT(!_is_point_in_view(bn::point(0, 0), bn::point(-121, 0), 0));

	BN_ASSERT(_relative_tile_index(1, 2, 2, 16) == 35);

	BN_ASSERT(space_point_to_tile_point(bn::fixed_point(0.4, 2)) == bn::point(0, 0), space_point_to_tile_point(bn::fixed_point(0.4, 2)).x());

	BN_ASSERT(space_point_to_tile_point(bn::fixed_point(-0.4, -2)) == bn::point(-1, -1), space_point_to_tile_point(bn::fixed_point(-0.4, -2)).x());
}

chunked_space::~chunked_space() {
}

void chunked_space::generate_next_chunk() {
	BN_ASSERT(_chunks_generated < MAX_CHUNKS);

	int start_x = (_chunks_generated % SPACE_SIZE) * CHUNK_TILE_WIDTH;
	int start_y = (_chunks_generated / SPACE_SIZE) * CHUNK_TILE_WIDTH;
	int to_x = start_x + CHUNK_TILE_WIDTH;
	int to_y = start_y + CHUNK_TILE_WIDTH;

	auto spawn_point = this->spawn_point();
	spawn_point.set_y(0);
	spawn_point.set_x(spawn_point.x().integer() / 16);
	auto spawn_area = 4;

	// generate tiles
	for (int y = start_y; y < to_y; y++) {
		for (int x = start_x; x < to_x; x++) {
			tile_data data{};

			auto index = _tile_pos_to_index(x, y, SPACE_TILE_WIDTH);

			if (x > spawn_point.x() - spawn_area //
					&& x < spawn_point.x() + spawn_area //
					&& y > spawn_point.y() - spawn_area //
					&& y < spawn_point.y() + spawn_area) {
				data.material = tile_material::AIR;
			} else {
				auto sample = stb_perlin_noise3(bn::fixed(x) / bn::fixed(8), bn::fixed(y) / bn::fixed(8), 0, SPACE_TILE_WIDTH / 8, SPACE_TILE_WIDTH, 1);
				if (sample > bn::fixed(-0.08)) {
					data.material = tile_material::ROCK;
				}
				if (sample > bn::fixed(0.2)) {
					// dense enough to spawn an ore

					_rng.set_seed(index);
					auto d = _rng.get() % 64;
					if (d > 42) {
						data.material = tile_material::IRON;
					}
					if (d > 60) {
						data.material = tile_material::COBALT;
					}
				}
			}

			_tile_cells[index] = pack_tile_data(data);
		}
	}

	_chunks_generated++;
}

int chunked_space::generated_chunks_count() {
	return _chunks_generated;
}

bool chunked_space::is_generated() {
	return _chunks_generated == MAX_CHUNKS;
}

inline bool _has_flags(unsigned short value, unsigned short mask) {
	return (value & mask) == mask;
}

void chunked_space::_set_tilemap_tile(int seed, int p_x, int p_y, int p_edge_mask, tile_material p_material) {
	const int tileset_columns = 32;

	const int solid_offsets[3] = { tileset_columns, tileset_columns + 1, 0 };

	int corner_ids[4] = { 0, 0, 0, 0 };
	bool corner_flip_v[4] = { false, false, false, false };
	bool corner_flip_h[4] = { false, false, false, false };

	if (p_material == tile_material::AIR) {
		// leave the index on 0
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
				} else if (!bottom && right) {
					corner_ids[3] = tileset_columns + 3;
				} else {
					corner_ids[3] = tileset_columns + 4;
				}
			}
		}

		for (size_t i = 0; i < 4; i++) {
			auto s = corner_ids[i];

			if (s == 0 || s == 1) {
				corner_flip_v[i] = _rng.get_bool();
			}
			if (s == 0 || s % tileset_columns == 3) {
				corner_flip_h[i] = _rng.get_bool();
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
		top_left_info.set_palette_id(1);
		top_right_info.set_palette_id(1);
		bottom_left_info.set_palette_id(1);
		bottom_right_info.set_palette_id(1);
	} else {
		top_left_info.set_palette_id(0);
		top_right_info.set_palette_id(0);
		bottom_left_info.set_palette_id(0);
		bottom_right_info.set_palette_id(0);
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

bn::fixed_point chunked_space::spawn_point() {
	return bn::fixed_point(SPACE_SIZE * CHUNK_SIZE / 2 + 8, 0);
}

bn::point chunked_space::space_point_to_tile_point(bn::fixed_point p_pos) {
	return bn::point(
			p_pos.x() < 0
					? -((-p_pos.x().floor_integer()) / TILE_SIZE_PX) - 1
					: (p_pos.x().floor_integer() / TILE_SIZE_PX),
			p_pos.y() < 0
					? -((-p_pos.y().floor_integer()) / TILE_SIZE_PX) - 1
					: (p_pos.y().floor_integer() / TILE_SIZE_PX));
}

bool chunked_space::is_solid_tile(bn::point p_pos) {
	auto c = _get_tile_at(p_pos.x(), p_pos.y());
	return c.material != tile_material::AIR;
}

bool chunked_space::can_mine_tile(bn::point p_pos) {
	auto c = _get_tile_at(p_pos.x(), p_pos.y());
	// todo
	return c.material != tile_material::AIR && c.material != tile_material::BEDROCK;
}

void chunked_space::spawn_floating_object(obj_type p_type, bn::fixed_point p_position, bn::fixed_point p_velocity) {
	if (_objects.full()) {
		// make room
		_objects.pop_back();
	}

	// todo you can do better (lut?)
	unsigned char sprite_index = static_cast<unsigned char>(p_type) + 1;
	if (sprite_index == 1 && _rng.get_bool()) {
		sprite_index = 0;
	}

	floating_object obj{
		p_type,
		sprite_index,
		p_position,
		p_velocity,
	};

	_objects.push_front(obj);
}

bn::optional<chunked_space::raycast_hit> chunked_space::raycast(bn::fixed_point p_origin, bn::fixed_point p_dir, bn::fixed p_max_distance) {
	bn::optional<chunked_space::raycast_hit> hit;

	auto step = helpers::set_length(p_dir, 8.0);
	auto steps = p_max_distance / 8.0;

	for (bn::fixed i = 0; i < steps; i += 1) {
		auto tile_pos = space_point_to_tile_point(p_origin);
		if (is_solid_tile(tile_pos)) {
			hit = raycast_hit{
				tile_pos,
				// todo we need to figure out the actual intersection point
				bn::fixed_point(tile_pos * 16) + bn::point(8, 8),
			};
			return hit;
		}
		p_origin += step;
	}

	return hit;
}

packed_tile_data chunked_space::pack_tile_data(tile_data p_data) {
	packed_tile_data data;

	// first 6 bits for material 0..64
	// next 2 for light level 0..4

	data = static_cast<unsigned char>(p_data.material);
	BN_ASSERT(data < 64);
	BN_ASSERT(p_data.light_level < 4);
	data = data | p_data.light_level << 6;
	return data;
}

tile_data chunked_space::unpack_tile_data(packed_tile_data p_data) {
	tile_data data;
	data.material = static_cast<tile_material>(p_data & 0b00111111);
	data.light_level = (p_data & 0b11000000) >> 6;
	return data;
}

tile_data chunked_space::get_tile(bn::point p_tile_point) {
	return _get_tile_at(p_tile_point.x(), p_tile_point.y());
}

void chunked_space::set_tile_material(bn::point p_tile_point, tile_material p_tile_material) {
	int index = _tile_pos_to_index(
			p_tile_point.x(),
			p_tile_point.y(),
			chunked_space::SPACE_TILE_WIDTH);

	if (index < 0 || index >= MAX_TILES) {
		return; // bedrock borders
	}

	auto d = unpack_tile_data(_tile_cells[index]);

	if (d.material == p_tile_material) {
		return;
	}

	d.material = p_tile_material;

	_tile_cells[index] = pack_tile_data(d);

	_update_tilemap();
}

void chunked_space::mine_tile(bn::point p_tile_point) {
	if (!can_mine_tile(p_tile_point)) {
		return;
	}

	auto tile = get_tile(p_tile_point);
	set_tile_material(p_tile_point, tile_material::AIR);

	int drop_amount = 0;
	obj_type drop_type = static_cast<obj_type>(static_cast<unsigned char>(tile.material) - static_cast<unsigned char>(tile_material::ROCK));

	if (drop_type == obj_type::ROCK) {
		drop_amount = 1;
	} else {
		drop_amount = _rng.get_bool() ? 1 : 2;
	}

	for (int i = 0; i < drop_amount; i++) {
		bn::fixed_point position(
				p_tile_point.x() * 16 + 2 + _rng.get_fixed() % 4,
				p_tile_point.y() * 16 + 2 + _rng.get_fixed() % 4);
		bn::fixed_point velocity(_rng.get_fixed() % 4 - 2, _rng.get_fixed() % 4 - 2);

		spawn_floating_object(drop_type, position, velocity);
	}
}

bn::point chunked_space::_point_to_tilemap_pos(bn::fixed_point point) {
	return bn::point(
			(point.x().floor_integer() + 8) / TILEMAP_LOAD_STRIDE_PX,
			(point.y().floor_integer() + 8) / TILEMAP_LOAD_STRIDE_PX);
}

bool chunked_space::_is_point_in_view(bn::fixed_point camera_pos, bn::fixed_point point, bn::fixed size) {
	bn::fixed left = camera_pos.x() - 120;
	bn::fixed right = camera_pos.x() + 120;
	bn::fixed top = camera_pos.y() - 80;
	bn::fixed bottom = camera_pos.y() + 80;

	if (point.x() + size < left) {
		return false;
	}
	if (point.x() > right) {
		return false;
	}
	if (point.y() + size < top) {
		return false;
	}
	if (point.y() > bottom) {
		return false;
	}
	return true;
}

inline tile_data chunked_space::_get_tile_at(int p_tile_x, int p_tile_y) {
	if (p_tile_x < 0 || p_tile_y < 0 || p_tile_x >= SPACE_TILE_WIDTH || p_tile_y >= SPACE_TILE_WIDTH) {
		const int middle = SPACE_SIZE * CHUNK_SIZE / 2 / 16;

		if (p_tile_y < 0 && p_tile_x > middle - 4 && p_tile_x < middle + 4) {
			return tile_data{
				tile_material::AIR,
				0
			};
		}

		return tile_data{
			tile_material::BEDROCK,
			0
		};
	}

	int index = _tile_pos_to_index(
			p_tile_x,
			p_tile_y,
			chunked_space::SPACE_TILE_WIDTH);

	return unpack_tile_data(_tile_cells[index]);
}

void chunked_space::_update_tilemap() {
	_tilemap_bg->set_position(
			_tilemap_loaded_point * TILEMAP_LOAD_STRIDE_PX);
	auto top_left_world_point = _tilemap_loaded_point * TILEMAP_LOAD_STRIDE_PX;
	top_left_world_point -= bn::point(128, 128);
	auto top_left_tile_point = top_left_world_point / 16;

	// populate current tilemap
	for (int y = 0; y < TILEMAP_SIZE; y++) {
		for (int x = 0; x < TILEMAP_SIZE; x++) {
			auto px = top_left_tile_point.x() + x;
			auto py = top_left_tile_point.y() + y;

			auto tile = _get_tile_at(px, py);

			auto seed = _tile_pos_to_index(
					px,
					py,
					chunked_space::SPACE_TILE_WIDTH);

			if (tile.material == tile_material::AIR) {
				_set_tilemap_tile(seed, x, y, 0, tile.material);
				continue;
			}

			auto top = _get_tile_at(px, py - 1);
			auto right = _get_tile_at(px + 1, py);
			auto bottom = _get_tile_at(px, py + 1);
			auto left = _get_tile_at(px - 1, py);

			auto top_left = _get_tile_at(px - 1, py - 1);
			auto top_right = _get_tile_at(px + 1, py - 1);
			auto bottom_left = _get_tile_at(px - 1, py + 1);
			auto bottom_right = _get_tile_at(px + 1, py + 1);

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

			_set_tilemap_tile(seed, x, y, flag, tile.material);
		}
	}

	bn::regular_bg_map_ptr map = _tilemap_bg->map();
	map.reload_cells_ref();
}

void chunked_space::set_visible(bool p_visible) {
	if (p_visible) {
		_tilemap_bg = _tilemap_bg_item->create_bg(0, 0);
		_tilemap_bg->set_priority(2);
		_tilemap_bg->set_camera(_camera);

		_tilemap_loaded_point = _point_to_tilemap_pos(_camera.position());
		_update_tilemap();

	} else {
		_tilemap_bg.reset();
	}

	if (!p_visible) {
		for (int i = 0; i < _obj_sprites.size(); i++) {
			_obj_sprites.at(i).set_visible(false);
		}
	}
}

void chunked_space::update() {
	bn::fixed_point camera_pos = _camera.position();

	bn::point tilemap_pos = _point_to_tilemap_pos(camera_pos);

	if (tilemap_pos != _tilemap_loaded_point) {
		_tilemap_loaded_point = tilemap_pos;
		_update_tilemap();
	}

	// go through all the loaded chunks and move the objects by their current
	// velocity

	for (auto it = _objects.begin(); it != _objects.end(); ++it) {
		auto &obj = *it;

		bn::fixed dist = helpers::max_box_dist(obj.position, camera_pos);

		if (dist < 2) {
			_state.pickup_resource(obj.object_type, 1);
			_objects.erase(it);
			continue;
		}

		// assumes the ship is in the center of the screen...
		bn::fixed len = helpers::distance(camera_pos, obj.position);
		bn::fixed_point dir = helpers::normalize_point(camera_pos - obj.position);
		// attractor influence distance
		constexpr int d = 40;
		// push object towards ship
		obj.velocity += dir * bn::min(bn::fixed(10), bn::max(d - len, bn::fixed(0.2))) * bn::fixed(0.03);

		// move by velocity
		auto desired_pos = obj.position + obj.velocity;

		if (is_solid_tile(space_point_to_tile_point(desired_pos))) {
			// bounce
			obj.velocity *= bn::fixed(-0.9);
		} else {
			obj.position = desired_pos;
			// velocity damping
			obj.velocity *= bn::fixed(0.98);
			// gravity
			obj.velocity += bn::fixed_point(0.0, 0.08);
		}
	}

	// render the objects

	int sprite_index = 0;
	int sprite_flicker_index = -1;

	int flicker_frame = _obj_flicker_frame / 10;

	for (auto obj : _objects) {
		if (sprite_index >= MAX_VISIBLE_OBJS) {
			break;
		}

		sprite_flicker_index++;

		if (!_is_point_in_view(_camera.position(), obj.position, 8)) {
			continue;
		}

		BN_ASSERT(sprite_index <= _obj_sprites.size());

		if (sprite_flicker_index >= MAX_VISIBLE_OBJS / 2 && flicker_frame % 2 == sprite_flicker_index % 2) {
			continue;
		}

		if (sprite_index == _obj_sprites.size()) {
			// add sprite
			_obj_sprites.push_back(bn::sprite_items::dropped_items.create_sprite());
		}
		bn::sprite_ptr &existing = _obj_sprites.at(sprite_index);
		existing.set_tiles(bn::sprite_items::dropped_items.tiles_item()
						.create_tiles(obj.sprite_index));
		existing.set_position(obj.position);
		existing.set_visible(true);
		existing.set_camera(_camera);
		sprite_index++;
	}

	_obj_flicker_frame = (_obj_flicker_frame + 1) % 60;

	// hide unused
	for (; sprite_index < _obj_sprites.size(); sprite_index++) {
		_obj_sprites.at(sprite_index).set_visible(false);
	}
}

} //namespace game
