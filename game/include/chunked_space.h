#pragma once

#include "helpers.h"

#include "bn_bg_palette_item.h"
#include "bn_bg_tiles.h"
#include "bn_camera_ptr.h"
#include "bn_fixed.h"
#include "bn_fixed_point.h"
#include "bn_list.h"
#include "bn_point.h"
#include "bn_rect.h"
#include "bn_regular_bg_item.h"
#include "bn_regular_bg_map_cell_info.h"
#include "bn_regular_bg_map_item.h"
#include "bn_regular_bg_map_ptr.h"
#include "bn_regular_bg_ptr.h"
#include "bn_seed_random.h"
#include "bn_sprite_ptr.h"
#include "bn_unordered_map.h"
#include "bn_vector.h"

// assets
#include "bn_bg_palette_items_palette.h"
#include "bn_regular_bg_tiles_items_tiles.h"
#include "bn_sprite_items_dev8.h"

namespace game {

using chunk_point = bn::point;

enum class obj_type : unsigned char {
	TRASH,
	TECH,
	// https://en.wikipedia.org/wiki/Asteroid_mining#:~:text=crust%20with%20metals%20like%20gold%2C%20cobalt%2C%20iron%2C%20manganese%2C%20molybdenum%2C%20nickel%2C%20osmium%2C%20palladium%2C%20platinum%2C%20rhenium%2C%20rhodium%2C%20ruthenium%20and%20tungsten
	IRON,
	COBALT,
	GOLD,
	NICKEL,
	OBJ_TYPE_MAX
};

enum marching_tile_flags : unsigned char {
	TOP = 1 << 0, //          0000 0001
	RIGHT = 1 << 1, //        0000 0010
	BOTTOM = 1 << 2, //       0000 0100
	LEFT = 1 << 3, //         0000 1000
	// unused
	// TOP_LEFT = 1 << 4, //     0001 0000
	// TOP_RIGHT = 1 << 5, //    0010 0000
	// BOTTOM_RIGHT = 1 << 6, // 0100 0000
	// BOTTOM_LEFT = 1 << 7, //  1000 0000
};

// this corresponds to the tileset
enum marching_tile_masks : unsigned char {
	NONE = 0b0000, // 0 -> 0
	LEFT_KNOB = 0b0010, // 1 -> 2
	HORIZONTAL = 0b1010, // 2 -> 10
	RIGHT_KNOB = 0b1000, // 3 -> 8
	UP_KNOB = 0b0100, // 4 -> 4
	TOP_LEFT_CORNER = 0b0110, // 5 -> 6
	TOP_EDGE = 0b1110, // 6 -> 14
	TOP_RIGHT_CORNER = 0b1100, // 7 -> 12
	VERTICAL = 0b0101, // 8 -> 5
	LEFT_EDGE = 0b0111, // 9 -> 7
	CENTER = 0b1111, // 10 -> 15
	RIGHT_EDGE = 0b1101, // 11 -> 13
	BOTTOM_KNOB = 0b0001, // 12 -> 1
	BOTTOM_LEFT_EDGE = 0b0011, // 13 -> 3
	BOTTOM_EDGE = 0b1011, // 14 -> 11
	BOTTOM_RIGHT_CORNER = 0b1001, // 15 -> 9
};

const unsigned char _mask_to_tileset_index[16] = {
	0,
	12,
	1,
	13,
	4,
	8,
	5,
	9,
	3,
	15,
	2,
	14,
	7,
	11,
	6,
	10
};

class chunked_space {
public:
	static const int MAX_OBJS_PER_CHUNK = 32;
	static const int MAX_UNIQUE_OBJS_PER_CHUNK = 8;
	static const int CHUNK_SIZE = 128;
	static const int SPACE_SIZE = 32;
	static const int MAX_CHUNKS = SPACE_SIZE * SPACE_SIZE;
	static const int MAX_VISIBLE_OBJS = 64;
	static const int MAX_LOADED_CHUNKS = 9;

	static const int CHUNK_TILE_WIDTH = CHUNK_SIZE / 16;
	static const int SPACE_TILE_WIDTH = SPACE_SIZE * CHUNK_TILE_WIDTH;
	static const int TILES_PER_CHUNK = CHUNK_TILE_WIDTH * CHUNK_TILE_WIDTH;
	static const int MAX_TILES = TILES_PER_CHUNK * MAX_CHUNKS;

	// we want the tilemap to span 2x2 chunks
	static const int TILEMAP_SIZE = CHUNK_TILE_WIDTH * 2;
	static const int TILEMAP_CELLS_SIZE = TILEMAP_SIZE * 2;
	static const int TILEMAP_MAX_CELLS = TILEMAP_CELLS_SIZE * TILEMAP_CELLS_SIZE;
	// how many pixels does the camera travel to cause the tilemap to reload

	static const int TILEMAP_LOAD_STRIDE_PX = 16;

	static const int TILESET_COLUMNS_X16 = 16;
	static const int TILESET_COLUMNS_X8 = TILESET_COLUMNS_X16 * TILESET_COLUMNS_X16;

	const bn::point _chunk_neighbors[MAX_LOADED_CHUNKS] = {
		bn::point(0, 0),
		bn::point(-1, 0),
		bn::point(1, 0),
		bn::point(1, -1),
		bn::point(1, 1),
		bn::point(-1, -1),
		bn::point(1, -1),
		bn::point(-1, 1),
		bn::point(1, 1),
	};

	struct floating_object {
		obj_type object_type;
		bn::fixed_point velocity;
		bn::fixed_point position;
	};

	struct loaded_chunk {
		chunk_point position;
		bn::vector<floating_object, MAX_OBJS_PER_CHUNK> objects;
	};

	struct unloaded_chunk {
		unsigned char counts[static_cast<unsigned long>(obj_type::OBJ_TYPE_MAX)];
	};

	chunked_space(bn::camera_ptr camera);
	~chunked_space();

	void update();

	bn::fixed_point spawn_point();

private:
	alignas(unsigned char) volatile unsigned char _tile_cells[MAX_TILES];
	alignas(int) bn::regular_bg_map_cell _tilemap_cells[TILEMAP_MAX_CELLS];
	bn::regular_bg_map_item _tilemap_item;
	bn::optional<bn::regular_bg_item> _tilemap_bg_item;
	bn::optional<bn::regular_bg_ptr> _tilemap_bg;
	bn::point _tilemap_loaded_point;

	bn::camera_ptr _camera;
	bn::vector<unloaded_chunk, MAX_CHUNKS> _unloaded_chunks;
	bn::vector<loaded_chunk, MAX_LOADED_CHUNKS> _loaded_chunks;
	bn::seed_random _rng;
	bn::vector<bn::sprite_ptr, MAX_VISIBLE_OBJS> _obj_sprites;

	bn::point _point_to_tilemap_pos(bn::fixed_point p_pos);
	void _set_tile(int p_x, int p_y, int p_tile_id);
	void _update_tilemap();
	unsigned char _get_tile_at(int p_tile_x, int p_tile_y);

	bn::point _chunk_index_to_world_pos(int i);
	int _chunk_pos_to_index(int cx, int cy);
	chunk_point _point_to_chunk_pos(bn::fixed_point point);
	bn::fixed_point _chunk_pos_to_world_pos(chunk_point chunk_pos);
	bool _is_chunk_in_view(bn::fixed_point camera_pos, chunk_point chunk_pos);
	bool _is_point_in_view(bn::fixed_point camera_pos, bn::fixed_point point, bn::fixed size);
};

} //namespace game