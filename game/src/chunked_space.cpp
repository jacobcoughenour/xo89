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

int _get_marching_tile_index(
		int p_starting, // the top left index
		unsigned char p_neighbor_flags) {
	// need to shave off the first zeros
	auto f = p_neighbor_flags & 0b00001111;

	auto relative_index = _mask_to_tileset_index[f];
	auto relative_pos = _tile_index_to_pos(relative_index, 4);

	return _relative_tile_index(p_starting, relative_pos.x(), relative_pos.y(), chunked_space::TILESET_COLUMNS_X16);
}

chunked_space::chunked_space(bn::camera_ptr camera) :
		_camera(camera),
		_unloaded_chunks(),
		_loaded_chunks(),
		_rng(),
		_tilemap_item(_tilemap_cells[0], bn::size(TILEMAP_CELLS_SIZE, TILEMAP_CELLS_SIZE)) {
	for (int i = 0; i < MAX_CHUNKS; i++) {
		_unloaded_chunks.push_back(unloaded_chunk{
				.counts = {
						0,
						0,
						1,
						5,
						0,
						0,
				} });
	}

	// setup tilemap

	bn::bg_tiles::set_allow_offset(false);

	_tilemap_bg_item = bn::regular_bg_item(
			bn::regular_bg_tiles_items::tiles,
			bn::bg_palette_items::palette,
			this->_tilemap_item);
	_tilemap_bg = _tilemap_bg_item->create_bg(0, 0);

	bn::bg_tiles::set_allow_offset(true);

	_tilemap_bg->set_camera(_camera);

	// generate tiles
	for (int y = 0; y < SPACE_TILE_WIDTH; y++) {
		for (int x = 0; x < SPACE_TILE_WIDTH; x++) {
			// auto sample = stb_perlin_noise3(bn::fixed(x) / bn::fixed(12), bn::fixed(y) / bn::fixed(12), 0, SPACE_TILE_WIDTH / 8, SPACE_TILE_WIDTH, 1);

			unsigned char tile_id = 0;
			// if (sample > bn::fixed(-0.08)) {
			// 	tile_id = 1;
			// }
			// if (sample > bn::fixed(-0.04)) {
			// 	tile_id = 2;
			// }

			if (x % 16 == 0 && y % 16 == 0) {
				tile_id = 1;
			}

			_tile_cells[_tile_pos_to_index(x, y, SPACE_TILE_WIDTH)] = tile_id;
		}
	}

	_tile_cells[_tile_pos_to_index(128, 128, SPACE_TILE_WIDTH)] = 1;

	BN_ASSERT(_chunk_index_to_world_pos(0) == bn::point(0, 0));
	BN_ASSERT(_point_to_chunk_pos(bn::point(0, 0)) == bn::point(0, 0));
	BN_ASSERT(_point_to_chunk_pos(bn::point(CHUNK_SIZE, 0)) == bn::point(1, 0));

	BN_ASSERT(_is_point_in_view(bn::point(0, 0), bn::point(-120, 0), 0));
	BN_ASSERT(!_is_point_in_view(bn::point(0, 0), bn::point(-121, 0), 0));

	BN_ASSERT(_is_chunk_in_view(bn::point(0, 0), bn::point(0, 0)));

	BN_ASSERT(_relative_tile_index(1, 2, 2, 16) == 35);

	_update_tilemap();
}

chunked_space::~chunked_space() {
}

void chunked_space::_set_tile(int p_x, int p_y, int p_tile_id) {
	bn::regular_bg_map_cell &top_left = _tilemap_cells[_tilemap_item.cell_index(p_x * 2, p_y * 2)];
	bn::regular_bg_map_cell &top_right = _tilemap_cells[_tilemap_item.cell_index(p_x * 2 + 1, p_y * 2)];
	bn::regular_bg_map_cell &bottom_left = _tilemap_cells[_tilemap_item.cell_index(p_x * 2, p_y * 2 + 1)];
	bn::regular_bg_map_cell &bottom_right = _tilemap_cells[_tilemap_item.cell_index(p_x * 2 + 1, p_y * 2 + 1)];

	bn::regular_bg_map_cell_info top_left_info(top_left);
	bn::regular_bg_map_cell_info top_right_info(top_right);
	bn::regular_bg_map_cell_info bottom_left_info(bottom_left);
	bn::regular_bg_map_cell_info bottom_right_info(bottom_right);

	// how many 16x16 columns are there in the original asset
	const auto source_tileset_columns = 16;
	// gba only supports 8x8 internally
	const auto target_tileset_columns = source_tileset_columns * 2;

	const auto source_tile_pos = _tile_index_to_pos(p_tile_id, source_tileset_columns);

	// convert to 8x8 index meta tiles
	const auto target_top_left_index = _tile_pos_to_index(source_tile_pos.x() * 2, source_tile_pos.y() * 2, target_tileset_columns);
	const auto target_top_right_index = target_top_left_index + 1;
	const auto target_bottom_left_index = target_top_left_index + target_tileset_columns;
	const auto target_bottom_right_index = target_bottom_left_index + 1;

	top_left_info.set_tile_index(target_top_left_index);
	top_right_info.set_tile_index(target_top_right_index);
	bottom_left_info.set_tile_index(target_bottom_left_index);
	bottom_right_info.set_tile_index(target_bottom_right_index);

	top_left_info.set_palette_id(0);
	top_right_info.set_palette_id(0);
	bottom_left_info.set_palette_id(0);
	bottom_right_info.set_palette_id(0);

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
	return c != 0;
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
			point.x().floor_integer() / TILEMAP_LOAD_STRIDE_PX,
			point.y().floor_integer() / TILEMAP_LOAD_STRIDE_PX);
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

inline unsigned char chunked_space::_get_tile_at(int p_tile_x, int p_tile_y) {
	int index = _tile_pos_to_index(
			p_tile_x,
			p_tile_y,
			chunked_space::SPACE_TILE_WIDTH);

	if (index < 0 || index >= MAX_TILES) {
		return 35; // bedrock borders
	}
	return _tile_cells[index];
}

void chunked_space::_update_tilemap() {
	auto top_left_world_point = _tilemap_loaded_point * TILEMAP_LOAD_STRIDE_PX;
	top_left_world_point += bn::point(TILEMAP_CELLS_SIZE / 4, TILEMAP_SIZE / 4);

	_tilemap_bg->set_position(top_left_world_point + bn::point(TILEMAP_CELLS_SIZE / 4, TILEMAP_CELLS_SIZE / 4));

	auto top_left_tile_point = top_left_world_point / 16;

	// populate current tilemap
	for (int y = 0; y < TILEMAP_SIZE; y++) {
		for (int x = 0; x < TILEMAP_SIZE; x++) {
			auto px = x + top_left_tile_point.x();
			auto py = y + top_left_tile_point.y();

			auto tile = _get_tile_at(px, py);

			if (tile == 0) {
				_set_tile(x, y, 0);
				continue;
			}

			auto top = _get_tile_at(px, py - 1);
			auto right = _get_tile_at(px + 1, py);
			auto bottom = _get_tile_at(px, py + 1);
			auto left = _get_tile_at(px - 1, py);

			unsigned char flag = 0;
			if (top != 0) {
				flag |= marching_tile_flags::TOP;
			}
			if (right != 0) {
				flag |= marching_tile_flags::RIGHT;
			}
			if (bottom != 0) {
				flag |= marching_tile_flags::BOTTOM;
			}
			if (left != 0) {
				flag |= marching_tile_flags::LEFT;
			}

			_set_tile(x, y, _get_marching_tile_index(1, flag));
		}
	}

	bn::regular_bg_map_ptr map = _tilemap_bg->map();
	map.reload_cells_ref();
}

void chunked_space::update() {
	bn::fixed_point ship_pos = _camera.position();

	bn::point tilemap_pos = _point_to_tilemap_pos(ship_pos);

	if (tilemap_pos != _tilemap_loaded_point) {
		_tilemap_loaded_point = tilemap_pos;
		_update_tilemap();
	}

	chunk_point current_chunk_pos = _point_to_chunk_pos(ship_pos);

	// loaded chunk area around the ship
	int chunk_left = current_chunk_pos.x() - 1;
	int chunk_right = current_chunk_pos.x() + 1;
	int chunk_top = current_chunk_pos.y() - 1;
	int chunk_bottom = current_chunk_pos.y() + 1;

	for (int i = 0; i < _loaded_chunks.size(); i++) {
		loaded_chunk &chunk = _loaded_chunks[i];
		if (chunk.position.x() < chunk_left || chunk.position.x() > chunk_right || chunk.position.y() < chunk_top || chunk.position.y() > chunk_bottom) {
			int chunk_i = _chunk_pos_to_index(chunk.position.x(), chunk.position.y());

			// unload the chunk

			// count the current resources in the chunk to serialize them
			auto unloaded = unloaded_chunk{
				.counts = { 0, 0, 0, 0, 0, 0 }
			};
			for (int j = 0; j < chunk.objects.size(); j++) {
				floating_object &obj = chunk.objects.at(j);
				unloaded.counts[static_cast<unsigned long>(obj.object_type)]++;
			}
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

			_rng.set_seed(chunk_index);

			for (int j = 0; j < static_cast<unsigned char>(obj_type::OBJ_TYPE_MAX); j++) {
				for (int k = 0; k < data.counts[j]; k++) {
					auto obj = floating_object{
						.object_type = static_cast<obj_type>(j),
						.velocity = bn::fixed_point(0, 0),
						.position = chunk_origin + bn::fixed_point(_rng.get_fixed() % CHUNK_SIZE, _rng.get_fixed() % CHUNK_SIZE)
					};
					chunk.objects.push_back(obj);
				}
			}

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

			bn::fixed dist = helpers::max_box_dist(obj.position, ship_pos);

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

			bn::fixed len = helpers::distance(ship_pos, obj.position);
			bn::fixed_point dir = helpers::normalize_point(ship_pos - obj.position);

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
