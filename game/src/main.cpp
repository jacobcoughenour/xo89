#include "bn_core.h"
#include "bn_memory.h"

#include "scene.h"
#include "scene_type.h"
#include "game_state.h"

#include "flying_scene.h"

#include "bn_common.h"
#include "bn_memory.h"


int main() {
	bn::core::init();

	bn::unique_ptr<game::scene> current_scene;
	bn::optional<game::scene_type> next_scene = game::scene_type::FLYING;
	bn::unique_ptr<game::game_state> game_state(new game::game_state());

	// main loop
	while (true) {
		// scene management
		if (current_scene) {
			next_scene = current_scene->update();
		}
		if (next_scene) {
			// change scenes
			if (current_scene) {
				current_scene.reset();
			} else {
				switch (*next_scene) {
					// case game::scene_type::SPLASH:
					// 	current_scene.reset(new game::nostabyte_splash_scene());
					// 	break;
					// case game::scene_type::MAIN_MENU:
					// 	current_scene.reset(new game::main_menu_scene(*shared));
					// 	break;
					case game::scene_type::FLYING:
						current_scene.reset(new game::flying_scene(*game_state));
					default:
						break;
				}
			}
		}

		game_state->update();
		bn::core::update();
	}
}
