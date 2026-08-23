#include "bn_common.h"
#include "bn_core.h"
#include "bn_memory.h"
#include "bn_optional.h"

#include "state/mining_state.h"
#include "state/shared_state.h"

#include "scenes/scene.h"
#include "scenes/scene_type.h"

#include "bn_log.h"
#include "scenes/mining_scene.h"
#include "scenes/ship_scene.h"
#include "scenes/splash_scene.h"

#include "tests.h"

#include "bn_sram.h"

int main() {
	bn::core::init();

	// todo don't include in release
	game::tests::run_tests();

	bn::unique_ptr<game::scene> current_scene;
	bn::optional<game::scene_type> next_scene_type = game::scene_type::SHIP;

	bn::unique_ptr<game::shared_state> shared_state(new game::shared_state());

	bn::unique_ptr<game::mining_state> _mining_state(nullptr);

	BN_LOG("sram size: ", bn::sram::size());

	// main loop
	while (true) {
		// scene management
		if (current_scene) {
			next_scene_type = current_scene->update();
		}
		if (next_scene_type) {
			BN_LOG("changing to scene ", static_cast<int>(next_scene_type.value()));

			if (next_scene_type == game::scene_type::MINING) {
				_mining_state.reset(new game::mining_state(*shared_state));
			} else {
				_mining_state.reset(nullptr);
			}

			if (current_scene) {
				// cleanup current scene
				current_scene.reset();
				bn::core::update();
			}

			// change scenes
			switch (*next_scene_type) {
				// case game::scene_type::SPLASH:
				// 	current_scene.reset(new game::nostabyte_splash_scene());
				// 	break;
				// case game::scene_type::MAIN_MENU:
				// 	current_scene.reset(new game::main_menu_scene(*shared));
				// 	break;
				case game::scene_type::SHIP:
					current_scene.reset(new game::ship_scene(*shared_state));
					break;
				case game::scene_type::MINING:
					current_scene.reset(new game::mining_scene(*shared_state, *_mining_state));
					break;
				default:
					break;
			}

			next_scene_type.reset();
		}

		shared_state->update();
		bn::core::update();
	}
}
