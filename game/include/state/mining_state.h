#pragma once

#include "helpers.h"
#include "items.h"
#include "shared_state.h"

#include "entities/creep.h"
#include "entities/entity.h"
#include "entities/floating_item.h"
#include "entities/projectile.h"
#include "entities/turret.h"

#include "bn_array.h"
#include "bn_bg_palettes.h"
#include "bn_bg_tiles.h"
#include "bn_camera_ptr.h"
#include "bn_common.h"
#include "bn_fixed_rect.h"
#include "bn_keypad.h"
#include "bn_list.h"
#include "bn_memory.h"
#include "bn_random.h"
#include "bn_regular_bg_item.h"
#include "bn_regular_bg_map_cell_info.h"
#include "bn_regular_bg_map_ptr.h"
#include "bn_regular_bg_ptr.h"
#include "bn_seed_random.h"
#include "bn_sprite_ptr.h"
#include "bn_vector.h"

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

enum class drone_mode : unsigned char {
	MINING,
	COMBAT
};

class mining_state : public state {
public:
	static const int MAX_VISIBLE_FLOATING_ITEMS = 16;
	static const int MAX_FLOATING_ITEMS = MAX_VISIBLE_FLOATING_ITEMS * 2;

	static const int MAX_VISIBLE_PROJECTILES = 16;
	static const int MAX_PROJECTILES = MAX_VISIBLE_PROJECTILES * 2;

	static const int MAX_TURRETS = 64;
	static const int MAX_CREEPS = 64;
	static const int MAX_ENTITIES = MAX_TURRETS + MAX_CREEPS;

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

	static const int SHIP_INVINCIBLE_FRAMES = 60;

private:
	shared_state &_shared;

public:
	explicit mining_state(shared_state &p_shared);
	~mining_state();

	void update();

private:
	unsigned int _seed;
	bn::seed_random _rng;
	int _chunks_generated;
	alignas(packed_tile_data) packed_tile_data _tile_cells[MAX_TILES];
	unsigned char _calc_tile_light_level(bn::point p_tile_pos);
	void _recalculate_lighting(bn::point p_tile_pos);

	drone_mode _drone_mode = drone_mode::MINING;
	bool _is_thrusting = false;

	int _mining_timer;
	int _mining_duration = 30;
	bn::optional<bn::point> _laser_target_cell;
	bn::fixed_point _aim_direction;
	combat_entity *_target_entity;

	int _fire_timer;
	int _fire_cooldown = 16;

public:
	drone_mode get_drone_mode() { return _drone_mode; }
	void set_drone_mode(drone_mode p_drone_mode);
	bool is_thrusting() { return _is_thrusting; }

	bn::fixed get_mining_progress() { return bn::fixed(_mining_timer) / bn::fixed(_mining_duration); }
	bn::optional<bn::point> get_targeting_cell() { return _laser_target_cell; }
	bn::fixed_point get_aim_direction() { return _aim_direction; }

	void generate_next_chunk();
	int generated_chunks_count();
	bool is_generated();
	void bake_lighting();
	void place_entities();

	bn::fixed_point spawn_point();

	void leave();
	void leave_canceled();

	bn::point space_point_to_tile_point(bn::fixed_point p_pos);
	bool is_solid_tile(bn::point p_pos);
	bool can_mine_tile(bn::point p_pos);
	tile_data get_tile(bn::point p_tile_point);
	void set_tile_material(bn::point p_tile_point, tile_material p_tile_material);
	void mine_tile(bn::point p_tile_point);

	void take_damage(unsigned int p_damage_amount);

private:
	bn::camera_ptr _camera;

public:
	inline bn::camera_ptr get_camera() { return _camera; }

	// do we want to combine these into a "transform"?
	bn::fixed_rect ship_hitbox;
	// start pointing down
	bn::fixed ship_rotation = 180;
	bn::fixed_point ship_velocity;
	unsigned int ship_health = 100;
	unsigned int ship_invincible_timer = 0;

	bn::list<floating_item, MAX_FLOATING_ITEMS> floating_items;
	bn::list<projectile, MAX_PROJECTILES> projectiles;
	bn::list<turret, MAX_TURRETS> turrets;
	bn::list<creep, MAX_CREEPS> creeps;

	bn::point point_to_tilemap_pos(bn::fixed_point p_pos);
	tile_data get_tile_at(int p_tile_x, int p_tile_y);

	bool is_tileset_dirty;
	bool show_leave_confirmation;

private:
	packed_tile_data _pack_tile_data(tile_data p_data);
	tile_data _unpack_tile_data(packed_tile_data p_data);

public:
	void spawn_floating_object(item_type p_type, bn::fixed_point p_position, bn::fixed_point p_velocity);
	void spawn_projectile(bool p_from_player, unsigned int p_damage_amount, bn::fixed_point p_position, bn::fixed_point p_velocity);

	struct raycast_hit {
		bn::point tile_pos;
		bn::fixed_point intersection_pos;
	};

	bn::optional<raycast_hit> raycast(bn::fixed_point p_origin, bn::fixed_point p_dir, bn::fixed p_max_distance);

	int item_inventory[ITEM_TYPE_COUNT];
	void clear_inventory();
	void pickup_resource(item_type p_type, int amount);

	struct item_queue_entry {
		item_type object_type;
		int amount;
		int frame;
	};

	static constexpr int ITEM_QUEUE_FRAMES_TIME = 90;

private:
	int _item_queue_frame;

public:
	bn::list<item_queue_entry, 5> item_pickup_queue;
	int item_queue_frame() { return _item_queue_frame; }
};

} //namespace game