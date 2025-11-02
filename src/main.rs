#![windows_subsystem = "windows"]

use std::ffi::c_void;
use windows::{
    core::*,
    Win32::Foundation::*,
    Win32::Graphics::Gdi::*,
    Win32::UI::ColorSystem::*,
    Win32::UI::WindowsAndMessaging::*,
    Win32::UI::Shell::*,
    Win32::System::LibraryLoader::*,
};

const WM_TRAYICON: u32 = WM_APP + 1;
const ID_EXIT: u32 = 1;
const ID_BRIGHTNESS_25: u32 = 2;
const ID_BRIGHTNESS_50: u32 = 3;
const ID_BRIGHTNESS_75: u32 = 4;
const ID_BRIGHTNESS_100: u32 = 5;
const ID_RESET_BRIGHTNESS: u32 = 6;

fn main() -> Result<()> {
    unsafe {
        let instance = GetModuleHandleW(None)?.into();
        let class_name = w!("CandelaTrayWindowClass");

        let wc = WNDCLASSW {
            lpfnWndProc: Some(wnd_proc),
            lpszClassName: class_name,
            hInstance: instance,
            ..Default::default()
        };

        RegisterClassW(&wc);

        let hwnd = CreateWindowExW(
            WINDOW_EX_STYLE::default(),
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

        let mut nid = NOTIFYICONDATAW {
            cbSize: std::mem::size_of::<NOTIFYICONDATAW>() as u32,
            hWnd: hwnd,
            uID: 1,
            uFlags: NIF_ICON | NIF_MESSAGE | NIF_TIP,
            uCallbackMessage: WM_TRAYICON,
            hIcon: LoadIconW(None, IDI_APPLICATION)?,
            ..Default::default()
        };
        let tip = w!("Candela");
        nid.szTip[..tip.as_wide().len()].copy_from_slice(tip.as_wide());

        Shell_NotifyIconW(NIM_ADD, &nid);

        let mut msg = MSG::default();
        while GetMessageW(&mut msg, None, 0, 0).into() {
            TranslateMessage(&msg);
            DispatchMessageW(&msg);
        }

        Shell_NotifyIconW(NIM_DELETE, &nid);
    }

    Ok(())
}

extern "system" fn wnd_proc(hwnd: HWND, msg: u32, wparam: WPARAM, lparam: LPARAM) -> LRESULT {
    match msg {
        WM_TRAYICON => {
            if lparam.0 as u32 == WM_RBUTTONUP {
                let mut point = POINT::default();
                unsafe { let _ = GetCursorPos(&mut point); };

                let hmenu = unsafe { CreatePopupMenu() }.unwrap();
                unsafe {
                    let _ = AppendMenuW(hmenu, MF_STRING, ID_BRIGHTNESS_25 as usize, w!("25%"));
                    let _ = AppendMenuW(hmenu, MF_STRING, ID_BRIGHTNESS_50 as usize, w!("50%"));
                    let _ = AppendMenuW(hmenu, MF_STRING, ID_BRIGHTNESS_75 as usize, w!("75%"));
                    let _ = AppendMenuW(hmenu, MF_STRING, ID_BRIGHTNESS_100 as usize, w!("100%"));
                    let _ = AppendMenuW(hmenu, MF_STRING, ID_RESET_BRIGHTNESS as usize, w!("Reset"));
                    let _ = AppendMenuW(hmenu, MF_SEPARATOR, 0, None);
                    let _ = AppendMenuW(hmenu, MF_STRING, ID_EXIT as usize, w!("Exit"));
                }

                unsafe {
                    let _ = SetForegroundWindow(hwnd);
                    let _ = TrackPopupMenu(
                        hmenu,
                        TPM_RIGHTALIGN | TPM_BOTTOMALIGN,
                        point.x,
                        point.y,
                        0,
                        hwnd,
                        None,
                    );
                }
            }
            LRESULT(0)
        }
        WM_COMMAND => {
            match wparam.0 as u32 {
                ID_EXIT => {
                    unsafe { PostQuitMessage(0) };
                }
                ID_BRIGHTNESS_25 => set_brightness(0.25),
                ID_BRIGHTNESS_50 => set_brightness(0.5),
                ID_BRIGHTNESS_75 => set_brightness(0.75),
                ID_BRIGHTNESS_100 => set_brightness(1.0),
                ID_RESET_BRIGHTNESS => set_brightness(1.0),
                _ => (),
            }
            LRESULT(0)
        }
        _ => unsafe { DefWindowProcW(hwnd, msg, wparam, lparam) },
    }
}

fn set_brightness(brightness: f32) {
    let mut monitors: Vec<String> = Vec::new();
    let lparam = LPARAM(&mut monitors as *mut _ as isize);

    unsafe {
        if EnumDisplayMonitors(
            HDC::default(),
            None,
            Some(monitor_enum_proc_for_set_brightness),
            lparam,
        ) == FALSE {
            // Handle error, maybe log it
        };
    }

    for device_name in monitors {
        let device_name_utf16: Vec<u16> = device_name.encode_utf16().chain(std::iter::once(0)).collect();
        let hdc = unsafe { CreateDCW(PCWSTR(device_name_utf16.as_ptr()), PCWSTR::null(), PCWSTR::null(), None) };
        if hdc.is_invalid() {
            continue;
        }

        let mut ramp: [u16; 256 * 3] = [0; 256 * 3];
        for i in 0..256 {
            let value = (i as f32 * brightness * 255.0) as u16;
            ramp[i] = value;
            ramp[i + 256] = value;
            ramp[i + 512] = value;
        }

        unsafe {
            let _ = SetDeviceGammaRamp(hdc, ramp.as_ptr() as *const c_void);
            let _ = DeleteDC(hdc);
        }
    }
}

extern "system" fn monitor_enum_proc_for_set_brightness(
    _hmonitor: HMONITOR,
    _hdc_monitor: HDC,
    _lprc_monitor: *mut RECT,
    lparam: LPARAM,
) -> BOOL {
    let monitors = unsafe { &mut *(lparam.0 as *mut Vec<String>) };

    let mut monitor_info_ex: MONITORINFOEXW = unsafe { std::mem::zeroed() };
    monitor_info_ex.monitorInfo.cbSize = std::mem::size_of::<MONITORINFOEXW>() as u32;

    unsafe {
        if GetMonitorInfoW(_hmonitor, &mut monitor_info_ex as *mut _ as *mut MONITORINFO) == FALSE {
            return BOOL(1);
        }
    }

    let device_name = String::from_utf16_lossy(
        &monitor_info_ex.szDevice[..]
            .iter()
            .take_while(|&&c| c != 0)
            .copied()
            .collect::<Vec<u16>>(),
    );

    monitors.push(device_name);

    BOOL(1) // Continue enumeration
}