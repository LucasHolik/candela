use eframe::egui;

pub struct CandelaGui {
    // Add fields here for state
}

impl Default for CandelaGui {
    fn default() -> Self {
        Self {}
    }
}

impl eframe::App for CandelaGui {
    fn update(&mut self, ctx: &egui::Context, _frame: &mut eframe::Frame) {
        egui::CentralPanel::default().show(ctx, |ui| {
            ui.heading("Candela");
        });
    }
}
