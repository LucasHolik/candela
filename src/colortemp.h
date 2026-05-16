#pragma once
#include <windows.h>

namespace ColorTempUtils
{
  constexpr int KELVIN_MIN = 1200;
  constexpr int KELVIN_MAX = 6500;
  constexpr int KELVIN_DEFAULT = 6500;

  /**
   * @brief Options controlling how the gamma ramp is built.
   *
   * Stages inside ApplyGammaRamp are applied in a strict order:
   *   1. Hardware brightness  (applied outside this ramp, via DDC/CI)
   *   2. Software brightness  (multiplicative on each ramp entry)
   *   3. Colour temperature   (per-channel R/G/B multipliers)
   *
   * Future per-channel filters (e.g. invert) can be added as fields here
   * without changing the signature. Filters that need cross-channel data
   * (e.g. grayscale) cannot be expressed in a gamma LUT — those live in
   * a separate module (see bwfilter.h).
   */
  struct GammaRampOptions
  {
    int brightness = 100; // Software brightness, 1..100
    int kelvin = 6500;    // Colour temperature, 1200..6500
  };

  void KelvinToRGB(int kelvin, double &r, double &g, double &b);
  bool ApplyGammaRamp(HDC hdc, const GammaRampOptions &opts);
}
