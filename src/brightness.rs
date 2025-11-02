use ddc::Ddc;
use ddc_winapi::Monitor;

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

    pub fn set_brightness(&self, brightness: u32, software_brightness_fn: fn(f32)) {
        match self.mode {
            BrightnessMode::Software => {
                let brightness_float = brightness as f32 / 100.0;
                software_brightness_fn(brightness_float);
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
