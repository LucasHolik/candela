#pragma once

/**
 * @brief System-wide black & white (grayscale) screen filter.
 *
 * Uses the Windows Magnification API (MagSetFullscreenColorEffect) to apply a
 * grayscale colour matrix to the entire desktop. This is the same mechanism
 * Windows' built-in Colour Filters accessibility feature uses, and is the only
 * supported way to get *real* per-pixel grayscale on Windows — gamma ramps
 * cannot do it because each channel's LUT only sees its own channel.
 *
 * The effect is necessarily global (all monitors); there is no per-monitor
 * equivalent in this API.
 */
namespace BWFilter
{
  /**
   * @brief Initialises the Magnification runtime. Must succeed before
   *        SetEnabled has any effect. Safe to call multiple times.
   * @return true on success.
   */
  bool Initialize();

  /**
   * @brief Releases Magnification resources. Disables the filter first so
   *        the desktop returns to normal colour on shutdown.
   */
  void Cleanup();

  /**
   * @brief Applies or clears the grayscale colour effect.
   * @param enabled true to set the grayscale matrix; false to restore identity.
   * @return true if the effect was applied successfully.
   */
  bool SetEnabled(bool enabled);

  /**
   * @brief Returns the most recently applied state.
   */
  bool IsEnabled();
}
