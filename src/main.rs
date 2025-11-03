#![windows_subsystem = "windows"]

mod brightness;
mod tray;

use windows::{
    core::*,
    Win32::UI::WindowsAndMessaging::{
        CreateWindowExW, DispatchMessageW, GetMessageW, RegisterClassW,
        TranslateMessage, CW_USEDEFAULT, MSG, WNDCLASSW, WS_OVERLAPPEDWINDOW,
    },
    Win32::System::LibraryLoader::GetModuleHandleW,
};

fn main() -> Result<()> {
    unsafe {
        let instance = GetModuleHandleW(None)?.into();
        let class_name = w!("CandelaTrayWindowClass");

        let wc = WNDCLASSW {
            lpfnWndProc: Some(tray::wnd_proc),
            lpszClassName: class_name,
            hInstance: instance,
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
            None,
        );

        tray::create_tray(hwnd)?;

        let mut msg = MSG::default();
        while GetMessageW(&mut msg, None, 0, 0).into() {
            TranslateMessage(&msg);
            DispatchMessageW(&msg);
        }

        tray::remove_tray(hwnd);
    }

    Ok(())
}