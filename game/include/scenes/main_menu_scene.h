#pragma once

#include "scene.h"

#include "bn_affine_bg_ptr.h"
#include "bn_affine_bg_tiles_ptr.h"
#include "bn_bg_palette_ptr.h"
#include "bn_regular_bg_ptr.h"
#include "bn_sound_handle.h"
#include "bn_sprite_item.h"
#include "bn_sprite_ptr.h"
#include "bn_sprite_text_generator.h"
#include "bn_sprite_tiles_ptr.h"

namespace game {

class main_menu_scene : public scene {
public:
	explicit main_menu_scene(shared_state &p_shared);
	~main_menu_scene();
	[[nodiscard]] bn::optional<scene_type> update() final;

private:
	int _frame = 0;

	enum main_menu_option {
		NEW_GAME,
		LOAD_GAME,
		CREDITS
	};

	main_menu_option _selected_menu = main_menu_option::NEW_GAME;
	bool _option_selected = false;

	bn::regular_bg_ptr _logo_bg;

	bn::sprite_text_generator _small_text;
	bn::sprite_text_generator _small_var_text;
	bn::vector<bn::sprite_ptr, 80> _text_sprites;

	bn::sprite_ptr _ship_sprite;
};

} //namespace game