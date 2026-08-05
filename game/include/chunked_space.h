#pragma once

#include "game_state.h"
#include "helpers.h"
#include "obj_type.h"

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
#include "bn_sprite_tiles_ptr.h"
#include "bn_unordered_map.h"
#include "bn_vector.h"

// assets
#include "bn_bg_palette_items_palette.h"
#include "bn_regular_bg_tiles_items_tiles.h"
#include "bn_sprite_items_dropped_items.h"

namespace game {

using chunk_point = bn::point;

using packed_tile_data = unsigned char;

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
	static const int MAX_VISIBLE_OBJS = 16;
	static const int MAX_OBJS = MAX_VISIBLE_OBJS * 2;

	static const int CHUNK_SIZE = 128;
	static const int SPACE_SIZE = 16;
	static const int MAX_CHUNKS = SPACE_SIZE * SPACE_SIZE;
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

	struct raycast_hit {
		bn::point tile_pos;
		bn::fixed_point intersection_pos;
	};

	explicit chunked_space(game_state &state, bn::camera_ptr camera);
	~chunked_space();

	void generate_next_chunk();
	int generated_chunks_count();
	bool is_generated();

	void update();

	bn::fixed_point spawn_point();

	bn::point space_point_to_tile_point(bn::fixed_point p_pos);
	bool is_solid_tile(bn::point p_pos);
	bool can_mine_tile(bn::point p_pos);

	bn::optional<raycast_hit> raycast(bn::fixed_point p_origin, bn::fixed_point p_dir, bn::fixed p_max_distance);

	void mine_tile(bn::point p_tile_point);

	tile_data get_tile(bn::point p_tile_point);
	void set_tile_material(bn::point p_tile_point, tile_material p_tile_material);

	void spawn_floating_object(obj_type p_type, bn::fixed_point p_position, bn::fixed_point p_velocity);

	packed_tile_data pack_tile_data(tile_data p_data);
	tile_data unpack_tile_data(packed_tile_data p_data);

private:
	game_state &_state;

	struct floating_object {
		obj_type object_type;
		unsigned char sprite_index;
		bn::fixed_point position;
		bn::fixed_point velocity;
	};

	int _chunks_generated;

	alignas(packed_tile_data) packed_tile_data _tile_cells[MAX_TILES];
	alignas(int) bn::regular_bg_map_cell _tilemap_cells[TILEMAP_MAX_CELLS];
	bn::regular_bg_map_item _tilemap_item;
	bn::optional<bn::regular_bg_item> _tilemap_bg_item;
	bn::optional<bn::regular_bg_ptr> _tilemap_bg;
	bn::point _tilemap_loaded_point;

	bn::camera_ptr _camera;
	bn::seed_random _rng;

	bn::list<floating_object, MAX_OBJS> _objects;
	bn::vector<bn::sprite_ptr, MAX_VISIBLE_OBJS> _obj_sprites;
	int _obj_flicker_frame;

	bn::point _point_to_tilemap_pos(bn::fixed_point p_pos);
	void _set_tilemap_tile(int seed, int p_x, int p_y, int p_tile_id, tile_material p_material);
	void _update_tilemap();
	tile_data _get_tile_at(int p_tile_x, int p_tile_y);

	bool _is_point_in_view(bn::fixed_point camera_pos, bn::fixed_point point, bn::fixed size);
};

} //namespace game