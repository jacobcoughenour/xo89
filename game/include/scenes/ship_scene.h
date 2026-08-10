#include "scene.h"

namespace game {

class ship_scene : public scene {
public:
	explicit ship_scene(shared_state &p_shared);
	~ship_scene();
	[[nodiscard]] bn::optional<scene_type> update() final;
};
} //namespace game
