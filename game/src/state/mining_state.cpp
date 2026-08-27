#include "state/mining_state.h"

#include "entities/floating_item.h"

#include "bn_log.h"
#include "stb_perlin.h"

namespace game {

mining_state::mining_state(shared_state &p_shared) :
		state(),
		_shared(p_shared),
		_rng(),
		objects(),
		projectiles() {
	BN_ASSERT(helpers::is_point_in_view(bn::point(0, 0), bn::point(-120, 0), 0));
	BN_ASSERT(!helpers::is_point_in_view(bn::point(0, 0), bn::point(-121, 0), 0));

	BN_ASSERT(helpers::relative_tile_index(1, 2, 2, 16) == 35);

	BN_ASSERT(space_point_to_tile_point(bn::fixed_point(0.4, 2)) == bn::point(0, 0), space_point_to_tile_point(bn::fixed_point(0.4, 2)).x());

	BN_ASSERT(space_point_to_tile_point(bn::fixed_point(-0.4, -2)) == bn::point(-1, -1), space_point_to_tile_point(bn::fixed_point(-0.4, -2)).x());

	_rng.set_seed(p_shared.get_frame_count());
	_seed = _rng.get_int();
	clear_inventory();
	_chunks_generated = 0;

	BN_LOG("generating with seed ", _seed);

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
				auto sample = stb_perlin_noise3(
						bn::fixed(x) / bn::fixed(8),
						bn::fixed(y) / bn::fixed(8),
						_seed,
						SPACE_TILE_WIDTH / 8,
						SPACE_TILE_WIDTH,
						255);
				if (sample > bn::fixed(-0.08)) {
					data.material = tile_material::ROCK;
				}
				if (sample > bn::fixed(0.2)) {
					// dense enough to spawn an ore

					_rng.set_seed(_seed + index);
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

// 3 3 3 2 3 3 3
// 3 3 2 1 2 3 3
// 3 2 1 0 1 2 3
// 2 1 0 x 0 1 2
// 3 2 1 0 1 2 3
// 3 3 2 1 2 3 3
// 3 3 3 2 3 3 3

static const bn::point _light_circles[]{
	// 0
	{ 0, -1 },
	{ 1, 0 },
	{ 0, 1 },
	{ -1, 0 },
	// 1
	{ 0, -2 },
	{ 1, -1 },
	{ 2, 0 },
	{ 1, 1 },
	{ 0, 2 },
	{ -1, 1 },
	{ -2, 0 },
	{ -1, -1 },
	// 2
	{ 0, -3 },
	{ 1, -2 },
	{ 2, -1 },
	{ 3, 0 },
	{ 2, 1 },
	{ 1, 2 },
	{ 0, 3 },
	{ -1, 2 },
	{ -2, 1 },
	{ -3, 0 },
	{ -2, -1 },
	{ -1, -2 },
};

static const int _light_circle_lengths[]{
	4,
	8,
	12
};

unsigned char mining_state::_calc_tile_light_level(bn::point p_tile_pos) {
	unsigned char light_level = 0;
	int i = 0;
	for (; light_level < 3; light_level++) {
		auto end = i + _light_circle_lengths[light_level];
		for (; i < end; i++) {
			auto p = _light_circles[i];
			if (!is_solid_tile(p_tile_pos + p)) {
				return light_level;
			}
		}
	}
	return 3;
}

void mining_state::bake_lighting() {
	for (int y = 0; y < SPACE_TILE_WIDTH; y++) {
		for (int x = 0; x < SPACE_TILE_WIDTH; x++) {
			auto light = _calc_tile_light_level(bn::point(x, y));
			if (light > 0) {
				int index = helpers::tile_pos_to_index(
						x,
						y,
						SPACE_TILE_WIDTH);
				auto data = _unpack_tile_data(_tile_cells[index]);
				data.light_level = light;
				_tile_cells[index] = _pack_tile_data(data);
			}
		}
	}
}

void mining_state::_recalculate_lighting(bn::point p_tile_pos) {
	for (auto p : _light_circles) {
		auto relative = p_tile_pos + p;
		int index = helpers::tile_pos_to_index(
				relative.x(),
				relative.y(),
				SPACE_TILE_WIDTH);
		if (index < 0 || index >= MAX_TILES) {
			continue;
		}
		auto light = _calc_tile_light_level(relative);
		auto data = _unpack_tile_data(_tile_cells[index]);
		data.light_level = light;
		_tile_cells[index] = _pack_tile_data(data);
	}
}

bn::fixed_point mining_state::spawn_point() {
	return bn::fixed_point(SPACE_SIZE * CHUNK_SIZE / 2 + 8, 0);
}

void mining_state::leave() {
	// transfer items
	for (size_t i = 0; i < ITEM_TYPE_COUNT; i++) {
		_shared.deposit_to_inventory(static_cast<item_type>(i), item_inventory[i]);
	}
	show_leave_confirmation = false;
	_shared.save();
}

void mining_state::leave_canceled() {
	ship_hitbox.set_position(spawn_point());
	ship_rotation = 180;
	ship_velocity = bn::fixed_point(0, 0.2);
	show_leave_confirmation = false;
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

void mining_state::spawn_floating_object(item_type p_type, bn::fixed_point p_position, bn::fixed_point p_velocity) {
	if (objects.full()) {
		// make room
		objects.pop_back();
	}

	// todo you can do better (lut?)
	unsigned char sprite_index = static_cast<unsigned char>(p_type) + 1;
	if (sprite_index == 1 && _rng.get_bool()) {
		sprite_index = 0;
	}

	floating_item obj(*this, p_type, sprite_index, p_position, p_velocity);

	objects.push_front(obj);
}

void mining_state::spawn_projectile(bn::fixed_point p_position, bn::fixed_point p_velocity) {
	if (projectiles.full()) {
		// make room
		projectiles.pop_back();
	}

	projectile proj(*this, p_position, p_velocity);

	projectiles.push_front(proj);
}

bn::optional<mining_state::raycast_hit> mining_state::raycast(bn::fixed_point p_origin, bn::fixed_point p_dir, bn::fixed p_max_distance) {
	bn::optional<mining_state::raycast_hit> hit;

	auto step = helpers::set_length(p_dir, 8.0);
	auto steps = p_max_distance / 8.0;

	for (bn::fixed i = 0; i < steps; i += 1) {
		auto tile_pos = space_point_to_tile_point(p_origin);
		if (is_solid_tile(tile_pos)) {
			hit = raycast_hit{
				.tile_pos = tile_pos,
				// todo we need to figure out the actual intersection point
				.intersection_pos = bn::fixed_point(tile_pos * 16) + bn::point(8, 8),
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
	return tile_data{
		.material = static_cast<tile_material>(p_data & 0b00111111),
		.light_level = static_cast<unsigned char>((p_data & 0b11000000) >> 6)
	};
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

	_recalculate_lighting(p_tile_point);

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
	item_type drop_type = static_cast<item_type>(static_cast<unsigned char>(tile.material) - static_cast<unsigned char>(tile_material::ROCK));

	if (drop_type == item_type::ROCK) {
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
				.material = tile_material::AIR,
				.light_level = 0,
			};
		}

		return tile_data{
			.material = tile_material::BEDROCK,
			.light_level = 0
		};
	}

	int index = helpers::tile_pos_to_index(
			p_tile_x,
			p_tile_y,
			mining_state::SPACE_TILE_WIDTH);

	return _unpack_tile_data(_tile_cells[index]);
}

void mining_state::take_damage(unsigned int p_damage_amount) {
	if (p_damage_amount >= ship_health) {
		ship_health = 0;
	} else {
		ship_health -= p_damage_amount;
		ship_invincible_timer = SHIP_INVINCIBLE_FRAMES;
	}
}

void mining_state::update() {
	// handle ship movement

	if (ship_hitbox.position().y() < -80) {
		show_leave_confirmation = true;
		return;
	}

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

	if (ship_invincible_timer > 0) {
		ship_invincible_timer--;
	}

	// collision

	bool hit = false;
	if (
			is_solid_tile(space_point_to_tile_point(final_aabb.top_left())) //
			|| is_solid_tile(space_point_to_tile_point(final_aabb.top_right())) //
			|| is_solid_tile(space_point_to_tile_point(final_aabb.bottom_left())) //
			|| is_solid_tile(space_point_to_tile_point(final_aabb.bottom_right()))) {
		hit = true;
		if (ship_invincible_timer == 0) {
			auto speed = helpers::point_length(ship_velocity);
			if (speed > 0.72) {
				take_damage(3);
			}
		}
		ship_velocity = ship_velocity * bn::fixed(-0.6);
	}

	ship_hitbox = hit ? ship_hitbox : final_aabb;

	// process abilities

	const bn::fixed max_dist = 64;

	auto targetting_hit = raycast(ship_hitbox.center(), -helpers::angle_to_dir(-ship_rotation), max_dist);
	_aim_direction = -helpers::set_length(helpers::angle_to_dir(-ship_rotation), max_dist - 4.0);

	if (targetting_hit.has_value()) {
		auto dist = helpers::distance(ship_hitbox.center(), targetting_hit.value().intersection_pos);
		_aim_direction = targetting_hit.value().intersection_pos - ship_hitbox.position();

		auto new_tile = targetting_hit.value().tile_pos;
		if (new_tile != _laser_target_cell) {
			_mining_timer = 0;
			_laser_target_cell = new_tile;
		}

		if (bn::keypad::r_held()) {
			_mining_timer++;
			if (_mining_timer >= 30) {
				mine_tile(_laser_target_cell.value());
				_mining_timer = 0;
			}
		} else {
			_mining_timer = 0;
		}
	} else {
		_mining_timer = 0;
		_laser_target_cell.reset();
	}

	// process entities

	for (auto it = objects.begin(); it != objects.end(); ++it) {
		auto &obj = *it;
		if (!obj.update()) {
			objects.erase(it);
		}
	}

	for (auto it = projectiles.begin(); it != projectiles.end(); ++it) {
		auto &obj = *it;
		if (!obj.update()) {
			projectiles.erase(it);
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
	for (size_t i = 0; i < ITEM_TYPE_COUNT; i++) {
		item_inventory[i] = 0;
	}
}

void mining_state::pickup_resource(item_type p_type, int p_amount) {
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
			.object_type = p_type,
			.amount = p_amount,
			.frame = frame,
	});
}

} //namespace game