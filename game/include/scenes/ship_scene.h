#include "scene.h"

#include "bn_bg_palettes.h"
#include "bn_cameras.h"
#include "bn_direct_bitmap_item.h"
#include "bn_keypad.h"
#include "bn_sp_direct_bitmap_bg_painter.h"
#include "bn_sprite_ptr.h"
#include "bn_sprite_text_generator.h"

namespace game {

class ship_scene : public scene {
public:
	explicit ship_scene(shared_state &p_shared);
	~ship_scene();
	[[nodiscard]] bn::optional<scene_type> update() final;

private:
	bn::camera_ptr _camera;
	bn::sprite_text_generator _small_text;
	bn::sp_direct_bitmap_bg_ptr _pano_bg;
	bn::sprite_ptr _test_sprite;
	int _rotation = 1024 / 2;
};
} //namespace game
