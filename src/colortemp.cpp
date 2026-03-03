#include "colortemp.h"
#include "brightness.h"
#include <cmath>
#include <algorithm>

namespace ColorTempUtils
{

  void KelvinToRGB(int kelvin, double &r, double &g, double &b)
  {
    kelvin = std::max(KELVIN_MIN, std::min(kelvin, KELVIN_MAX));
    double temp = kelvin / 100.0;

    // Red
    if (temp <= 66.0)
      r = 1.0;
    else
      r = 329.698727446 * std::pow(temp - 60.0, -0.1332047592) / 255.0;

    // Green
    if (temp <= 66.0)
      g = (99.4708025861 * std::log(temp) - 161.1195681661) / 255.0;
    else
      g = 288.1221695283 * std::pow(temp - 60.0, -0.0755148492) / 255.0;

    // Blue
    if (temp >= 66.0)
      b = 1.0;
    else if (temp <= 19.0)
      b = 0.0;
    else
      b = (138.5177312231 * std::log(temp - 10.0) - 305.0447927307) / 255.0;

    // Clamp all channels to [0.0, 1.0]
    r = std::max(0.0, std::min(r, 1.0));
    g = std::max(0.0, std::min(g, 1.0));
    b = std::max(0.0, std::min(b, 1.0));
  }

  bool ApplyGammaRamp(HDC hdc, int brightness, int kelvin)
  {
    if (!hdc)
      return false;

    double r, g, b;
    KelvinToRGB(kelvin, r, g, b);

    double factor = MapBrightnessToSafeFactor(brightness) / 100.0;

    WORD ramp[256 * 3];
    for (int i = 0; i < 256; i++)
    {
      double base = i * factor * 257.0;
      ramp[i] = (WORD)std::max(0.0, std::min(base * r, 65535.0));
      ramp[i + 256] = (WORD)std::max(0.0, std::min(base * g, 65535.0));
      ramp[i + 512] = (WORD)std::max(0.0, std::min(base * b, 65535.0));
    }

    return SetDeviceGammaRamp(hdc, ramp) != FALSE;
  }

} // namespace ColorTempUtils
