use eframe::egui;
use eframe::{App, Frame, NativeOptions};
use std::sync::Arc;

// Import our plugin components
mod warrior_headphone;
mod titan7_mix_master;

use warrior_headphone::{HeadphonePlugin, HeadphoneParams};
use titan7_mix_master::{Titan7Plugin, Titan7Params};

struct PluginDemoApp {
    show_headphone: bool,
    show_titan7: bool,
    headphone_params: Arc<HeadphoneParams>,
    titan7_params: Arc<Titan7Params>,
}

impl PluginDemoApp {
    fn new() -> Self {
        Self {
            show_headphone: true,
            show_titan7: false,
            headphone_params: Arc::new(HeadphoneParams::default()),
            titan7_params: Arc::new(Titan7Params::default()),
        }
    }
}

impl App for PluginDemoApp {
    fn update(&mut self, ctx: &egui::Context, _frame: &mut Frame) {
        egui::TopBottomPanel::top("top_panel").show(ctx, |ui| {
            ui.horizontal(|ui| {
                ui.heading("WarriorPlug-Ins Demo");
                ui.separator();
                if ui.button("Warrior Headphone").clicked() {
                    self.show_headphone = true;
                    self.show_titan7 = false;
                }
                if ui.button("Titan7 Mix Master").clicked() {
                    self.show_headphone = false;
                    self.show_titan7 = true;
                }
            });
        });

        if self.show_headphone {
            self.show_warrior_headphone_ui(ctx);
        }
        
        if self.show_titan7 {
            self.show_titan7_ui(ctx);
        }
    }
}

impl PluginDemoApp {
    fn show_warrior_headphone_ui(&mut self, ctx: &egui::Context) {
        egui::CentralPanel::default().show(ctx, |ui| {
            ui.heading("🎧 Warrior Headphone Plugin");
            ui.separator();
            
            ui.label("Professional headphone calibration and enhancement plugin");
            ui.label("SKU: WH-01 | Price: $129 USD");
            
            ui.separator();
            
            // Parameters display (simulated UI)
            egui::Grid::new("headphone_grid").show(ui, |ui| {
                ui.label("Gain:");
                ui.add(egui::Slider::new(&mut 0.0f32, -24.0..=24.0).suffix(" dB"));
                ui.end_row();
                
                ui.label("Blend:");
                ui.add(egui::Slider::new(&mut 1.0f32, 0.0..=1.0));
                ui.end_row();
                
                ui.label("Crossfeed:");
                ui.add(egui::Slider::new(&mut 0.0f32, 0.0..=1.0));
                ui.end_row();
                
                ui.label("Model:");
                egui::ComboBox::from_label("")
                    .selected_text("HD800S")
                    .show_ui(ui, |ui| {
                        ui.selectable_value(&mut 0, 0, "HD800S");
                        ui.selectable_value(&mut 0, 1, "DT1990 Pro");
                        ui.selectable_value(&mut 0, 2, "LCD-X");
                    });
                ui.end_row();
                
                ui.label("Codec:");
                egui::ComboBox::from_label("")
                    .selected_text("None")
                    .show_ui(ui, |ui| {
                        ui.selectable_value(&mut 0, 0, "None");
                        ui.selectable_value(&mut 0, 1, "SBC");
                        ui.selectable_value(&mut 0, 2, "AAC");
                        ui.selectable_value(&mut 0, 3, "aptX");
                    });
                ui.end_row();
            });
            
            ui.separator();
            ui.checkbox(&mut false, "Ultra Low Latency Mode");
            
            // Feature highlights
            ui.separator();
            ui.label("Features:");
            ui.bullet_point("8-band parametric EQ with surgical precision");
            ui.bullet_point("AI-driven headphone correction (HD800S, DT1990 Pro, LCD-X)");
            ui.bullet_point("Crossfeed processing for spatial enhancement");
            ui.bullet_point("Bluetooth codec compensation (AAC, SBC, aptX)");
            ui.bullet_point("Ultra-low latency mode for live monitoring");
            ui.bullet_point("Professional studio reference presets");
        });
    }
    
    fn show_titan7_ui(&mut self, ctx: &egui::Context) {
        egui::CentralPanel::default().show(ctx, |ui| {
            ui.heading("🎛️ Titan7 Mix Master Plugin");
            ui.separator();
            
            ui.label("Professional mixing and mastering suite");
            ui.label("SKU: T7-MM-01 | Price: $199 USD");
            
            ui.separator();
            
            // Mode Selection
            ui.horizontal(|ui| {
                ui.label("Mode:");
                egui::ComboBox::from_label("")
                    .selected_text("Mixing")
                    .show_ui(ui, |ui| {
                        ui.selectable_value(&mut 0, 0, "Mixing");
                        ui.selectable_value(&mut 0, 1, "Mastering");
                        ui.selectable_value(&mut 0, 2, "Creative");
                    });
                    
                ui.separator();
                ui.label("Preset:");
                egui::ComboBox::from_label("")
                    .selected_text("Neutral")
                    .show_ui(ui, |ui| {
                        ui.selectable_value(&mut 0, 0, "Neutral");
                        ui.selectable_value(&mut 0, 1, "Rock/Metal");
                        ui.selectable_value(&mut 0, 2, "Pop/Hip-Hop");
                        ui.selectable_value(&mut 0, 3, "Electronic");
                        ui.selectable_value(&mut 0, 4, "Jazz/Acoustic");
                    });
            });
            
            ui.separator();
            
            // Master Section
            ui.collapsing("Master Section", |ui| {
                egui::Grid::new("master_grid").show(ui, |ui| {
                    ui.label("Input Gain:");
                    ui.add(egui::Slider::new(&mut 0.0f32, -18.0..=18.0).suffix(" dB"));
                    ui.end_row();
                    
                    ui.label("Output Gain:");
                    ui.add(egui::Slider::new(&mut 0.0f32, -18.0..=18.0).suffix(" dB"));
                    ui.end_row();
                });
            });
            
            // EQ Section
            ui.collapsing("4-Band Parametric EQ", |ui| {
                egui::Grid::new("eq_grid").show(ui, |ui| {
                    ui.label("Low Gain:");
                    ui.add(egui::Slider::new(&mut 0.0f32, -15.0..=15.0).suffix(" dB"));
                    ui.label("Freq:");
                    ui.add(egui::Slider::new(&mut 100.0f32, 20.0..=500.0).suffix(" Hz"));
                    ui.end_row();
                    
                    ui.label("Mid Gain:");
                    ui.add(egui::Slider::new(&mut 0.0f32, -15.0..=15.0).suffix(" dB"));
                    ui.label("Freq:");
                    ui.add(egui::Slider::new(&mut 1000.0f32, 200.0..=5000.0).suffix(" Hz"));
                    ui.end_row();
                    
                    ui.label("High Gain:");
                    ui.add(egui::Slider::new(&mut 0.0f32, -15.0..=15.0).suffix(" dB"));
                    ui.label("Freq:");
                    ui.add(egui::Slider::new(&mut 8000.0f32, 2000.0..=20000.0).suffix(" Hz"));
                    ui.end_row();
                    
                    ui.label("Presence:");
                    ui.add(egui::Slider::new(&mut 0.0f32, -10.0..=10.0).suffix(" dB"));
                    ui.end_row();
                });
            });
            
            // Dynamics Section
            ui.collapsing("Dynamics (Compressor + Limiter)", |ui| {
                egui::Grid::new("dynamics_grid").show(ui, |ui| {
                    ui.label("Threshold:");
                    ui.add(egui::Slider::new(&mut -12.0f32, -40.0..=0.0).suffix(" dB"));
                    ui.end_row();
                    
                    ui.label("Ratio:");
                    ui.add(egui::Slider::new(&mut 3.0f32, 1.0..=20.0));
                    ui.end_row();
                    
                    ui.label("Attack:");
                    ui.add(egui::Slider::new(&mut 10.0f32, 0.1..=100.0).suffix(" ms"));
                    ui.end_row();
                    
                    ui.label("Release:");
                    ui.add(egui::Slider::new(&mut 100.0f32, 10.0..=1000.0).suffix(" ms"));
                    ui.end_row();
                    
                    ui.separator();
                    ui.separator();
                    ui.end_row();
                    
                    ui.label("Limiter Ceiling:");
                    ui.add(egui::Slider::new(&mut -0.3f32, -3.0..=0.0).suffix(" dB"));
                    ui.end_row();
                });
            });
            
            // Stereo Enhancement
            ui.collapsing("Stereo Enhancement", |ui| {
                ui.horizontal(|ui| {
                    ui.label("Stereo Width:");
                    ui.add(egui::Slider::new(&mut 1.0f32, 0.0..=2.0));
                });
            });
            
            // Mix Controls
            ui.separator();
            ui.horizontal(|ui| {
                ui.label("Dry/Wet:");
                ui.add(egui::Slider::new(&mut 1.0f32, 0.0..=1.0));
                ui.separator();
                ui.checkbox(&mut false, "Bypass");
            });
            
            // Feature highlights
            ui.separator();
            ui.label("Professional Features:");
            ui.bullet_point("4-band parametric EQ with musical curves");
            ui.bullet_point("Multi-stage compression with vintage modeling");
            ui.bullet_point("Brick-wall limiting for broadcast standards");
            ui.bullet_point("Stereo enhancement and width control");
            ui.bullet_point("Genre-specific mastering presets");
            ui.bullet_point("Loudness metering (LUFS, dBFS, RMS)");
        });
    }
}

fn main() -> eframe::Result<()> {
    let options = NativeOptions {
        viewport: egui::ViewportBuilder::default()
            .with_inner_size([1200.0, 800.0])
            .with_title("WarriorPlug-Ins Demo - Pre-Sale UI Preview"),
        ..Default::default()
    };
    
    eframe::run_native(
        "WarriorPlug-Ins Demo",
        options,
        Box::new(|_cc| Box::new(PluginDemoApp::new())),
    )
}