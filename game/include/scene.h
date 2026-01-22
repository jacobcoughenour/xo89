#ifndef SCENE_H
#define SCENE_H

#include "bn_optional.h"

namespace game {

enum class scene_type;

class scene {
protected:
	scene() = default;

public:
	virtual ~scene() = default;
	// return the scene you want to switch to
	[[nodiscard]] virtual bn::optional<scene_type> update() = 0;
};

} //namespace game
#endif