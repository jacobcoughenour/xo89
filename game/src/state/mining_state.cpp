#include "state/mining_state.h"

#include "stb_perlin.h"

namespace game {

mining_state::mining_state(shared_state &p_shared) :
		state(),
		_shared(p_shared),
		_rng(),
		objects() {
	BN_ASSERT(helpers::is_point_in_view(bn::point(0, 0), bn::point(-120, 0), 0));
	BN_ASSERT(!helpers::is_point_in_view(bn::point(0, 0), bn::point(-121, 0), 0));

	BN_ASSERT(helpers::relative_tile_index(1, 2, 2, 16) == 35);

	BN_ASSERT(space_point_to_tile_point(bn::fixed_point(0.4, 2)) == bn::point(0, 0), space_point_to_tile_point(bn::fixed_point(0.4, 2)).x());

	BN_ASSERT(space_point_to_tile_point(bn::fixed_point(-0.4, -2)) == bn::point(-1, -1), space_point_to_tile_point(bn::fixed_point(-0.4, -2)).x());

	clear_inventory();

	ship_hitbox.set_width(8);
	ship_hitbox.set_height(8);
	ship_hitbox.set_position(spawn_point());
}

mining_state::~mining_state() {
}

void mining_state::generate_next_chunk() {
	BN_ASSERT(_chunks_generated < MAX_CHUNKS);

	int start_x = (_chunks_generated % SPACE_SIZE) * CHUNK_TILE_WIDTH;
	int start_y = (_chunks_generated / SPACE_SIZE) * CHUNK_TILE_WIDTH;
	int to_x = start_x + CHUNK_TILE_WIDTH;
	int to_y = start_y + CHUNK_TILE_WIDTH;

	auto spawn_point = this->spawn_point();
	spawn_point.set_y(0);
	spawn_point.set_x(spawn_point.x().integer() / 16);
	auto spawn_area = 4;

	// generate tiles
	for (int y = start_y; y < to_y; y++) {
		for (int x = start_x; x < to_x; x++) {
			tile_data data{};

			auto index = helpers::tile_pos_to_index(x, y, SPACE_TILE_WIDTH);

			if (x > spawn_point.x() - spawn_area //
					&& x < spawn_point.x() + spawn_area //
					&& y > spawn_point.y() - spawn_area //
					&& y < spawn_point.y() + spawn_area) {
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

			_tile_cells[index] = _pack_tile_data(data);
		}
	}

	_chunks_generated++;
}

int mining_state::generated_chunks_count() {
	return _chunks_generated;
}

bool mining_state::is_generated() {
	return _chunks_generated == MAX_CHUNKS;
}

bn::fixed_point mining_state::spawn_point() {
	return bn::fixed_point(SPACE_SIZE * CHUNK_SIZE / 2 + 8, 0);
}

bn::point mining_state::space_point_to_tile_point(bn::fixed_point p_pos) {
	return bn::point(
			p_pos.x() < 0
					? -((-p_pos.x().floor_integer()) / TILE_SIZE_PX) - 1
					: (p_pos.x().floor_integer() / TILE_SIZE_PX),
			p_pos.y() < 0
					? -((-p_pos.y().floor_integer()) / TILE_SIZE_PX) - 1
					: (p_pos.y().floor_integer() / TILE_SIZE_PX));
}

bool mining_state::is_solid_tile(bn::point p_pos) {
	auto c = get_tile_at(p_pos.x(), p_pos.y());
	return c.material != tile_material::AIR;
}

bool mining_state::can_mine_tile(bn::point p_pos) {
	auto c = get_tile_at(p_pos.x(), p_pos.y());
	// todo
	return c.material != tile_material::AIR && c.material != tile_material::BEDROCK;
}

void mining_state::spawn_floating_object(obj_type p_type, bn::fixed_point p_position, bn::fixed_point p_velocity) {
	if (objects.full()) {
		// make room
		objects.pop_back();
	}

	// todo you can do better (lut?)
	unsigned char sprite_index = static_cast<unsigned char>(p_type) + 1;
	if (sprite_index == 1 && _rng.get_bool()) {
		sprite_index = 0;
	}

	floating_object obj{
		p_type,
		sprite_index,
		p_position,
		p_velocity,
	};

	objects.push_front(obj);
}

bn::optional<mining_state::raycast_hit> mining_state::raycast(bn::fixed_point p_origin, bn::fixed_point p_dir, bn::fixed p_max_distance) {
	bn::optional<mining_state::raycast_hit> hit;

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

packed_tile_data mining_state::_pack_tile_data(tile_data p_data) {
	packed_tile_data data;

	// first 6 bits for material 0..64
	// next 2 for light level 0..4

	data = static_cast<unsigned char>(p_data.material);
	BN_ASSERT(data < 64);
	BN_ASSERT(p_data.light_level < 4);
	data = data | p_data.light_level << 6;
	return data;
}

tile_data mining_state::_unpack_tile_data(packed_tile_data p_data) {
	tile_data data;
	data.material = static_cast<tile_material>(p_data & 0b00111111);
	data.light_level = (p_data & 0b11000000) >> 6;
	return data;
}

tile_data mining_state::get_tile(bn::point p_tile_point) {
	return get_tile_at(p_tile_point.x(), p_tile_point.y());
}

void mining_state::set_tile_material(bn::point p_tile_point, tile_material p_tile_material) {
	int index = helpers::tile_pos_to_index(
			p_tile_point.x(),
			p_tile_point.y(),
			mining_state::SPACE_TILE_WIDTH);

	if (index < 0 || index >= MAX_TILES) {
		return; // bedrock borders
	}

	auto d = _unpack_tile_data(_tile_cells[index]);

	if (d.material == p_tile_material) {
		return;
	}

	d.material = p_tile_material;

	_tile_cells[index] = _pack_tile_data(d);

	// todo should only mark dirty when close to the camera?
	is_tileset_dirty = true;
}

void mining_state::mine_tile(bn::point p_tile_point) {
	if (!can_mine_tile(p_tile_point)) {
		return;
	}

	auto tile = get_tile(p_tile_point);
	set_tile_material(p_tile_point, tile_material::AIR);

	int drop_amount = 0;
	obj_type drop_type = static_cast<obj_type>(static_cast<unsigned char>(tile.material) - static_cast<unsigned char>(tile_material::ROCK));

	if (drop_type == obj_type::ROCK) {
		drop_amount = 1;
	} else {
		drop_amount = _rng.get_bool() ? 1 : 2;
	}

	for (int i = 0; i < drop_amount; i++) {
		bn::fixed_point position(
				p_tile_point.x() * 16 + 2 + _rng.get_fixed() % 4,
				p_tile_point.y() * 16 + 2 + _rng.get_fixed() % 4);
		bn::fixed_point velocity(_rng.get_fixed() % 4 - 2, _rng.get_fixed() % 4 - 2);

		spawn_floating_object(drop_type, position, velocity);
	}
}

bn::point mining_state::point_to_tilemap_pos(bn::fixed_point point) {
	return bn::point(
			(point.x().floor_integer() + 8) / TILEMAP_LOAD_STRIDE_PX,
			(point.y().floor_integer() + 8) / TILEMAP_LOAD_STRIDE_PX);
}

tile_data mining_state::get_tile_at(int p_tile_x, int p_tile_y) {
	if (p_tile_x < 0 || p_tile_y < 0 || p_tile_x >= SPACE_TILE_WIDTH || p_tile_y >= SPACE_TILE_WIDTH) {
		const int middle = SPACE_SIZE * CHUNK_SIZE / 2 / 16;

		if (p_tile_y < 0 && p_tile_x > middle - 4 && p_tile_x < middle + 4) {
			return tile_data{
				tile_material::AIR,
				0
			};
		}

		return tile_data{
			tile_material::BEDROCK,
			0
		};
	}

	int index = helpers::tile_pos_to_index(
			p_tile_x,
			p_tile_y,
			mining_state::SPACE_TILE_WIDTH);

	return _unpack_tile_data(_tile_cells[index]);
}

void mining_state::update() {
	// handle ship movement

	if (bn::keypad::left_held()) {
		ship_rotation -= 2.5;
	}
	if (bn::keypad::right_held()) {
		ship_rotation += 2.5;
	}
	ship_rotation = helpers::fposmod(ship_rotation, 360);

	if (bn::keypad::a_held()) {
		ship_velocity -= helpers::angle_to_dir(-ship_rotation) * bn::fixed(0.045);
	}
	if (bn::keypad::b_held()) {
		ship_velocity += helpers::angle_to_dir(-ship_rotation) * bn::fixed(0.035);
	}
	ship_velocity *= bn::fixed(0.99);

	bn::fixed_point cur_pos = ship_hitbox.position();
	bn::fixed_point desired_pos = cur_pos + ship_velocity;

	bn::fixed_rect final_aabb = ship_hitbox;
	final_aabb.set_position(desired_pos);

	bool hit = false;

	if (
			is_solid_tile(space_point_to_tile_point(final_aabb.top_left())) //
			|| is_solid_tile(space_point_to_tile_point(final_aabb.top_right())) //
			|| is_solid_tile(space_point_to_tile_point(final_aabb.bottom_left())) //
			|| is_solid_tile(space_point_to_tile_point(final_aabb.bottom_right()))) {
		hit = true;

		ship_velocity = ship_velocity * bn::fixed(-0.6);
	}

	ship_hitbox = hit ? ship_hitbox : final_aabb;

	bn::fixed_point ship_pos = ship_hitbox.center();

	// go through all the loaded chunks and move the objects by their current
	// velocity

	for (auto it = objects.begin(); it != objects.end(); ++it) {
		auto &obj = *it;

		bn::fixed dist = helpers::max_box_dist(obj.position, ship_pos);

		if (dist < 2) {
			pickup_resource(obj.object_type, 1);
			objects.erase(it);
			continue;
		}

		bn::fixed len = helpers::distance(ship_pos, obj.position);
		bn::fixed_point dir = helpers::normalize_point(ship_pos - obj.position);
		// attractor influence distance
		constexpr int d = 40;
		// push object towards ship
		obj.velocity += dir * bn::min(bn::fixed(10), bn::max(d - len, bn::fixed(0.2))) * bn::fixed(0.03);

		// move by velocity
		auto obj_desired_pos = obj.position + obj.velocity;

		if (is_solid_tile(space_point_to_tile_point(obj_desired_pos))) {
			// bounce
			obj.velocity *= bn::fixed(-0.9);
		} else {
			obj.position = obj_desired_pos;
			// velocity damping
			obj.velocity *= bn::fixed(0.98);
			// gravity
			obj.velocity += bn::fixed_point(0.0, 0.08);
		}
	}

	// handle item pickup queue

	for (auto it = item_pickup_queue.begin(); it != item_pickup_queue.end(); ++it) {
		auto &obj = *it;
		if (obj.frame < _item_queue_frame) {
			item_pickup_queue.erase(it);
		}
	}

	if (item_pickup_queue.size() > 0) {
		_item_queue_frame++;
	} else {
		_item_queue_frame = 0;
	}
}

void mining_state::clear_inventory() {
	for (size_t i = 0; i < static_cast<unsigned long>(obj_type::OBJ_TYPE_MAX); i++) {
		item_inventory[i] = 0;
	}
}

void mining_state::pickup_resource(obj_type p_type, int p_amount) {
	item_inventory[static_cast<unsigned long>(p_type)] += p_amount;

	int frame = _item_queue_frame + ITEM_QUEUE_FRAMES_TIME;

	for (auto it = item_pickup_queue.begin(); it != item_pickup_queue.end(); ++it) {
		auto &obj = *it;
		if (obj.object_type == p_type) {
			obj.amount += p_amount;
			obj.frame = frame;
			return;
		}
	}

	if (item_pickup_queue.size() >= item_pickup_queue.max_size()) {
		item_pickup_queue.pop_front();
	}
	item_pickup_queue.push_back({
			p_type,
			p_amount,
			frame,
	});
}

} //namespace game