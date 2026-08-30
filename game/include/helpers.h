#pragma once

#include "bn_color.h"
#include "bn_fixed.h"
#include "bn_fixed_point.h"
#include "bn_math.h"
#include "bn_point.h"
#include "bn_sstream.h"
#include "bn_string.h"

namespace game {

class helpers {
public:
	static inline const bn::fixed lerp_fixed(bn::fixed a, bn::fixed b, bn::fixed t) {
		if (abs(a - b) < 1) {
			return b;
		}
		return a + ((b - a) * t);
	}

	static inline const bn::fixed_point lerp_fixed_point(bn::fixed_point a, bn::fixed_point b, bn::fixed t) {
		return a + ((b - a) * t);
	}

	static inline const bn::fixed remap_fixed(bn::fixed x, bn::fixed in_start, bn::fixed in_end, bn::fixed out_start, bn::fixed out_end) {
		return bn::clamp(out_start + ((x - in_start) * (out_end - out_start)) / (in_end - in_start), bn::min(out_start, out_end), bn::max(out_start, out_end));
	}

	static inline int lerp_int(int a, int b, bn::fixed t) {
		return int(a + (bn::fixed(b - a) * t));
	}

	static inline const bn::color lerp_color(bn::color a, bn::color b, bn::fixed t) {
		return bn::color(lerp_int(a.red(), b.red(), t), lerp_int(a.green(), b.green(), t), lerp_int(a.blue(), b.blue(), t));
	}

	static inline int truncate(bn::fixed x) {
		return x < 0 ? -(-x).floor_integer() : x.floor_integer();
	}

	static inline const bn::fixed fmod(bn::fixed x, bn::fixed y) {
		return x - truncate(x / y) * y;
	}

	static inline const bn::fixed fmod1(bn::fixed x) {
		return x - truncate(x);
	}

	static inline const bn::fixed fposmod(bn::fixed x, bn::fixed y) {
		bn::fixed value = fmod(x, y);
		if (((value < 0) && (y > 0)) || ((value > 0) && (y < 0))) {
			value += y;
		}
		return value;
	}

	static inline bn::fixed fposmod1(bn::fixed x) {
		bn::fixed value = fmod1(x);
		if (value < 0) {
			value += 1;
		}
		return value;
	}

	static inline int posmod(int x, int y) {
		int value = x % y;
		if (((value < 0) && (y > 0)) || ((value > 0) && (y < 0))) {
			value += y;
		}
		return value;
	}

	static inline const bn::fixed_point rad_to_dir(bn::fixed p_rad) {
		p_rad /= PI_2;
		return bn::fixed_point(bn::sin(p_rad), bn::cos(p_rad));
	}

	static inline const bn::fixed_point angle_to_dir(bn::fixed p_angle) {
		const auto p = bn::degrees_lut_sin_and_cos_safe(p_angle);
		return bn::fixed_point(p.first, p.second);
	}

	static inline const bn::fixed dir_to_angle_deg(bn::fixed_point p_point) {
		return bn::degrees_atan2(p_point.x().integer(), p_point.y().integer());
	}

	static inline bool is_deg_within_range(bn::fixed p_a, bn::fixed p_b, bn::fixed p_range) {
		p_a = fposmod(p_a, 360);
		p_b = fposmod(p_b, 360);
		auto diff = bn::abs(p_a - p_b);
		if (diff > 180) {
			return 360 - diff < p_range;
		}
		return diff <= p_range;
	}

	static constexpr double PI = 3.1415926535897932384626433832795;
	static constexpr double PI_2 = 3.1415926535897932384626433832795 * 2.0;

	static inline bn::fixed point_length(bn::fixed_point point) {
		return bn::sqrt(point.x() * point.x() + point.y() * point.y());
	}

	static inline const bn::fixed_point normalize_point(bn::fixed_point point) {
		bn::fixed d = point_length(point);
		if (d == 0) {
			return bn::fixed_point(0, 0);
		}
		return point / d;
	}

	static inline bn::fixed distance(bn::fixed_point a, bn::fixed_point b) {
		return point_length(a - b);
	}

	static inline bn::fixed_point set_length(bn::fixed_point a, bn::fixed length) {
		return normalize_point(a) * length;
	}

	// bounding box test between two points.
	// this is faster than doing a real distance check.
	static inline bool box_dist_test(bn::fixed_point a, bn::fixed_point b, bn::fixed dist) {
		return bn::abs(a.x() - b.x()) <= dist && bn::abs(a.y() - b.y()) <= dist;
	}

	static inline const bn::fixed max_box_dist(bn::fixed_point a, bn::fixed_point b) {
		return bn::max(bn::abs(a.x() - b.x()), bn::abs(a.y() - b.y()));
	}

	static inline int get_digit(int num, int pos) {
		if (num == 0) {
			return 0;
		}
		int temp = num;
		while (temp > 0 && pos > 0) {
			temp /= 10;
			pos--;
		}
		return temp % 10;
	}

	static inline int set_digit(int num, int pos, int digit) {
		int before = 1;
		for (int i = 0; i < pos + 1; i++) {
			before *= 10;
		}
		before = num / before;
		int after = 1;
		for (int i = 0; i < pos; i++) {
			after *= 10;
		}
		after = num % after;

		for (int i = 0; i < pos + 1; i++) {
			before *= 10;
		}
		for (int i = 0; i < pos; i++) {
			digit *= 10;
		}
		return before + digit + after;
	}

	static inline void append_with_padding(bn::ostringstream &stream, int num, int length, char padChar) {
		int num2 = num;
		int digits = 0;
		if (num == 0) {
			digits = 1;
		} else {
			while (num2 > 0) {
				num2 /= 10;
				digits++;
			}
		}
		for (int i = 0; i < length - digits; i++) {
			stream.append(padChar);
		}
		stream.append(num);
	}

	static inline int tile_pos_to_index(int p_x, int p_y, int p_columns) {
		return p_x + p_y * p_columns;
	}

	static inline int relative_tile_index(int p_from_index, int p_x, int p_y, int p_tileset_columns) {
		return p_from_index + p_x + p_y * p_tileset_columns;
	}

	static inline bool is_point_in_view(bn::fixed_point camera_pos, bn::fixed_point point, bn::fixed size) {
		// assuming gba screen size here
		bn::fixed left = camera_pos.x() - 120;
		bn::fixed right = camera_pos.x() + 120;
		bn::fixed top = camera_pos.y() - 80;
		bn::fixed bottom = camera_pos.y() + 80;

		if (point.x() + size < left) {
			return false;
		}
		if (point.x() > right) {
			return false;
		}
		if (point.y() + size < top) {
			return false;
		}
		if (point.y() > bottom) {
			return false;
		}
		return true;
	}
};

} //namespace game