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

using packed_tile_data = unsigned char;

enum class obj_type : unsigned char {
	// https://en.wikipedia.org/wiki/Asteroid_mining#:~:text=crust%20with%20metals%20like%20gold%2C%20cobalt%2C%20iron%2C%20manganese%2C%20molybdenum%2C%20nickel%2C%20osmium%2C%20palladium%2C%20platinum%2C%20rhenium%2C%20rhodium%2C%20ruthenium%20and%20tungsten
	IRON,
	COBALT,
	GOLD,
	NICKEL,
	OBJ_TYPE_MAX
};

enum class tile_material : unsigned char {
	AIR,
	BEDROCK,
	ROCK,
	IRON,
	COBALT,
	GOLD,
	NICKEL,
	TILE_MAT_MAX
};

enum tile_flags : unsigned short {
	TOP = 1 << 0, //          0000 0001
	RIGHT = 1 << 1, //        0000 0010
	BOTTOM = 1 << 2, //       0000 0100
	LEFT = 1 << 3, //         0000 1000
	TOP_LEFT = 1 << 4, //     0001 0000
	TOP_RIGHT = 1 << 5, //    0010 0000
	BOTTOM_RIGHT = 1 << 6, // 0100 0000
	BOTTOM_LEFT = 1 << 7, //  1000 0000
};

struct tile_data {
	tile_material material;
	unsigned char light_level;
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

	static const int TILE_SIZE_PX = 16;
	static const int CHUNK_TILE_WIDTH = CHUNK_SIZE / TILE_SIZE_PX;
	static const int SPACE_TILE_WIDTH = SPACE_SIZE * CHUNK_TILE_WIDTH;
	static const int TILES_PER_CHUNK = CHUNK_TILE_WIDTH * CHUNK_TILE_WIDTH;
	static const int MAX_TILES = TILES_PER_CHUNK * MAX_CHUNKS;

	// we want the tilemap to span 2x2 chunks
	static const int TILEMAP_SIZE = CHUNK_TILE_WIDTH * 2;
	static const int TILEMAP_CELLS_SIZE = TILEMAP_SIZE * 2;
	static const int TILEMAP_MAX_CELLS = TILEMAP_CELLS_SIZE * TILEMAP_CELLS_SIZE;
	// how many pixels does the camera travel to cause the tilemap to reload

	static const int TILEMAP_LOAD_STRIDE_PX = TILE_SIZE_PX;

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
		// unsigned char counts[static_cast<unsigned long>(obj_type::OBJ_TYPE_MAX)];
	};

	struct raycast_hit {
		bn::point tile_pos;
		bn::fixed_point intersection_pos;
	};

	chunked_space(bn::camera_ptr camera);
	~chunked_space();

	void generate_next_chunk();
	int generated_chunks_count();
	bool is_generated();

	void update();

	bn::fixed_point spawn_point();

	bn::point space_point_to_tile_point(bn::fixed_point p_pos);
	bool is_solid_tile(bn::point p_pos);
	bool can_mine_tile(bn::point p_pos);

	bn::optional<raycast_hit> raycast(bn::fixed_point p_origin, bn::fixed_point p_dir);

	void mine_tile(bn::point p_tile_point);

	tile_data get_tile(bn::point p_tile_point);
	void set_tile_material(bn::point p_tile_point, tile_material p_tile_material);

	void spawn_floating_object(obj_type p_type, bn::fixed_point p_position, bn::fixed_point p_velocity);

	packed_tile_data pack_tile_data(tile_data p_data);
	tile_data unpack_tile_data(packed_tile_data p_data);

private:
	int chunks_generated;

	alignas(packed_tile_data) packed_tile_data _tile_cells[MAX_TILES];
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
	void _set_tilemap_tile(int seed, int p_x, int p_y, int p_tile_id, tile_material p_material);
	void _update_tilemap();
	tile_data _get_tile_at(int p_tile_x, int p_tile_y);

	bn::point _chunk_index_to_world_pos(int i);
	int _chunk_pos_to_index(int cx, int cy);
	chunk_point _point_to_chunk_pos(bn::fixed_point point);
	bn::fixed_point _chunk_pos_to_world_pos(chunk_point chunk_pos);
	bool _is_chunk_in_view(bn::fixed_point camera_pos, chunk_point chunk_pos);
	bool _is_point_in_view(bn::fixed_point camera_pos, bn::fixed_point point, bn::fixed size);
};

} //namespace game