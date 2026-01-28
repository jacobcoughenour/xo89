#ifndef CHUNKED_SPACE_H
#define CHUNKED_SPACE_H

#include "helpers.h"

#include "bn_camera_ptr.h"
#include "bn_fixed_point.h"
#include "bn_list.h"
#include "bn_point.h"
#include "bn_rect.h"
#include "bn_seed_random.h"
#include "bn_sprite_ptr.h"
#include "bn_unordered_map.h"
#include "bn_vector.h"

// assets
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

class chunked_space {
public:
	static const int MAX_OBJS_PER_CHUNK = 32;
	static const int MAX_UNIQUE_OBJS_PER_CHUNK = 8;
	static const int CHUNK_SIZE = 128;
	static const int SPACE_SIZE = 64;
	static const int MAX_CHUNKS = SPACE_SIZE * SPACE_SIZE;
	static const int MAX_VISIBLE_OBJS = 64;
	static const int MAX_LOADED_CHUNKS = 9;

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
	bn::camera_ptr _camera;
	bn::vector<unloaded_chunk, MAX_CHUNKS> _unloaded_chunks;
	bn::vector<loaded_chunk, MAX_LOADED_CHUNKS> _loaded_chunks;
	bn::seed_random _rng;
	bn::vector<bn::sprite_ptr, MAX_VISIBLE_OBJS> _obj_sprites;

	bn::point _chunk_index_to_world_pos(int i);
	int _chunk_pos_to_index(int cx, int cy);
	chunk_point _point_to_chunk_pos(bn::fixed_point point);
	bn::fixed_point _chunk_pos_to_world_pos(chunk_point chunk_pos);
	bool _is_chunk_in_view(bn::fixed_point camera_pos, chunk_point chunk_pos);
	bool _is_point_in_view(bn::fixed_point camera_pos, bn::fixed_point point, bn::fixed size);
};

} //namespace game
#endif