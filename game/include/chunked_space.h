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
#include "bn_vector.h"

// assets
#include "bn_sprite_items_dev8.h"

namespace game {

class chunked_space {
public:
	static const int MAX_OBJS_PER_CHUNK = 32;
	static const int CHUNK_SIZE = 128;
	static const int SPACE_SIZE = 8;
	static const int MAX_CHUNKS = SPACE_SIZE * SPACE_SIZE;
	static const int MAX_VISIBLE_OBJS = 64;

	struct floating_object {
		bn::fixed_point velocity;
		bn::fixed_point position;
	};

	struct chunk {
		bn::vector<floating_object, MAX_OBJS_PER_CHUNK> objs;
	};

	chunked_space(bn::camera_ptr camera);
	~chunked_space();

	void update();

private:
	bn::camera_ptr _camera;
	bn::vector<chunk, MAX_CHUNKS> _chunks;
	bn::seed_random _rng;
	bn::vector<bn::sprite_ptr, MAX_VISIBLE_OBJS> _obj_sprites;

	bn::point _chunk_index_to_pos(int i);
	bool _is_chunk_in_view(int i);
	bool _is_point_in_view(bn::fixed_point point);
};

} //namespace game
#endif