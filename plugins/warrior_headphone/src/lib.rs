// Warrior Headphone (SKU: WH-01) — Proprietary
// Copyright (c) 2025 Warrior Audio. All rights reserved.
// Licensed under the accompanying LICENSE.txt in this crate. Redistribution prohibited.

use nih_plug::prelude::*;
use nih_plug_egui::{create_egui_editor, EguiState, egui};
use common_dsp::{stock_curves, Biquad, Crossfeed, SimpleCompressor, coeffs_peaking};
use std::sync::Arc;

const N_BANDS: usize = 8;

#[derive(Debug, Clone, Copy, PartialEq, Eq, Enum)]
enum HeadphoneModel {
    #[id = "model0"]
    Model0 = 0,
    #[id = "model1"] 
    Model1 = 1,
    #[id = "model2"]
    Model2 = 2,
    #[id = "model3"]
    Model3 = 3,
    #[id = "model4"]
    Model4 = 4,
    #[id = "model5"]
    Model5 = 5,
    #[id = "model6"]
    Model6 = 6,
    #[id = "model7"]
    Model7 = 7,
}

#[derive(Debug, Clone, Copy, PartialEq, Eq, Enum)]
enum CodecType {
    #[id = "off"]
    Off = 0,
    #[id = "sbc_aac"]
    SbcAac = 1,
    #[id = "aptx"]
    AptX = 2,
    #[id = "ldac"]
    Ldac = 3,
}

#[derive(Params)]
struct HeadphoneParams {
    #[id = "gain"]
    gain: FloatParam,
    #[id = "blend"]
    blend: FloatParam,
    #[id = "cross"]
    cross: FloatParam,
    #[id = "hp_model"]
    hp_model: EnumParam<HeadphoneModel>,
    #[id = "codec"]
    codec: EnumParam<CodecType>,
    #[id = "ull"]
    ultra_low_latency: BoolParam,
}

impl Default for HeadphoneParams {
    fn default() -> Self {
        Self {
            gain: FloatParam::new("Output Gain", 0.0, FloatRange::Linear { min: -24.0, max: 24.0 }).with_unit(" dB"),
            blend: FloatParam::new("Blend", 1.0, FloatRange::Linear { min: 0.0, max: 1.0 }),
            cross: FloatParam::new("Crossfeed", 0.15, FloatRange::Linear { min: 0.0, max: 1.0 }),
            hp_model: EnumParam::new("Model", HeadphoneModel::Model0),
            codec: EnumParam::new("Codec", CodecType::Off),
            ultra_low_latency: BoolParam::new("Ultra Low Latency", false),
        }
    }
}

struct HeadphonePlugin {
    params: Arc<HeadphoneParams>,
    egui_state: Arc<EguiState>,
    fs: f32,
    eq_l: [Biquad<f32>; N_BANDS],
    eq_r: [Biquad<f32>; N_BANDS],
    crossfeed: Crossfeed,
    comp: SimpleCompressor,
    ab_enabled: bool,
    last_model_idx: usize,
}

impl Default for HeadphonePlugin {
    fn default() -> Self {
        Self {
            params: Arc::new(HeadphoneParams::default()),
            egui_state: Arc::from(EguiState::from_size(600, 360)),
            fs: 44100.0,
            eq_l: [(); N_BANDS].map(|_| Biquad::new()),
            eq_r: [(); N_BANDS].map(|_| Biquad::new()),
            crossfeed: Crossfeed::new(),
            comp: SimpleCompressor::new(),
            ab_enabled: false,
            last_model_idx: 0,
        }
    }
}

impl Plugin for HeadphonePlugin {
    const NAME: &'static str = "Warrior Headphone";
    const VENDOR: &'static str = "Warrior";
    const URL: &'static str = "https://example.com";
    const EMAIL: &'static str = "support@example.com";

    const VERSION: &'static str = "0.1.0";
    const AUDIO_IO_LAYOUTS: &'static [AudioIOLayout] = &[AudioIOLayout { main_input_channels: NonZeroU32::new(2), main_output_channels: NonZeroU32::new(2), ..AudioIOLayout::const_default() } ];

    type SysExMessage = ();
    type BackgroundTask = ();

    fn params(&self) -> Arc<dyn Params> { self.params.clone() }

    fn initialize(&mut self, _audio_io_layout: &AudioIOLayout, buffer_config: &BufferConfig, _context: &mut impl InitContext<Self>) -> bool {
        self.fs = buffer_config.sample_rate as f32;
        self.crossfeed.set_delay_ms(0.25, self.fs);
        true
    }

    fn reset(&mut self) {
        for b in &mut self.eq_l { b.reset(); }
        for b in &mut self.eq_r { b.reset(); }
    }

    fn process(&mut self, buffer: &mut Buffer, _aux: &mut AuxiliaryBuffers, _context: &mut impl ProcessContext<Self>) -> ProcessStatus {
        let fs = self.fs;
        let blend = self.params.blend.value();
        let gain = self.params.gain.value();
        let cross = self.params.cross.value();
        self.crossfeed.set_amount(cross);

        let current_model_idx = self.params.hp_model.value() as usize;
        if current_model_idx != self.last_model_idx {
            // reset filters when model changes
            for b in &mut self.eq_l { b.reset(); }
            for b in &mut self.eq_r { b.reset(); }
            self.last_model_idx = current_model_idx;
        }

        // compute log-spaced center frequencies for bands
        let mut centers = [0.0f32; N_BANDS];
        for i in 0..N_BANDS {
            let t = i as f32 / (N_BANDS as f32 - 1.0);
            centers[i] = 20.0 * 10f32.powf(t * (std::f32::consts::LOG10_2 * 14.0)); // ~20..~20k
        }
        // prepare codec compensation
        let codec_mode = self.params.codec.value();
        let (codec_tilt_db, codec_hf_dip_db) = match codec_mode {
            CodecType::SbcAac => (1.0, -1.5),  // SBC/AAC
            CodecType::AptX => (0.5, -0.5),    // aptX
            CodecType::Ldac => (0.3, 0.0),     // LDAC
            CodecType::Off => (0.0, 0.0),
        };
        let model = stock_curves();
        let curve = &model[current_model_idx];

        // process per channel explicitly
        let stereo = buffer.channels() >= 2;
        let ull = self.params.ultra_low_latency.value();
        if stereo {
            for (_sample_id, channel_samples) in buffer.iter_samples().enumerate() {
                let mut samples: Vec<&mut f32> = channel_samples.into_iter().collect();
                if samples.len() >= 2 {
                    let mut xl = *samples[0];
                    let mut xr = *samples[1];
                    
                    // EQ
                    for (bi, f0) in centers.iter().enumerate() {
                        let mut g = curve.gain_at(*f0);
                        // codec compensation: a gentle tilt and HF dip
                        let tilt = codec_tilt_db * ((*f0 / 1000.0).log10()).clamp(-1.0, 1.0);
                        let hf = if *f0 > 8000.0 { codec_hf_dip_db } else { 0.0 };
                        g += tilt + hf;
                        let (b0,b1,b2,a1,a2) = coeffs_peaking(fs, *f0, 0.9, g);
                        self.eq_l[bi].set_coefficients(b0,b1,b2,a1,a2);
                        self.eq_r[bi].set_coefficients(b0,b1,b2,a1,a2);
                        xl = self.eq_l[bi].process(xl);
                        xr = self.eq_r[bi].process(xr);
                    }
                    if !ull {
                        xl = self.comp.process(xl, fs);
                        xr = self.comp.process(xr, fs);
                    }
                    let orig_l = *samples[0];
                    let orig_r = *samples[1];
                    xl = (1.0 - blend) * orig_l + blend * xl;
                    xr = (1.0 - blend) * orig_r + blend * xr;
                    *samples[0] = xl;
                    *samples[1] = xr;
                    // crossfeed last
                    if !ull {
                        let (nl, nr) = self.crossfeed.process(*samples[0], *samples[1]);
                        *samples[0] = nl;
                        *samples[1] = nr;
                    }
                    *samples[0] *= 10f32.powf(gain/20.0);
                    *samples[1] *= 10f32.powf(gain/20.0);
                }
            }
        } else {
            for (_sample_id, channel_samples) in buffer.iter_samples().enumerate() {
                let mut samples: Vec<&mut f32> = channel_samples.into_iter().collect();
                if !samples.is_empty() {
                    let mut x = *samples[0];
                    for (bi, f0) in centers.iter().enumerate() {
                        let mut g = curve.gain_at(*f0);
                        let tilt = codec_tilt_db * ((*f0 / 1000.0).log10()).clamp(-1.0, 1.0);
                        let hf = if *f0 > 8000.0 { codec_hf_dip_db } else { 0.0 };
                        g += tilt + hf;
                        let (b0,b1,b2,a1,a2) = coeffs_peaking(fs, *f0, 0.9, g);
                        self.eq_l[bi].set_coefficients(b0,b1,b2,a1,a2);
                        x = self.eq_l[bi].process(x);
                    }
                    if !ull { x = self.comp.process(x, fs); }
                    let orig = *samples[0];
                    x = (1.0 - blend) * orig + blend * x;
                    *samples[0] = x * 10f32.powf(gain/20.0);
                }
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
                    ui.heading("Warrior Headphone");
                    
                    // Simple parameter display for now - we'll fix this in the next task
                    ui.label(format!("Gain: {:.1} dB", params.gain.value()));
                    ui.label(format!("Blend: {:.2}", params.blend.value()));
                    ui.label(format!("Crossfeed: {:.2}", params.cross.value()));
                    ui.label(format!("Ultra Low Latency: {}", params.ultra_low_latency.value()));
                    ui.label(format!("Model: {:?}", params.hp_model.value()));
                    ui.label(format!("Codec: {:?}", params.codec.value()));
                });
            },
        )
    }
}

impl ClapPlugin for HeadphonePlugin {
    const CLAP_ID: &'static str = "com.warrior.headphone";
    const CLAP_DESCRIPTION: Option<&'static str> = Some("Headphone calibration and enhancement");
    const CLAP_MANUAL_URL: Option<&'static str> = None;
    const CLAP_SUPPORT_URL: Option<&'static str> = None;
    const CLAP_FEATURES: &'static [ClapFeature] = &[ClapFeature::AudioEffect, ClapFeature::Mastering, ClapFeature::Stereo];
}

impl Vst3Plugin for HeadphonePlugin {
    const VST3_CLASS_ID: [u8; 16] = *b"WARRHPHNPLUG0001";
    const VST3_SUBCATEGORIES: &'static [Vst3SubCategory] = &[Vst3SubCategory::Fx, Vst3SubCategory::Mastering];
}

nih_export_clap!(HeadphonePlugin);
nih_export_vst3!(HeadphonePlugin);
