#include "chunked_space.h"

namespace game {

chunked_space::chunked_space(bn::camera_ptr camera) :
		_camera(camera),
		_chunks(),
		_rng() {
	for (int i = 0; i < MAX_CHUNKS; i++) {
		_chunks.push_back(chunk{
				.objs = bn::vector<floating_object, MAX_OBJS_PER_CHUNK>() });

		for (int j = 0; j < MAX_OBJS_PER_CHUNK; j++) {
			floating_object obj = floating_object{
				.velocity = bn::fixed_point(0, 0),
				.position = _chunk_index_to_pos(i) + bn::fixed_point(_rng.get_fixed() % CHUNK_SIZE, _rng.get_fixed() % CHUNK_SIZE)
			};
			_chunks[i].objs.push_back(obj);
		}
	}
}

chunked_space::~chunked_space() {
}

bn::point chunked_space::_chunk_index_to_pos(int i) {
	return bn::point((i % SPACE_SIZE - SPACE_SIZE / 2) * CHUNK_SIZE, (i / SPACE_SIZE - SPACE_SIZE / 2) * CHUNK_SIZE);
}

bool chunked_space::_is_chunk_in_view(int i) {
	bn::point chunk_pos = _chunk_index_to_pos(i);

	bn::fixed left = _camera.x() - 120;
	bn::fixed right = _camera.x() + 120;
	bn::fixed top = _camera.y() - 80;
	bn::fixed bottom = _camera.y() + 80;

	if (chunk_pos.x() + CHUNK_SIZE < left) {
		return false;
	}
	if (chunk_pos.x() > right) {
		return false;
	}
	if (chunk_pos.y() + CHUNK_SIZE < top) {
		return false;
	}
	if (chunk_pos.y() > bottom) {
		return false;
	}

	return true;
}

bool chunked_space::_is_point_in_view(bn::fixed_point point) {
	bn::fixed left = _camera.x() - 120;
	bn::fixed right = _camera.x() + 120;
	bn::fixed top = _camera.y() - 80;
	bn::fixed bottom = _camera.y() + 80;

	if (point.x() + 8 < left) {
		return false;
	}
	if (point.x() > right) {
		return false;
	}
	if (point.y() + 8 < top) {
		return false;
	}
	if (point.y() > bottom) {
		return false;
	}
	return true;
}

void chunked_space::update() {
	// go through all the loaded chunks and move the objects by their current
	// velocity

	bn::fixed_point attr = _camera.position();

	for (int i = 0; i < MAX_CHUNKS; i++) {
		if (!_is_chunk_in_view(i)) {
			continue;
		}

		// todo load/unload chunks if camera moved

		chunk &c = _chunks.at(i);

		for (int j = 0; j < c.objs.size(); j++) {
			floating_object &obj = c.objs.at(j);

			bn::fixed dist = helpers::max_box_dist(obj.position, attr);

			if (dist < 2) {
				// pickup
				c.objs.erase(&obj);
				j--;
				continue;
			}

			constexpr int d = 48;

			if (dist > d) {
				continue;
			}

			bn::fixed len = helpers::distance(attr, obj.position);
			bn::fixed_point dir = helpers::normalize_point(attr - obj.position);

			obj.velocity += dir * bn::min(bn::fixed(10), bn::max(bn::fixed(d - len), bn::fixed(0.2))) * bn::fixed(0.03);
			obj.position += obj.velocity;
			obj.velocity *= bn::fixed(0.96);
		}
	}

	// for (int i = 0; i < MAX_CHUNKS; i++) {
	// 	if (!_is_chunk_in_view(i)) {
	// 		continue;
	// 	}
	// 	chunk &c = _chunks.at(i);
	// 	for (int j = 0; j < c.objs.size(); j++) {
	// 		floating_object &obj = c.objs.at(j);
	// 		// todo do a bounds check and move to different chunk if outside this one
	// 		// todo this probably doesn't have to run every frame...
	// 	}
	// }

	// render the objects

	int sprite_index = 0;

	for (int i = 0; i < MAX_CHUNKS; i++) {
		if (sprite_index >= MAX_VISIBLE_OBJS) {
			break;
		}
		if (!_is_chunk_in_view(i)) {
			continue;
		}
		chunk &c = _chunks.at(i);
		for (int j = 0; j < c.objs.size(); j++) {
			if (sprite_index >= MAX_VISIBLE_OBJS) {
				break;
			}

			floating_object &obj = c.objs.at(j);
			if (!_is_point_in_view(obj.position)) {
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
