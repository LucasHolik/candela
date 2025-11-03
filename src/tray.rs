use windows::{
    core::*,
    Win32::Foundation::{HWND, LPARAM, LRESULT, POINT, WPARAM},
    Win32::UI::WindowsAndMessaging::{
        AppendMenuW, CreatePopupMenu, DefWindowProcW,
        GetCursorPos, PostQuitMessage,
        SetForegroundWindow, TrackPopupMenu, MF_SEPARATOR, MF_STRING, TPM_BOTTOMALIGN, TPM_RIGHTALIGN, WM_APP, WM_COMMAND,
        WM_RBUTTONUP,
    },
    Win32::UI::Shell::{
        Shell_NotifyIconW, NOTIFYICONDATAW, NIF_ICON, NIF_MESSAGE, NIF_TIP, NIM_ADD, NIM_DELETE,
    },
};

use crate::brightness::{BrightnessController, BrightnessMode};

const WM_TRAYICON: u32 = WM_APP + 1;
const ID_EXIT: u32 = 1;
const ID_BRIGHTNESS_25: u32 = 2;
const ID_BRIGHTNESS_50: u32 = 3;
const ID_BRIGHTNESS_75: u32 = 4;
const ID_BRIGHTNESS_100: u32 = 5;
const ID_RESET_BRIGHTNESS: u32 = 6;
const ID_TOGGLE_MODE: u32 = 7;

static mut CURRENT_MODE: BrightnessMode = BrightnessMode::Software;

pub fn create_tray(hwnd: HWND) -> Result<()> {
    let mut nid = NOTIFYICONDATAW {
        cbSize: std::mem::size_of::<NOTIFYICONDATAW>() as u32,
        hWnd: hwnd,
        uID: 1,
        uFlags: NIF_ICON | NIF_MESSAGE | NIF_TIP,
        uCallbackMessage: WM_TRAYICON,
        hIcon: unsafe { windows::Win32::UI::WindowsAndMessaging::LoadIconW(None, windows::Win32::UI::WindowsAndMessaging::IDI_APPLICATION)? },
        ..Default::default()
    };
    let tip = w!("Candela");
    unsafe {
        let tip_wide = tip.as_wide();
        nid.szTip[..tip_wide.len()].copy_from_slice(tip_wide);
    }

    unsafe { Shell_NotifyIconW(NIM_ADD, &nid) };

    Ok(())
}

pub fn remove_tray(hwnd: HWND) {
    let nid = NOTIFYICONDATAW {
        cbSize: std::mem::size_of::<NOTIFYICONDATAW>() as u32,
        hWnd: hwnd,
        uID: 1,
        ..Default::default()
    };
    unsafe { Shell_NotifyIconW(NIM_DELETE, &nid) };
}

pub extern "system" fn wnd_proc(hwnd: HWND, msg: u32, wparam: WPARAM, lparam: LPARAM) -> LRESULT {
    match msg {
        WM_TRAYICON => {
            if lparam.0 as u32 == WM_RBUTTONUP {
                let mut point = POINT::default();
                unsafe {
                    let _ = GetCursorPos(&mut point);
                };

                let hmenu = unsafe { CreatePopupMenu() }.unwrap();
                unsafe {
                    let _ = AppendMenuW(hmenu, MF_STRING, ID_BRIGHTNESS_25 as usize, w!("25%"));
                    let _ = AppendMenuW(hmenu, MF_STRING, ID_BRIGHTNESS_50 as usize, w!("50%"));
                    let _ = AppendMenuW(hmenu, MF_STRING, ID_BRIGHTNESS_75 as usize, w!("75%"));
                    let _ = AppendMenuW(hmenu, MF_STRING, ID_BRIGHTNESS_100 as usize, w!("100%"));
                    let _ = AppendMenuW(hmenu, MF_STRING, ID_RESET_BRIGHTNESS as usize, w!("Reset"));
                    let _ = AppendMenuW(hmenu, MF_SEPARATOR, 0, None);
                    let _ = AppendMenuW(hmenu, MF_STRING, ID_TOGGLE_MODE as usize, w!("Toggle Mode"));
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
            let controller = unsafe { BrightnessController::new(CURRENT_MODE) };
            match wparam.0 as u32 {
                ID_EXIT => {
                    unsafe { PostQuitMessage(0) };
                }
                ID_TOGGLE_MODE => unsafe {
                    match CURRENT_MODE {
                        BrightnessMode::Software => {
                            CURRENT_MODE = BrightnessMode::Hardware;
                            // Reset software brightness to full
                            let software_controller = BrightnessController::new(BrightnessMode::Software);
                            software_controller.set_brightness(100);
                        }
                        BrightnessMode::Hardware => {
                            CURRENT_MODE = BrightnessMode::Software;
                            // Reset hardware brightness to full
                            let hardware_controller = BrightnessController::new(BrightnessMode::Hardware);
                            hardware_controller.set_brightness(100);
                        }
                    }
                },
                ID_BRIGHTNESS_25 => controller.set_brightness(25),
                ID_BRIGHTNESS_50 => controller.set_brightness(50),
                ID_BRIGHTNESS_75 => controller.set_brightness(75),
                ID_BRIGHTNESS_100 => controller.set_brightness(100),
                ID_RESET_BRIGHTNESS => controller.set_brightness(100),
                _ => (),
            }
            LRESULT(0)
        }
        _ => unsafe { DefWindowProcW(hwnd, msg, wparam, lparam) },
    }
}