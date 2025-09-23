// Titan7 Mixing & Mastering Suite (SKU: T7-MM-01) — Proprietary
// Copyright (c) 2025 Warrior Audio. All rights reserved.
// Licensed under the accompanying LICENSE.txt in this crate. Redistribution prohibited.

use nih_plug::prelude::*;
use nih_plug_egui::{create_egui_editor, EguiState, egui};
use common_dsp::{Biquad, SimpleCompressor, coeffs_peaking};
use std::sync::Arc;

const N_BANDS: usize = 4; // Multiband processor

#[derive(Debug, Clone, Copy, PartialEq, Eq, Enum)]
enum ProcessingMode {
    #[id = "mixing"]
    Mixing = 0,
    #[id = "mastering"] 
    Mastering = 1,
}

#[derive(Debug, Clone, Copy, PartialEq, Eq, Enum)]
enum GenrePreset {
    #[id = "neutral"]
    Neutral = 0,
    #[id = "pop"]
    Pop = 1,
    #[id = "rock"]
    Rock = 2,
    #[id = "electronic"]
    Electronic = 3,
    #[id = "classical"]
    Classical = 4,
}

#[derive(Params)]
struct Titan7Params {
    // Master Section
    #[id = "input_gain"]
    input_gain: FloatParam,
    #[id = "output_gain"]
    output_gain: FloatParam,
    
    // Processing Mode
    #[id = "mode"]
    mode: EnumParam<ProcessingMode>,
    #[id = "preset"]
    preset: EnumParam<GenrePreset>,
    
    // EQ Section (4-band)
    #[id = "eq_low_gain"]
    eq_low_gain: FloatParam,
    #[id = "eq_low_freq"]
    eq_low_freq: FloatParam,
    #[id = "eq_mid_gain"]
    eq_mid_gain: FloatParam,
    #[id = "eq_mid_freq"]
    eq_mid_freq: FloatParam,
    #[id = "eq_high_gain"]
    eq_high_gain: FloatParam,
    #[id = "eq_high_freq"]
    eq_high_freq: FloatParam,
    #[id = "eq_presence_gain"]
    eq_presence_gain: FloatParam,
    
    // Dynamics Section
    #[id = "comp_threshold"]
    comp_threshold: FloatParam,
    #[id = "comp_ratio"]
    comp_ratio: FloatParam,
    #[id = "comp_attack"]
    comp_attack: FloatParam,
    #[id = "comp_release"]
    comp_release: FloatParam,
    
    // Limiter Section
    #[id = "limit_ceiling"]
    limit_ceiling: FloatParam,
    #[id = "limit_release"]
    limit_release: FloatParam,
    
    // Stereo Enhancement
    #[id = "stereo_width"]
    stereo_width: FloatParam,
    
    // Mix Controls
    #[id = "dry_wet"]
    dry_wet: FloatParam,
    #[id = "bypass"]
    bypass: BoolParam,
}

impl Default for Titan7Params {
    fn default() -> Self {
        Self {
            // Master Section
            input_gain: FloatParam::new("Input Gain", 0.0, FloatRange::Linear { min: -18.0, max: 18.0 }).with_unit(" dB"),
            output_gain: FloatParam::new("Output Gain", 0.0, FloatRange::Linear { min: -18.0, max: 18.0 }).with_unit(" dB"),
            
            // Processing Mode  
            mode: EnumParam::new("Mode", ProcessingMode::Mixing),
            preset: EnumParam::new("Preset", GenrePreset::Neutral),
            
            // EQ Section
            eq_low_gain: FloatParam::new("Low Gain", 0.0, FloatRange::Linear { min: -15.0, max: 15.0 }).with_unit(" dB"),
            eq_low_freq: FloatParam::new("Low Freq", 100.0, FloatRange::Skewed { min: 20.0, max: 500.0, factor: 0.25 }).with_unit(" Hz"),
            eq_mid_gain: FloatParam::new("Mid Gain", 0.0, FloatRange::Linear { min: -15.0, max: 15.0 }).with_unit(" dB"),
            eq_mid_freq: FloatParam::new("Mid Freq", 1000.0, FloatRange::Skewed { min: 200.0, max: 5000.0, factor: 0.25 }).with_unit(" Hz"),
            eq_high_gain: FloatParam::new("High Gain", 0.0, FloatRange::Linear { min: -15.0, max: 15.0 }).with_unit(" dB"),
            eq_high_freq: FloatParam::new("High Freq", 8000.0, FloatRange::Skewed { min: 2000.0, max: 20000.0, factor: 0.25 }).with_unit(" Hz"),
            eq_presence_gain: FloatParam::new("Presence", 0.0, FloatRange::Linear { min: -10.0, max: 10.0 }).with_unit(" dB"),
            
            // Dynamics
            comp_threshold: FloatParam::new("Threshold", -12.0, FloatRange::Linear { min: -40.0, max: 0.0 }).with_unit(" dB"),
            comp_ratio: FloatParam::new("Ratio", 3.0, FloatRange::Linear { min: 1.0, max: 20.0 }),
            comp_attack: FloatParam::new("Attack", 10.0, FloatRange::Skewed { min: 0.1, max: 100.0, factor: 0.25 }).with_unit(" ms"),
            comp_release: FloatParam::new("Release", 100.0, FloatRange::Skewed { min: 10.0, max: 1000.0, factor: 0.25 }).with_unit(" ms"),
            
            // Limiter
            limit_ceiling: FloatParam::new("Ceiling", -0.3, FloatRange::Linear { min: -3.0, max: 0.0 }).with_unit(" dB"),
            limit_release: FloatParam::new("Release", 50.0, FloatRange::Skewed { min: 1.0, max: 500.0, factor: 0.25 }).with_unit(" ms"),
            
            // Stereo
            stereo_width: FloatParam::new("Stereo Width", 1.0, FloatRange::Linear { min: 0.0, max: 2.0 }),
            
            // Mix
            dry_wet: FloatParam::new("Mix", 1.0, FloatRange::Linear { min: 0.0, max: 1.0 }),
            bypass: BoolParam::new("Bypass", false),
        }
    }
}

struct Titan7Plugin {
    params: Arc<Titan7Params>,
    egui_state: Arc<EguiState>,
    fs: f32,
    
    // EQ filters (stereo)
    eq_low_l: Biquad<f32>,
    eq_low_r: Biquad<f32>,
    eq_mid_l: Biquad<f32>,
    eq_mid_r: Biquad<f32>,
    eq_high_l: Biquad<f32>,
    eq_high_r: Biquad<f32>,
    eq_presence_l: Biquad<f32>,
    eq_presence_r: Biquad<f32>,
    
    // Dynamics
    compressor_l: SimpleCompressor,
    compressor_r: SimpleCompressor,
    
    // Peak limiter (simple)
    limiter_peak_l: f32,
    limiter_peak_r: f32,
    
    // RMS metering
    input_rms_l: f32,
    input_rms_r: f32,
    output_rms_l: f32,
    output_rms_r: f32,
}

impl Default for Titan7Plugin {
    fn default() -> Self {
        Self {
            params: Arc::new(Titan7Params::default()),
            egui_state: Arc::from(EguiState::from_size(800, 600)),
            fs: 44100.0,
            
            // EQ
            eq_low_l: Biquad::new(),
            eq_low_r: Biquad::new(),
            eq_mid_l: Biquad::new(),
            eq_mid_r: Biquad::new(),
            eq_high_l: Biquad::new(),
            eq_high_r: Biquad::new(),
            eq_presence_l: Biquad::new(),
            eq_presence_r: Biquad::new(),
            
            // Dynamics
            compressor_l: SimpleCompressor::new(),
            compressor_r: SimpleCompressor::new(),
            
            // Limiter
            limiter_peak_l: 0.0,
            limiter_peak_r: 0.0,
            
            // Metering
            input_rms_l: 0.0,
            input_rms_r: 0.0,
            output_rms_l: 0.0,
            output_rms_r: 0.0,
        }
    }
}

impl Plugin for Titan7Plugin {
    const NAME: &'static str = "Titan7 Mixing & Mastering Suite";
    const VENDOR: &'static str = "Warrior Audio";
    const URL: &'static str = "https://warrior-audio.com";
    const EMAIL: &'static str = "support@warrior-audio.com";
    const VERSION: &'static str = "0.1.0";
    
    const AUDIO_IO_LAYOUTS: &'static [AudioIOLayout] = &[AudioIOLayout {
        main_input_channels: NonZeroU32::new(2),
        main_output_channels: NonZeroU32::new(2),
        ..AudioIOLayout::const_default()
    }];
    
    type SysExMessage = ();
    type BackgroundTask = ();
    
    fn params(&self) -> Arc<dyn Params> {
        self.params.clone()
    }
    
    fn initialize(&mut self, _audio_io_layout: &AudioIOLayout, buffer_config: &BufferConfig, _context: &mut impl InitContext<Self>) -> bool {
        self.fs = buffer_config.sample_rate as f32;
        true
    }
    
    fn reset(&mut self) {
        // Reset all filters and processors
        self.eq_low_l.reset();
        self.eq_low_r.reset();
        self.eq_mid_l.reset();
        self.eq_mid_r.reset();
        self.eq_high_l.reset();
        self.eq_high_r.reset();
        self.eq_presence_l.reset();
        self.eq_presence_r.reset();
        
        self.limiter_peak_l = 0.0;
        self.limiter_peak_r = 0.0;
        
        self.input_rms_l = 0.0;
        self.input_rms_r = 0.0;
        self.output_rms_l = 0.0;
        self.output_rms_r = 0.0;
    }
    
    fn process(&mut self, buffer: &mut Buffer, _aux: &mut AuxiliaryBuffers, _context: &mut impl ProcessContext<Self>) -> ProcessStatus {
        if self.params.bypass.value() {
            return ProcessStatus::Normal;
        }
        
        let fs = self.fs;
        let input_gain = 10f32.powf(self.params.input_gain.value() / 20.0);
        let output_gain = 10f32.powf(self.params.output_gain.value() / 20.0);
        let dry_wet = self.params.dry_wet.value();
        let stereo_width = self.params.stereo_width.value();
        
        // EQ parameters
        let low_gain = self.params.eq_low_gain.value();
        let low_freq = self.params.eq_low_freq.value();
        let mid_gain = self.params.eq_mid_gain.value();
        let mid_freq = self.params.eq_mid_freq.value();
        let high_gain = self.params.eq_high_gain.value();
        let high_freq = self.params.eq_high_freq.value();
        let presence_gain = self.params.eq_presence_gain.value();
        
        // Limiter
        let ceiling = 10f32.powf(self.params.limit_ceiling.value() / 20.0);
        
        // Process each sample
        for (_sample_id, channel_samples) in buffer.iter_samples().enumerate() {
            let mut samples: Vec<&mut f32> = channel_samples.into_iter().collect();
            if samples.len() >= 2 {
                let mut left = *samples[0] * input_gain;
                let mut right = *samples[1] * input_gain;
                
                let dry_left = left;
                let dry_right = right;
                
                // Update input RMS (simple)
                self.input_rms_l = 0.99 * self.input_rms_l + 0.01 * left * left;
                self.input_rms_r = 0.99 * self.input_rms_r + 0.01 * right * right;
                
                // EQ Section
                if low_gain.abs() > 0.01 {
                    let (b0, b1, b2, a1, a2) = coeffs_peaking(fs, low_freq, 0.7, low_gain);
                    self.eq_low_l.set_coefficients(b0, b1, b2, a1, a2);
                    self.eq_low_r.set_coefficients(b0, b1, b2, a1, a2);
                    left = self.eq_low_l.process(left);
                    right = self.eq_low_r.process(right);
                }
                
                if mid_gain.abs() > 0.01 {
                    let (b0, b1, b2, a1, a2) = coeffs_peaking(fs, mid_freq, 0.7, mid_gain);
                    self.eq_mid_l.set_coefficients(b0, b1, b2, a1, a2);
                    self.eq_mid_r.set_coefficients(b0, b1, b2, a1, a2);
                    left = self.eq_mid_l.process(left);
                    right = self.eq_mid_r.process(right);
                }
                
                if high_gain.abs() > 0.01 {
                    let (b0, b1, b2, a1, a2) = coeffs_peaking(fs, high_freq, 0.7, high_gain);
                    self.eq_high_l.set_coefficients(b0, b1, b2, a1, a2);
                    self.eq_high_r.set_coefficients(b0, b1, b2, a1, a2);
                    left = self.eq_high_l.process(left);
                    right = self.eq_high_r.process(right);
                }
                
                if presence_gain.abs() > 0.01 {
                    let (b0, b1, b2, a1, a2) = coeffs_peaking(fs, 3000.0, 1.5, presence_gain);
                    self.eq_presence_l.set_coefficients(b0, b1, b2, a1, a2);
                    self.eq_presence_r.set_coefficients(b0, b1, b2, a1, a2);
                    left = self.eq_presence_l.process(left);
                    right = self.eq_presence_r.process(right);
                }
                
                // Compression
                left = self.compressor_l.process(left, fs);
                right = self.compressor_r.process(right, fs);
                
                // Stereo width adjustment
                if (stereo_width - 1.0).abs() > 0.01 {
                    let mid = (left + right) * 0.5;
                    let side = (left - right) * 0.5 * stereo_width;
                    left = mid + side;
                    right = mid - side;
                }
                
                // Simple peak limiter
                left = left.clamp(-ceiling, ceiling);
                right = right.clamp(-ceiling, ceiling);
                
                // Dry/Wet mix
                left = (1.0 - dry_wet) * dry_left + dry_wet * left;
                right = (1.0 - dry_wet) * dry_right + dry_wet * right;
                
                // Output gain
                left *= output_gain;
                right *= output_gain;
                
                // Update output RMS
                self.output_rms_l = 0.99 * self.output_rms_l + 0.01 * left * left;
                self.output_rms_r = 0.99 * self.output_rms_r + 0.01 * right * right;
                
                *samples[0] = left;
                *samples[1] = right;
            }
        }
        
        ProcessStatus::Normal
    }
    
    fn editor(&mut self, _async_executor: AsyncExecutor<Self>) -> Option<Box<dyn Editor>> {
        let params = self.params.clone();
        create_egui_editor(
            self.egui_state.clone(),
            (),
            |_state, _context| {},
            move |egui_ctx, _setter, _state| {
                egui::CentralPanel::default().show(egui_ctx, |ui| {
                    ui.heading("🎛️ Titan7 Mixing & Mastering Suite");
                    
                    ui.separator();
                    
                    // Master Section
                    ui.horizontal(|ui| {
                        ui.label("Mode:");
                        ui.label(format!("{:?}", params.mode.value()));
                        ui.separator();
                        ui.label("Preset:");
                        ui.label(format!("{:?}", params.preset.value()));
                        ui.separator();
                        ui.checkbox(&mut params.bypass.value().clone(), "Bypass");
                    });
                    
                    ui.separator();
                    
                    // Gain Section
                    ui.horizontal(|ui| {
                        ui.label(format!("Input: {:.1} dB", params.input_gain.value()));
                        ui.separator();
                        ui.label(format!("Output: {:.1} dB", params.output_gain.value()));
                        ui.separator();
                        ui.label(format!("Mix: {:.0}%", params.dry_wet.value() * 100.0));
                    });
                    
                    ui.separator();
                    
                    // EQ Section
                    ui.label("🎚️ 4-Band EQ");
                    ui.horizontal(|ui| {
                        ui.vertical(|ui| {
                            ui.label("Low");
                            ui.label(format!("{:.1} dB", params.eq_low_gain.value()));
                            ui.label(format!("{:.0} Hz", params.eq_low_freq.value()));
                        });
                        ui.separator();
                        ui.vertical(|ui| {
                            ui.label("Mid");
                            ui.label(format!("{:.1} dB", params.eq_mid_gain.value()));
                            ui.label(format!("{:.0} Hz", params.eq_mid_freq.value()));
                        });
                        ui.separator();
                        ui.vertical(|ui| {
                            ui.label("High");
                            ui.label(format!("{:.1} dB", params.eq_high_gain.value()));
                            ui.label(format!("{:.0} Hz", params.eq_high_freq.value()));
                        });
                        ui.separator();
                        ui.vertical(|ui| {
                            ui.label("Presence");
                            ui.label(format!("{:.1} dB", params.eq_presence_gain.value()));
                        });
                    });
                    
                    ui.separator();
                    
                    // Dynamics Section  
                    ui.label("🔊 Dynamics");
                    ui.horizontal(|ui| {
                        ui.vertical(|ui| {
                            ui.label("Compressor");
                            ui.label(format!("Threshold: {:.1} dB", params.comp_threshold.value()));
                            ui.label(format!("Ratio: {:.1}:1", params.comp_ratio.value()));
                        });
                        ui.separator();
                        ui.vertical(|ui| {
                            ui.label("Limiter");
                            ui.label(format!("Ceiling: {:.1} dB", params.limit_ceiling.value()));
                        });
                        ui.separator();
                        ui.vertical(|ui| {
                            ui.label("Stereo");
                            ui.label(format!("Width: {:.1}x", params.stereo_width.value()));
                        });
                    });
                    
                    ui.separator();
                    ui.label("Professional mixing & mastering suite with 4-band EQ, dynamics, and stereo enhancement");
                });
            },
        )
    }
}

impl ClapPlugin for Titan7Plugin {
    const CLAP_ID: &'static str = "com.warrior.titan7_mix_master";
    const CLAP_DESCRIPTION: Option<&'static str> = Some("Comprehensive mixing and mastering suite");
    const CLAP_MANUAL_URL: Option<&'static str> = None;
    const CLAP_SUPPORT_URL: Option<&'static str> = None;
    const CLAP_FEATURES: &'static [ClapFeature] = &[
        ClapFeature::AudioEffect,
        ClapFeature::Mastering,
        ClapFeature::Mixing,
        ClapFeature::Equalizer,
        ClapFeature::Compressor,
        ClapFeature::Limiter,
    ];
}

impl Vst3Plugin for Titan7Plugin {
    const VST3_CLASS_ID: [u8; 16] = *b"TITAN7MIXMASTER1";
    const VST3_SUBCATEGORIES: &'static [Vst3SubCategory] = &[
        Vst3SubCategory::Fx,
        Vst3SubCategory::Mastering,
        Vst3SubCategory::Dynamics,
    ];
}

nih_export_clap!(Titan7Plugin);
nih_export_vst3!(Titan7Plugin);