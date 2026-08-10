#include "scenes/ship_scene.h"

namespace game {

ship_scene::ship_scene(shared_state &p_shared) : scene(p_shared) {
}

ship_scene::~ship_scene() {
}

bn::optional<scene_type> ship_scene::update() {
	bn::optional<scene_type> result;

	return result;
}

} // namespace game
