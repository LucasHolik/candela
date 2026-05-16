#include "bwfilter.h"
#include <windows.h>
#include <magnification.h>

// MinGW ships neither prototypes nor an import library for Magnification.dll,
// so we resolve the three entry points dynamically. The DLL is present on
// Windows 7 SP1 and later.
namespace
{
  using MagInitializeFn = BOOL(WINAPI *)();
  using MagUninitializeFn = BOOL(WINAPI *)();
  using MagSetFullscreenColorEffectFn = BOOL(WINAPI *)(PMAGCOLOREFFECT);

  HMODULE g_module = nullptr;
  MagInitializeFn g_init = nullptr;
  MagUninitializeFn g_uninit = nullptr;
  MagSetFullscreenColorEffectFn g_setEffect = nullptr;

  bool g_initialized = false;
  bool g_enabled = false;

  // Column 0..2 of each row holds the BT.601 luminance coefficient for that
  // row's input channel; alpha and constant columns are identity so the rest
  // of the pixel passes through unchanged.
  MAGCOLOREFFECT kGrayscale = {{
      {0.299f, 0.299f, 0.299f, 0.0f, 0.0f},
      {0.587f, 0.587f, 0.587f, 0.0f, 0.0f},
      {0.114f, 0.114f, 0.114f, 0.0f, 0.0f},
      {0.0f, 0.0f, 0.0f, 1.0f, 0.0f},
      {0.0f, 0.0f, 0.0f, 0.0f, 1.0f},
  }};

  MAGCOLOREFFECT kIdentity = {{
      {1.0f, 0.0f, 0.0f, 0.0f, 0.0f},
      {0.0f, 1.0f, 0.0f, 0.0f, 0.0f},
      {0.0f, 0.0f, 1.0f, 0.0f, 0.0f},
      {0.0f, 0.0f, 0.0f, 1.0f, 0.0f},
      {0.0f, 0.0f, 0.0f, 0.0f, 1.0f},
  }};

  bool LoadEntryPoints()
  {
    if (g_module)
      return true;
    g_module = LoadLibraryW(L"Magnification.dll");
    if (!g_module)
      return false;

    // Double-cast through void* keeps -Wcast-function-type quiet without
    // hiding genuine type mismatches at the call sites below.
    g_init = reinterpret_cast<MagInitializeFn>(
        reinterpret_cast<void *>(GetProcAddress(g_module, "MagInitialize")));
    g_uninit = reinterpret_cast<MagUninitializeFn>(
        reinterpret_cast<void *>(GetProcAddress(g_module, "MagUninitialize")));
    g_setEffect = reinterpret_cast<MagSetFullscreenColorEffectFn>(
        reinterpret_cast<void *>(GetProcAddress(g_module, "MagSetFullscreenColorEffect")));

    if (!g_init || !g_uninit || !g_setEffect)
    {
      FreeLibrary(g_module);
      g_module = nullptr;
      g_init = nullptr;
      g_uninit = nullptr;
      g_setEffect = nullptr;
      return false;
    }
    return true;
  }
}

namespace BWFilter
{
  bool Initialize()
  {
    if (g_initialized)
      return true;
    if (!LoadEntryPoints())
      return false;
    if (!g_init())
      return false;
    g_initialized = true;
    return true;
  }

  void Cleanup()
  {
    if (!g_initialized)
      return;
    if (g_setEffect)
      g_setEffect(&kIdentity);
    if (g_uninit)
      g_uninit();
    g_initialized = false;
    g_enabled = false;

    if (g_module)
    {
      FreeLibrary(g_module);
      g_module = nullptr;
      g_init = nullptr;
      g_uninit = nullptr;
      g_setEffect = nullptr;
    }
  }

  bool SetEnabled(bool enabled)
  {
    if (!g_initialized && !Initialize())
      return false;
    if (!g_setEffect(enabled ? &kGrayscale : &kIdentity))
      return false;
    g_enabled = enabled;
    return true;
  }

  bool IsEnabled()
  {
    return g_enabled;
  }
}
