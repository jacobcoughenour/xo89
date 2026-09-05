#pragma once

#include "scene.h"

#include "bn_affine_bg_ptr.h"
#include "bn_affine_bg_tiles_ptr.h"
#include "bn_bg_palette_ptr.h"
#include "bn_regular_bg_ptr.h"
#include "bn_sound_handle.h"
#include "bn_sprite_item.h"
#include "bn_sprite_ptr.h"
#include "bn_sprite_tiles_ptr.h"

namespace game {

class splash_scene : public scene {
public:
	explicit splash_scene(shared_state &p_shared);
	~splash_scene();
	[[nodiscard]] bn::optional<scene_type> update() final;

private:
	int _frame = 0;

	bn::sprite_ptr _nos_sprite;
	bn::sprite_ptr _ta_sprite;
	bn::sprite_ptr _byt_sprite;
	bn::sprite_ptr _e_sprite;
	bn::sprite_ptr _interac_sprite;
	bn::sprite_ptr _tive_sprite;

	bn::sound_handle _splash_sound;

	bn::regular_bg_ptr _jam_bg;

	void _animate_up(bn::sprite_ptr sprite, int end_frame, int duration_frames);
	void _animate_right(bn::sprite_ptr sprite, int end_frame, int duration_frames, int end_x);
};

} //namespace game