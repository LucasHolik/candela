use ddc::Ddc;
use ddc_winapi::Monitor;
use std::ffi::c_void;
use windows::{
    core::*,
    Win32::Foundation::{BOOL, FALSE, LPARAM, RECT},
    Win32::Graphics::Gdi::{CreateDCW, DeleteDC, EnumDisplayMonitors, GetMonitorInfoW, HDC, HMONITOR, MONITORINFO, MONITORINFOEXW},
    Win32::UI::ColorSystem::SetDeviceGammaRamp,
};

#[derive(Copy, Clone)]
pub enum BrightnessMode {
    Software,
    Hardware,
}

pub struct BrightnessController {
    pub mode: BrightnessMode,
}

impl BrightnessController {
    pub fn new(mode: BrightnessMode) -> Self {
        Self { mode }
    }

    pub fn set_brightness(&self, brightness: u32) {
        match self.mode {
            BrightnessMode::Software => {
                let brightness_float = brightness as f32 / 100.0;
                set_software_brightness(brightness_float);
            }
            BrightnessMode::Hardware => {
                set_hardware_brightness(brightness);
            }
        }
    }
}

fn set_hardware_brightness(brightness: u32) {
    for mut monitor in Monitor::enumerate().unwrap() {
        let _ = monitor.set_vcp_feature(0x10, brightness as u16);
    }
}

fn set_software_brightness(brightness: f32) {
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
    hmonitor: HMONITOR,
    _hdc_monitor: HDC,
    _lprc_monitor: *mut RECT,
    lparam: LPARAM,
) -> BOOL {
    let monitors = unsafe { &mut *(lparam.0 as *mut Vec<String>) };

    let mut monitor_info_ex: MONITORINFOEXW = unsafe { std::mem::zeroed() };
    monitor_info_ex.monitorInfo.cbSize = std::mem::size_of::<MONITORINFOEXW>() as u32;

    unsafe {
        if GetMonitorInfoW(hmonitor, &mut monitor_info_ex as *mut _ as *mut MONITORINFO) == FALSE {
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
