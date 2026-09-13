#pragma once

#include "bn_color.h"
#include "bn_seed_random.h"
#include "bn_sstream.h"

namespace game {

enum class item_type : unsigned char {
	// https://en.wikipedia.org/wiki/Asteroid_mining#:~:text=crust%20with%20metals%20like%20gold%2C%20cobalt%2C%20iron%2C%20manganese%2C%20molybdenum%2C%20nickel%2C%20osmium%2C%20palladium%2C%20platinum%2C%20rhenium%2C%20rhodium%2C%20ruthenium%20and%20tungsten
	ROCK,
	IRON,
	COBALT,
	GOLD,
	NICKEL,
	ITEM_TYPE_MAX
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

constexpr bn::color tile_material_color[static_cast<long>(tile_material::TILE_MAT_MAX)] = {
	bn::color(0, 0, 0), // 	AIR,
	bn::color(5, 0, 5), // 	BEDROCK,
	bn::color(10, 0, 10), // 	ROCK,
	bn::color(255 / 8, 148 / 8, 148 / 8), // 	IRON,
	bn::color(87 / 8, 131 / 8, 207 / 8), // 	COBALT,
	bn::color(255 / 8, 227 / 8, 115 / 8), // 	GOLD,
	bn::color(212 / 8, 212 / 8, 212 / 8), // 	NICKEL,
};

constexpr bn::color get_tile_color(tile_material p_mat) {
	return tile_material_color[static_cast<int>(p_mat)];
}

constexpr int ITEM_TYPE_COUNT = static_cast<int>(item_type::ITEM_TYPE_MAX);

constexpr int MAX_BOUNTIES = 4;

struct item_info {
	item_type type;
	const char *display_name;
	unsigned int sprite_index;
	bn::fixed avg_unit_price;
};

constexpr item_info items[ITEM_TYPE_COUNT] = {
	// clang-format off
	{ game::item_type::ROCK,   "ROCK",   0,  0.2 },
	{ game::item_type::IRON,   "IRON",   2,  5 },
	{ game::item_type::COBALT, "COBALT", 4, 20 },
	{ game::item_type::GOLD,   "GOLD",   6, 10 },
	{ game::item_type::NICKEL, "NICKEL", 8,  6 },
	// clang-format on
};

constexpr item_info get_item_info(item_type p_type) {
	return items[static_cast<int>(p_type)];
}

struct material_values {
	tile_material material;
	int min_depth;
	int max_depth;
	unsigned int base_drop_amount;
	unsigned int bonus_drop_amount;
	unsigned int chance;
};

constexpr unsigned int rock_chance = 60;
constexpr unsigned int material_table_size = static_cast<int>(tile_material::TILE_MAT_MAX) - static_cast<int>(tile_material::IRON);
constexpr material_values material_table[material_table_size] = {
	// clang-format off
	{  game::tile_material::IRON,    0, 128, 2, 1, 9 },
	{  game::tile_material::COBALT, 64,  80, 1, 2, 3 },
	{  game::tile_material::GOLD,   90, 128, 1, 0, 3 },
	{  game::tile_material::NICKEL,  0, 128, 2, 1, 9 },
	// clang-format on
};

} //namespace game