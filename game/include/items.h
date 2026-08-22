#pragma once

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

constexpr int ITEM_TYPE_COUNT = static_cast<int>(item_type::ITEM_TYPE_MAX);

struct item_info {
	item_type type;
	const char *display_name;
	// todo icon asset
};

constexpr item_info items[ITEM_TYPE_COUNT] = {
	{ game::item_type::ROCK, "ROCK" },
	{ game::item_type::IRON, "IRON" },
	{ game::item_type::COBALT, "COBALT" },
	{ game::item_type::GOLD, "GOLD" },
	{ game::item_type::NICKEL, "NICKEL" },
};

constexpr item_info get_item_info(item_type p_type) {
	return items[static_cast<int>(p_type)];
}

} //namespace game