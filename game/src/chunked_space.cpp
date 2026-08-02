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

chunked_space::chunked_space(bn::camera_ptr camera) :
		_camera(camera),
		_unloaded_chunks(),
		_loaded_chunks(),
		_rng(),
		_tilemap_item(_tilemap_cells[0], bn::size(TILEMAP_CELLS_SIZE, TILEMAP_CELLS_SIZE)) {
	// setup tilemap

	bn::bg_tiles::set_allow_offset(false);

	_tilemap_bg_item = bn::regular_bg_item(
			bn::regular_bg_tiles_items::tiles,
			bn::bg_palette_items::palette,
			this->_tilemap_item);
	_tilemap_bg = _tilemap_bg_item->create_bg(0, 0);

	bn::bg_tiles::set_allow_offset(true);

	_tilemap_bg->set_camera(_camera);

	BN_ASSERT(_chunk_index_to_world_pos(0) == bn::point(0, 0));
	BN_ASSERT(_point_to_chunk_pos(bn::point(0, 0)) == bn::point(0, 0));
	BN_ASSERT(_point_to_chunk_pos(bn::point(CHUNK_SIZE, 0)) == bn::point(1, 0));

	BN_ASSERT(_is_point_in_view(bn::point(0, 0), bn::point(-120, 0), 0));
	BN_ASSERT(!_is_point_in_view(bn::point(0, 0), bn::point(-121, 0), 0));

	BN_ASSERT(_is_chunk_in_view(bn::point(0, 0), bn::point(0, 0)));

	BN_ASSERT(_relative_tile_index(1, 2, 2, 16) == 35);
}

chunked_space::~chunked_space() {
}

void chunked_space::generate_next_chunk() {
	BN_ASSERT(chunks_generated < MAX_CHUNKS);

	int start_x = (chunks_generated % SPACE_SIZE) * CHUNK_TILE_WIDTH;
	int start_y = (chunks_generated / SPACE_SIZE) * CHUNK_TILE_WIDTH;
	int to_x = start_x + CHUNK_TILE_WIDTH;
	int to_y = start_y + CHUNK_TILE_WIDTH;

	// generate tiles
	for (int y = start_y; y < to_y; y++) {
		for (int x = start_x; x < to_x; x++) {
			tile_data data{};

			auto index = _tile_pos_to_index(x, y, SPACE_TILE_WIDTH);

			if (x > 120 && x < 136 && y > 120 && y < 136) {
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

	// chunks_generated = MAX_CHUNKS;
	chunks_generated++;
}

int chunked_space::generated_chunks_count() {
	return chunks_generated;
}

bool chunked_space::is_generated() {
	return chunks_generated == MAX_CHUNKS;
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
				auto rand_offset = _rng.get_int(p_material == tile_material::ROCK ? 2 : 3);
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
	return bn::fixed_point(SPACE_SIZE * CHUNK_SIZE / 2, SPACE_SIZE * CHUNK_SIZE / 2);
}

bn::point chunked_space::space_point_to_tile_point(bn::fixed_point p_pos) {
	return bn::point(
			p_pos.x().floor_integer() / TILE_SIZE_PX,
			p_pos.y().floor_integer() / TILE_SIZE_PX);
}

bool chunked_space::is_solid_tile(bn::point p_pos) {
	auto c = _get_tile_at(p_pos.x(), p_pos.y());
	return c.material != tile_material::AIR;
}

bool chunked_space::can_mine_tile(bn::point p_pos) {
	auto c = _get_tile_at(p_pos.x(), p_pos.y());
	// todo
	return c.material != tile_material::AIR || c.material != tile_material::BEDROCK;
}

void chunked_space::spawn_floating_object(obj_type p_type, bn::fixed_point p_position, bn::fixed_point p_velocity) {
	// get chunk it should go in

	// determine if loaded or unloaded chunk

	// add it

	// need to create sprite?
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

	// auto update_lighting =
	// (d.material == tile_material::AIR) != (p_tile_material == tile_material::AIR);
	// if (d.material == tile_material::AIR) {

	// }

	_tile_cells[index] = pack_tile_data(d);

	_update_tilemap();
}

void chunked_space::mine_tile(bn::point p_tile_point) {
	if (!can_mine_tile(p_tile_point)) {
		return;
	}

	auto tile = get_tile(p_tile_point);

	// todo spawn items

	set_tile_material(p_tile_point, tile_material::AIR);
}

bn::point chunked_space::_chunk_index_to_world_pos(int i) {
	return bn::point((i % SPACE_SIZE) * CHUNK_SIZE, (i / SPACE_SIZE) * CHUNK_SIZE);
}

bn::fixed_point chunked_space::_chunk_pos_to_world_pos(chunk_point chunk_pos) {
	return bn::fixed_point((chunk_pos.x()) * CHUNK_SIZE, (chunk_pos.y()) * CHUNK_SIZE);
}

int chunked_space::_chunk_pos_to_index(int cx, int cy) {
	return cx % SPACE_SIZE + cy * SPACE_SIZE;
}

chunk_point chunked_space::_point_to_chunk_pos(bn::fixed_point point) {
	return chunk_point(
			point.x().floor_integer() / CHUNK_SIZE,
			point.y().floor_integer() / CHUNK_SIZE);
}

bn::point chunked_space::_point_to_tilemap_pos(bn::fixed_point point) {
	return bn::point(
			(point.x().floor_integer() + 8) / TILEMAP_LOAD_STRIDE_PX,
			(point.y().floor_integer() + 8) / TILEMAP_LOAD_STRIDE_PX);
}

bool chunked_space::_is_chunk_in_view(bn::fixed_point camera_pos, chunk_point chunk_pos) {
	bn::fixed left = camera_pos.x() - 120;
	bn::fixed right = camera_pos.x() + 120;
	bn::fixed top = camera_pos.y() - 80;
	bn::fixed bottom = camera_pos.y() + 80;

	bn::fixed_point pos = _chunk_pos_to_world_pos(chunk_pos);

	if (pos.x() + CHUNK_SIZE < left) {
		return false;
	}
	if (pos.x() > right) {
		return false;
	}
	if (pos.y() + CHUNK_SIZE < top) {
		return false;
	}
	if (pos.y() > bottom) {
		return false;
	}

	return true;
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
	int index = _tile_pos_to_index(
			p_tile_x,
			p_tile_y,
			chunked_space::SPACE_TILE_WIDTH);

	if (index < 0 || index >= MAX_TILES) {
		return tile_data{
			tile_material::BEDROCK,
			0
		};
	}
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

void chunked_space::update() {
	bn::fixed_point camera_pos = _camera.position();

	bn::point tilemap_pos = _point_to_tilemap_pos(camera_pos);

	if (tilemap_pos != _tilemap_loaded_point) {
		_tilemap_loaded_point = tilemap_pos;
		_update_tilemap();
	}

	chunk_point current_chunk_pos = _point_to_chunk_pos(camera_pos);

	// loaded chunk area around the ship
	int chunk_left = current_chunk_pos.x() - 1;
	int chunk_right = current_chunk_pos.x() + 1;
	int chunk_top = current_chunk_pos.y() - 1;
	int chunk_bottom = current_chunk_pos.y() + 1;

	return;

	for (int i = 0; i < _loaded_chunks.size(); i++) {
		loaded_chunk &chunk = _loaded_chunks[i];
		if (chunk.position.x() < chunk_left || chunk.position.x() > chunk_right || chunk.position.y() < chunk_top || chunk.position.y() > chunk_bottom) {
			int chunk_i = _chunk_pos_to_index(chunk.position.x(), chunk.position.y());

			// unload the chunk

			// count the current resources in the chunk to serialize them
			auto unloaded = unloaded_chunk{
				// .counts = { 0, 0, 0, 0, 0, 0 }
			};
			// for (int j = 0; j < chunk.objects.size(); j++) {
			// 	floating_object &obj = chunk.objects.at(j);
			// 	unloaded.counts[static_cast<unsigned long>(obj.object_type)]++;
			// }
			_unloaded_chunks[chunk_i] = unloaded;

			_loaded_chunks.erase(&chunk);
			// modified the list we are iterating so we need to account for that
			i--;
			continue;
		}
	}

	for (int i = 0; i < MAX_LOADED_CHUNKS; i++) {
		bn::point dir = _chunk_neighbors[i];
		chunk_point chunk_pos = current_chunk_pos + dir;
		int chunk_index = _chunk_pos_to_index(chunk_pos.x(), chunk_pos.y());
		if (chunk_index < 0 || chunk_index >= MAX_CHUNKS) {
			continue;
		}

		// see if it is loaded
		// should we use a map instead?
		bool found = false;
		for (int j = 0; j < _loaded_chunks.size(); j++) {
			if (_loaded_chunks[j].position == chunk_pos) {
				found = true;
				break;
			}
		}

		if (!found) {
			// load in the chunk

			auto chunk =
					loaded_chunk{
						.position = chunk_pos,
						.objects = bn::vector<floating_object, MAX_OBJS_PER_CHUNK>()
					};

			auto data = _unloaded_chunks.at(chunk_index);
			auto chunk_origin = _chunk_index_to_world_pos(chunk_index);

			// _rng.set_seed(chunk_index);

			// for (int j = 0; j < static_cast<unsigned char>(obj_type::OBJ_TYPE_MAX); j++) {
			// 	for (int k = 0; k < data.counts[j]; k++) {
			// 		auto obj = floating_object{
			// 			.object_type = static_cast<obj_type>(j),
			// 			.velocity = bn::fixed_point(0, 0),
			// 			.position = chunk_origin + bn::fixed_point(_rng.get_fixed() % CHUNK_SIZE, _rng.get_fixed() % CHUNK_SIZE)
			// 		};
			// 		chunk.objects.push_back(obj);
			// 	}
			// }

			_loaded_chunks.push_back(chunk);
		}
	}

	// go through all the loaded chunks and move the objects by their current
	// velocity

	for (int i = 0; i < _loaded_chunks.size(); i++) {
		loaded_chunk &c = _loaded_chunks.at(i);

		if (!_is_chunk_in_view(_camera.position(), c.position)) {
			continue;
		}

		for (int j = 0; j < c.objects.size(); j++) {
			floating_object &obj = c.objects.at(j);

			bn::fixed dist = helpers::max_box_dist(obj.position, camera_pos);

			if (dist < 2) {
				// pickup
				c.objects.erase(&obj);
				// modified the list we are iterating so we need to account for that
				j--;
				continue;
			}

			// attractor influence distance
			constexpr int d = 48;

			if (dist > d) {
				continue;
			}

			bn::fixed len = helpers::distance(camera_pos, obj.position);
			bn::fixed_point dir = helpers::normalize_point(camera_pos - obj.position);

			obj.velocity += dir * bn::min(bn::fixed(10), bn::max(d - len, bn::fixed(0.2))) * bn::fixed(0.03);
			obj.position += obj.velocity;
			// velocity damping
			obj.velocity *= bn::fixed(0.98);
		}
	}

	// render the objects

	int sprite_index = 0;

	for (int i = 0; i < _loaded_chunks.size(); i++) {
		if (sprite_index >= MAX_VISIBLE_OBJS) {
			break;
		}
		loaded_chunk &c = _loaded_chunks.at(i);
		if (!_is_chunk_in_view(_camera.position(), c.position)) {
			continue;
		}
		for (int j = 0; j < c.objects.size(); j++) {
			if (sprite_index >= MAX_VISIBLE_OBJS) {
				break;
			}

			floating_object &obj = c.objects.at(j);
			if (!_is_point_in_view(_camera.position(), obj.position, 8)) {
				continue;
			}

			BN_ASSERT(sprite_index <= _obj_sprites.size());

			if (sprite_index == _obj_sprites.size()) {
				_obj_sprites.push_back(bn::sprite_items::dev8.create_sprite());
			}
			bn::sprite_ptr &existing = _obj_sprites.at(sprite_index);
			existing.set_position(obj.position);
			existing.set_visible(true);
			existing.set_camera(_camera);
			sprite_index++;
		}
	}

	// hide unused
	for (; sprite_index < _obj_sprites.size(); sprite_index++) {
		_obj_sprites.at(sprite_index).set_visible(false);
	}
}

} //namespace game
