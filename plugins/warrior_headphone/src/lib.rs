// Warrior Headphone (SKU: WH-01) — Proprietary
// Copyright (c) 2025 Warrior Audio. All rights reserved.
// Licensed under the accompanying LICENSE.txt in this crate. Redistribution prohibited.

use nih_plug::prelude::*;
use nih_plug_egui::{create_egui_editor, EguiContext, EguiState};
use common_dsp::{stock_curves, Biquad, CalibrationCurve, Crossfeed, SimpleCompressor, coeffs_peaking};
const N_BANDS: usize = 8;

#[derive(Params, Clone)]
struct HeadphoneParams {
    #[id = "gain"]
    gain: FloatParam,
    #[id = "blend"]
    blend: FloatParam,
    #[id = "cross"]
    cross: FloatParam,
    #[id = "hp_model"]
    hp_model: EnumParam<usize>,
    #[id = "codec"]
    codec: EnumParam<i32>,
    #[id = "ull"]
    ultra_low_latency: BoolParam,
}

impl Default for HeadphoneParams {
    fn default() -> Self {
        let models = stock_curves();
        Self {
            gain: FloatParam::new("Output Gain", 0.0, FloatRange::Linear { min: -24.0, max: 24.0 }).with_unit(" dB"),
            blend: FloatParam::new("Blend", 1.0, FloatRange::Linear { min: 0.0, max: 1.0 }),
            cross: FloatParam::new("Crossfeed", 0.15, FloatRange::Linear { min: 0.0, max: 1.0 }),
            hp_model: EnumParam::new("Model", 0, (0..models.len()).collect())
                .with_value_to_string(Arc::new(move |v| models[v].name.clone())),
            codec: EnumParam::new("Codec", 0, vec![0,1,2,3])
                .with_value_to_string(Arc::new(|v| match v { 0 => "Off".to_string(), 1 => "SBC/AAC".to_string(), 2 => "aptX".to_string(), 3 => "LDAC".to_string(), _ => "Off".to_string() })),
            ultra_low_latency: BoolParam::new("Ultra Low Latency", false),
        }
    }
}

struct HeadphonePlugin {
    params: Arc<HeadphoneParams>,
    egui_state: EguiState,
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
            egui_state: EguiState::from_size(600, 360),
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

    type Params = HeadphoneParams;

    fn params(&self) -> Arc<Self::Params> { self.params.clone() }

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
            1 => (1.0, -1.5),  // SBC/AAC
            2 => (0.5, -0.5),  // aptX
            3 => (0.3, 0.0),   // LDAC
            _ => (0.0, 0.0),
        };
        let model = stock_curves();
        let curve = &model[current_model_idx];

        // process per channel explicitly
        let stereo = buffer.channels() >= 2;
        let ull = self.params.ultra_low_latency.value();
        if stereo {
            let (left, right) = buffer.as_buffers_mut().split_at_mut(1);
            let left = &mut left[0];
            let right = &mut right[0];
            for i in 0..buffer.samples() {
                let mut xl = left[i];
                let mut xr = right[i];
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
                xl = (1.0 - blend) * left[i] + blend * xl;
                xr = (1.0 - blend) * right[i] + blend * xr;
                left[i] = xl;
                right[i] = xr;
                // crossfeed last
                if !ull {
                    let (nl, nr) = self.crossfeed.process(left[i], right[i]);
                    left[i] = nl;
                    right[i] = nr;
                }
                left[i] *= 10f32.powf(gain/20.0);
                right[i] *= 10f32.powf(gain/20.0);
            }
        } else {
            let chan = buffer.as_mut_slice().get_mut(0).unwrap();
            for i in 0..buffer.samples() {
                let mut x = chan[i];
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
                x = (1.0 - blend) * chan[i] + blend * x;
                chan[i] = x * 10f32.powf(gain/20.0);
            }
        }
        ProcessStatus::Normal
    }

    fn editor(&mut self, _context: &mut impl GuiContext<Self>) -> Option<Box<dyn Editor>> {
        let params = self.params.clone();
        create_egui_editor(
            self.egui_state.clone(),
            move |egui_ctx: &egui::Context, _| {
                egui::CentralPanel::default().show(egui_ctx, |ui| {
                    ui.heading("Warrior Headphone");
                    ui.add(egui::Slider::new(&mut params.gain.value_mut(), -24.0..=24.0).text("Gain dB"));
                    ui.add(egui::Slider::new(&mut params.blend.value_mut(), 0.0..=1.0).text("Blend"));
                    ui.add(egui::Slider::new(&mut params.cross.value_mut(), 0.0..=1.0).text("Crossfeed"));
                    ui.checkbox(&mut params.ultra_low_latency.value_mut(), "Ultra Low Latency");
                    ui.horizontal(|ui| {
                        ui.label("Codec:");
                        egui::ComboBox::from_label("")
                            .selected_text(params.codec.to_string())
                            .show_ui(ui, |ui| {
                                for (i, label) in ["Off","SBC/AAC","aptX","LDAC"].iter().enumerate() {
                                    ui.selectable_value(&mut params.codec.value_mut(), i as i32, *label);
                                }
                            });
                    });
                    ui.separator();
                    ui.horizontal(|ui| {
                        ui.label("Model:");
                        egui::ComboBox::from_label("")
                            .selected_text(params.hp_model.to_string())
                            .show_ui(ui, |ui| {
                                for (i, c) in stock_curves().iter().enumerate() {
                                    ui.selectable_value(&mut params.hp_model.value_mut(), i as i32, &c.name);
                                }
                            });
                    });
                    ui.separator();
                    // simple frequency response preview (log spaced 20..20k)
                    let response = {
                        let curve = &stock_curves()[params.hp_model.value() as usize];
                        let mut pts: Vec<[f32; 2]> = Vec::with_capacity(241);
                        for i in 0..241 {
                            let t = i as f32 / 240.0;
                            let f = 20.0 * 10f32.powf(t * 3.0); // 20..20k
                            let g = curve.gain_at(f);
                            pts.push([i as f32, g]);
                        }
                        pts
                    };
                    use egui::plot::{Line, Plot, PlotPoints};
                    let line = Line::new(PlotPoints::from_iter(response.into_iter().map(|p| [p[0] as f64, p[1] as f64])));
                    Plot::new("resp").view_aspect(2.5).height(180.0).show(ui, |plot_ui| {
                        plot_ui.line(line);
                    });
                });
            },
        )
    }
}

impl ClapPlugin for HeadphonePlugin {
    const CLAP_ID: &'static str = "com.warrior.headphone";
    const CLAP_DESCRIPTION: Option<&'static str> = Some("Headphone calibration and enhancement");
    const CLAP_FEATURES: &'static [ClapFeature] = &[ClapFeature::AudioEffect, ClapFeature::Mastering, ClapFeature::Stereo];
}

impl Vst3Plugin for HeadphonePlugin {
    const VST3_CLASS_ID: [u8; 16] = *b"WARRHPHNPLUG001";
    const VST3_SUBCATEGORIES: &'static [Vst3SubCategory] = &[Vst3SubCategory::Fx, Vst3SubCategory::Mastering];
}

nih_export_clap!(HeadphonePlugin);
nih_export_vst3!(HeadphonePlugin);
