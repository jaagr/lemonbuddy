#pragma once

#include <cstdlib>

#include "common.hpp"

POLYBAR_NS

/**
 * Represents immutable 32-bit color values.
 */
class rgba {
 public:
  enum color_type { NONE, ARGB, ALPHA_ONLY };

  explicit rgba();
  explicit rgba(uint32_t value, color_type type = ARGB);
  explicit rgba(string hex);

  operator string() const;
  operator uint32_t() const;
  bool operator==(const rgba& other) const;

  uint32_t value() const;
  color_type type() const;

  double a() const;
  double r() const;
  double g() const;
  double b() const;

  uint8_t a_int() const;
  uint8_t r_int() const;
  uint8_t g_int() const;
  uint8_t b_int() const;

  bool has_color() const;
  rgba apply_alpha(rgba other) const;
  rgba try_apply_alpha(rgba other) const;

 private:
  /**
   * Color value in the form ARGB or A000 depending on the type
   *
   * Cannot be const because we have to assign to it in the constructor and initializer lists are not possible.
   */
  uint32_t m_value;

  /**
   * NONE marks this instance as invalid. If such a color is encountered, it
   * should be treated as if no color was set.
   *
   * ALPHA_ONLY is used for color strings that only have an alpha channel (#AA)
   * these kinds should be combined with another color that has RGB channels
   * before they are used to render anything.
   *
   * Cannot be const because we have to assign to it in the constructor and initializer lists are not possible.
   */
  color_type m_type{NONE};
};

namespace color_util {
  string simplify_hex(string hex);
}  // namespace color_util

POLYBAR_NS_END
