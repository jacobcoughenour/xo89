#pragma once

#include "bn_sstream.h"

namespace game {

enum class upgrade_type : unsigned char {
	MINING_LASER,
	MAGNET,
	ARMOR,
	SCANNER,
	UPGRADE_TYPE_MAX
};

constexpr int UPGRADE_COUNT = static_cast<int>(upgrade_type::UPGRADE_TYPE_MAX);

struct upgrade_info {
	upgrade_type type;
	const char *display_name;
};

constexpr upgrade_info upgrades[UPGRADE_COUNT] = {
	// clang-format off
	{ game::upgrade_type::MINING_LASER, "MINING LASER" },
	{ game::upgrade_type::MAGNET,       "MAGNET" },
	{ game::upgrade_type::ARMOR,        "ARMOR" },
	{ game::upgrade_type::SCANNER,      "SCANNER" },
	// clang-format on
};

constexpr upgrade_info get_upgrade_info(upgrade_type p_type) {
	return upgrades[static_cast<int>(p_type)];
}

} //namespace game