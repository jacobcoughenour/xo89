#pragma once

#include "bn_sstream.h"

namespace game {

enum class obj_type : unsigned char {
	// https://en.wikipedia.org/wiki/Asteroid_mining#:~:text=crust%20with%20metals%20like%20gold%2C%20cobalt%2C%20iron%2C%20manganese%2C%20molybdenum%2C%20nickel%2C%20osmium%2C%20palladium%2C%20platinum%2C%20rhenium%2C%20rhodium%2C%20ruthenium%20and%20tungsten
	ROCK,
	IRON,
	COBALT,
	GOLD,
	NICKEL,
	OBJ_TYPE_MAX
};

constexpr int ITEM_TYPE_COUNT = static_cast<int>(obj_type::OBJ_TYPE_MAX);

inline void append_item_name(bn::ostringstream &stream, obj_type p_item_type) {
	if (p_item_type == obj_type::ROCK) {
		stream.append("ROCK");
	} else if (p_item_type == obj_type::IRON) {
		stream.append("IRON");
	} else if (p_item_type == obj_type::COBALT) {
		stream.append("COBALT");
	} else if (p_item_type == obj_type::GOLD) {
		stream.append("GOLD");
	} else if (p_item_type == obj_type::NICKEL) {
		stream.append("NICKEL");
	} else {
		stream.append("UNKNOWN");
	}
}

} //namespace game