#pragma once
#include <windows.h>

namespace ColorTempUtils
{
  constexpr int KELVIN_MIN = 1200;
  constexpr int KELVIN_MAX = 6500;
  constexpr int KELVIN_DEFAULT = 6500;

  void KelvinToRGB(int kelvin, double &r, double &g, double &b);
  bool ApplyGammaRamp(HDC hdc, int brightness, int kelvin);
}
