#![windows_subsystem = "windows"]

mod brightness;
mod tray;
mod gui;

use windows::{
    core::*,
    Win32::Foundation::HWND,
    Win32::UI::WindowsAndMessaging::{
        CreateWindowExW, DispatchMessageW, GetMessageW, RegisterClassW,
        TranslateMessage, CW_USEDEFAULT, MSG, WNDCLASSW, WS_OVERLAPPEDWINDOW,
    },
    Win32::System::LibraryLoader::GetModuleHandleW,
};

use brightness::BrightnessMode;

pub struct AppState {
    pub brightness_mode: BrightnessMode,
}

impl AppState {
    pub fn new() -> Self {
        Self {
            brightness_mode: BrightnessMode::Software,
        }
    }
}

// Global variable to track the slider window
pub static mut SLIDER_WINDOW: HWND = HWND(0);

fn main() -> Result<()> {
    unsafe {
        let instance = GetModuleHandleW(None)?.into();
        let class_name = w!("CandelaTrayWindowClass");

        let app_state = Box::new(AppState::new());
        let app_state_ptr = Box::into_raw(app_state);

        let wc = WNDCLASSW {
            lpfnWndProc: Some(tray::wnd_proc),
            lpszClassName: class_name,
            hInstance: instance,
            // Pass the app_state_ptr as the lpfnWndProc parameter
            cbWndExtra: std::mem::size_of::<*mut AppState>() as i32,
            ..Default::default()
        };

        RegisterClassW(&wc);

        let hwnd = CreateWindowExW(
            Default::default(),
            class_name,
            w!("Candela"),
            WS_OVERLAPPEDWINDOW,
            CW_USEDEFAULT,
            CW_USEDEFAULT,
            CW_USEDEFAULT,
            CW_USEDEFAULT,
            None,
            None,
            instance,
            Some(app_state_ptr as *mut std::ffi::c_void),
        );

        tray::create_tray(hwnd)?;

        let mut msg = MSG::default();
        while GetMessageW(&mut msg, None, 0, 0).into() {
            TranslateMessage(&msg);
            DispatchMessageW(&msg);
        }

        tray::remove_tray(hwnd);

        // Clean up the AppState when the application exits
        let _ = Box::from_raw(app_state_ptr);
    }

    Ok(())
}