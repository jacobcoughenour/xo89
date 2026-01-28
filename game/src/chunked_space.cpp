#include "chunked_space.h"

namespace game {

chunked_space::chunked_space(bn::camera_ptr camera) :
		_camera(camera),
		_unloaded_chunks(),
		_loaded_chunks(),
		_rng() {
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

	BN_ASSERT(_chunk_index_to_world_pos(0) == bn::point(0, 0));
	BN_ASSERT(_point_to_chunk_pos(bn::point(0, 0)) == bn::point(0, 0));
	BN_ASSERT(_point_to_chunk_pos(bn::point(CHUNK_SIZE, 0)) == bn::point(1, 0));

	BN_ASSERT(_is_point_in_view(bn::point(0, 0), bn::point(-120, 0), 0));
	BN_ASSERT(!_is_point_in_view(bn::point(0, 0), bn::point(-121, 0), 0));

	BN_ASSERT(_is_chunk_in_view(bn::point(0, 0), bn::point(0, 0)));
	// todo more tests
}

chunked_space::~chunked_space() {
}

bn::fixed_point chunked_space::spawn_point() {
	return bn::fixed_point(SPACE_SIZE * CHUNK_SIZE / 2, SPACE_SIZE * CHUNK_SIZE / 2);
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

void chunked_space::update() {
	bn::fixed_point ship_pos = _camera.position();

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
