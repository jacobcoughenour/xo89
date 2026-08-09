#pragma once

#include "bn_optional.h"

namespace game {

class screen {
protected:
	screen() = default;

public:
	virtual ~screen() = default;
};

} //namespace game