// Warrior Vocal Repair (SKU: WV-01) — Proprietary
// Copyright (c) 2025 Warrior Audio. All rights reserved.
// Licensed under the accompanying LICENSE.txt in this crate. Redistribution prohibited.

use nih_plug::prelude::*;
use nih_plug_egui::{create_egui_editor, EguiContext, EguiState};
use common_dsp::Yin;
use atomic_float::AtomicF32;
use once_cell::sync::Lazy;
use std::sync::Mutex;

static IN_RMS: Lazy<AtomicF32> = Lazy::new(|| AtomicF32::new(0.0));
static OUT_RMS: Lazy<AtomicF32> = Lazy::new(|| AtomicF32::new(0.0));
static ESS_GR_DB: Lazy<AtomicF32> = Lazy::new(|| AtomicF32::new(0.0));
static DEN_GR_DB: Lazy<AtomicF32> = Lazy::new(|| AtomicF32::new(0.0));
static LAST_F0: Lazy<AtomicF32> = Lazy::new(|| AtomicF32::new(0.0));
static SCOPE: Lazy<Mutex<Vec<f32>>> = Lazy::new(|| Mutex::new(vec![0.0; 0]));

#[derive(Clone, Copy, Default)]
struct VocalSnapshot {
    correction: f32,
    de_ess: f32,
    denoise: f32,
    declick: f32,
    mix: f32,
    bypass: bool,
}

#[derive(Default)]
struct VocalAB { a: VocalSnapshot, b: VocalSnapshot, active_a: bool }

static AB_STATE: Lazy<Mutex<VocalAB>> = Lazy::new(|| Mutex::new(VocalAB::default()));

const PRESETS: &[(&str, VocalSnapshot)] = &[
    ("Gentle Fix", VocalSnapshot { correction: 0.3, de_ess: 0.25, denoise: 0.15, declick: 0.1, mix: 0.9, bypass: false }),
    ("Studio Clean", VocalSnapshot { correction: 0.6, de_ess: 0.5, denoise: 0.35, declick: 0.2, mix: 1.0, bypass: false }),
    ("Podcast Tight", VocalSnapshot { correction: 0.4, de_ess: 0.6, denoise: 0.45, declick: 0.3, mix: 1.0, bypass: false }),
    ("Aggressive Pop", VocalSnapshot { correction: 0.8, de_ess: 0.5, denoise: 0.25, declick: 0.2, mix: 1.0, bypass: false }),
];

#[derive(Params, Clone)]
struct VocalParams {
    #[id = "corr"]
    correction: FloatParam,
    #[id = "ess"]
    de_ess: FloatParam,
    #[id = "den"]
    denoise: FloatParam,
    #[id = "click"]
    declick: FloatParam,
    #[id = "mix"]
    mix: FloatParam,
    #[id = "bypass"]
    bypass: BoolParam,
}

impl Default for VocalParams {
    fn default() -> Self {
        Self {
            correction: FloatParam::new("Pitch Correction", 0.5, FloatRange::Linear { min: 0.0, max: 1.0 }),
            de_ess: FloatParam::new("De-Esser", 0.4, FloatRange::Linear { min: 0.0, max: 1.0 }),
            denoise: FloatParam::new("Denoise", 0.2, FloatRange::Linear { min: 0.0, max: 1.0 }),
            declick: FloatParam::new("De-Click", 0.2, FloatRange::Linear { min: 0.0, max: 1.0 }),
            mix: FloatParam::new("Mix", 1.0, FloatRange::Linear { min: 0.0, max: 1.0 }),
            bypass: BoolParam::new("Bypass", false),
        }
    }
}

struct VocalPlugin {
    params: Arc<VocalParams>,
    egui_state: EguiState,
    fs: f32,
    yin: Yin,
    frame: Vec<f32>,
    frame_pos: usize,
    last_f0: f32,
    // de-esser state
    lp_z: f32,
    ess_env: f32,
    // denoise/expander envelope
    env: f32,
    // declick state
    prev: f32,
    // pitch correction ring buffer
    pitch_buf: Vec<f32>,
    widx: usize,
    ridx: f32,
    // meters smoothing
    in_rms_s: f32,
    out_rms_s: f32,
    ess_gr_s: f32,
    den_gr_s: f32,
}

impl Default for VocalPlugin {
    fn default() -> Self {
        let frame_len = 1024;
        Self {
            params: Arc::new(VocalParams::default()),
            egui_state: EguiState::from_size(720, 420),
            fs: 44100.0,
            yin: Yin::new(frame_len, 1024, 0.1),
            frame: vec![0.0; frame_len],
            frame_pos: 0,
            last_f0: 0.0,
            lp_z: 0.0,
            ess_env: 0.0,
            env: 0.0,
            prev: 0.0,
            pitch_buf: vec![0.0; 8192],
            widx: 0,
            ridx: 0.0,
            in_rms_s: 0.0,
            out_rms_s: 0.0,
            ess_gr_s: 0.0,
            den_gr_s: 0.0,
        }
    }
}

impl Plugin for VocalPlugin {
    const NAME: &'static str = "Warrior Vocal";
    const VENDOR: &'static str = "Warrior";
    const URL: &'static str = "https://example.com";
    const EMAIL: &'static str = "support@example.com";

    const VERSION: &'static str = "0.1.0";
    const AUDIO_IO_LAYOUTS: &'static [AudioIOLayout] = &[AudioIOLayout { main_input_channels: NonZeroU32::new(1), main_output_channels: NonZeroU32::new(1), ..AudioIOLayout::const_default() } ];

    type Params = VocalParams;

    fn params(&self) -> Arc<Self::Params> { self.params.clone() }

    fn initialize(&mut self, _io: &AudioIOLayout, buffer_config: &BufferConfig, _ctx: &mut impl InitContext<Self>) -> bool {
        self.fs = buffer_config.sample_rate as f32;
        true
    }

    fn process(&mut self, buffer: &mut Buffer, _aux: &mut AuxiliaryBuffers, _ctx: &mut impl ProcessContext<Self>) -> ProcessStatus {
        let mix = self.params.mix.value();
        let corr_amt = self.params.correction.value();
        let de_ess = self.params.de_ess.value();
        let denoise = self.params.denoise.value();
        let declick = self.params.declick.value();
        let bypass = self.params.bypass.value();

        let chan = buffer.as_mut_slice().get_mut(0).unwrap();
        let mut in_acc = 0.0f32;
        let mut out_acc = 0.0f32;
        for i in 0..buffer.samples() {
            let x = chan[i];
            in_acc += x * x;
            // accumulate frame for pitch detection
            self.frame[self.frame_pos] = x;
            self.frame_pos += 1;
            if self.frame_pos >= self.frame.len() {
                if let Some(f0) = self.yin.detect(&self.frame, self.fs) { self.last_f0 = f0; LAST_F0.store(f0, std::sync::atomic::Ordering::Relaxed); }
                self.frame_pos = 0;
            }
            if bypass {
                chan[i] = x;
                // scope (decimate)
                if i % 64 == 0 {
                    let mut sc = SCOPE.lock().unwrap();
                    sc.push(x);
                    if sc.len() > 1024 { let _ = sc.drain(0..(sc.len()-1024)); }
                }
                continue;
            }
            // de-esser: one-pole lowpass to get low content, subtract for high band
            let a = 0.98; // cutoff ~3-5k depending on fs
            let lp = a * self.lp_z + (1.0 - a) * x;
            self.lp_z = lp;
            let high = x - lp;
            // detect sibilant energy with fast attack/slow release
            let attack = 0.2f32;
            let release = 0.02f32;
            self.ess_env = if high.abs() > self.ess_env {
                attack * self.ess_env + (1.0 - attack) * high.abs()
            } else {
                release * self.ess_env + (1.0 - release) * high.abs()
            };
            let ess_gain = 1.0 / (1.0 + de_ess * 10.0 * self.ess_env);
            let ess_gr_db_inst = 20.0 * ess_gain.max(1e-6).log10();
            let deessed = lp + high * ess_gain;

            // denoise: simple downward expander based on envelope
            let a_env = 0.01f32; // fast envelope follower
            self.env = a_env * self.env + (1.0 - a_env) * deessed.abs();
            let noise_floor = 0.01 * (1.0 - denoise); // dynamic threshold
            let exp_ratio = 2.0 + 4.0 * denoise; // 2:1..6:1
            let gain = if self.env < noise_floor {
                (self.env / noise_floor).powf(exp_ratio - 1.0)
            } else { 1.0 };
            let den_gr_db_inst = 20.0 * gain.max(1e-6).log10();
            let den = deessed * gain;

            // de-click: slew-rate limiter
            let max_delta = 0.5 * (1.0 - declick) + 0.02; // smaller delta at higher declick
            let delta = (den - self.prev).clamp(-max_delta, max_delta);
            let dc = self.prev + delta;
            self.prev = dc;

            // pitch correction to nearest semitone using resampling ring buffer
            let mut y = dc;
            if self.last_f0 > 0.0 && corr_amt > 0.0 {
                let semis = 12.0 * (self.last_f0 / 440.0).log2();
                let target = semis.round().clamp(-48.0, 48.0);
                let f_tgt = 440.0 * 2f32.powf(target / 12.0);
                let ratio = (f_tgt / self.last_f0).powf(corr_amt).clamp(0.5, 2.0);
                // write input
                self.pitch_buf[self.widx] = dc;
                self.widx = (self.widx + 1) & (self.pitch_buf.len() - 1);
                // read at variable rate
                let ridx_int = self.ridx.floor() as usize & (self.pitch_buf.len() - 1);
                let ridx_next = (ridx_int + 1) & (self.pitch_buf.len() - 1);
                let frac = self.ridx - self.ridx.floor();
                let s0 = self.pitch_buf[ridx_int];
                let s1 = self.pitch_buf[ridx_next];
                y = s0 + (s1 - s0) * frac as f32;
                self.ridx += ratio;
                // prevent reader from overtaking writer too far
                let dist = ((self.widx as isize - self.ridx as isize) & ((self.pitch_buf.len() as isize) - 1)) as usize;
                if dist < 64 { self.ridx = (self.widx as f32 + 256.0) % (self.pitch_buf.len() as f32); }
            }

            chan[i] = (1.0 - mix) * x + mix * y;
            out_acc += chan[i] * chan[i];
            // meters smoothing
            let smooth = 0.8f32;
            self.ess_gr_s = smooth * self.ess_gr_s + (1.0 - smooth) * ess_gr_db_inst;
            self.den_gr_s = smooth * self.den_gr_s + (1.0 - smooth) * den_gr_db_inst;
            ESS_GR_DB.store(self.ess_gr_s, std::sync::atomic::Ordering::Relaxed);
            DEN_GR_DB.store(self.den_gr_s, std::sync::atomic::Ordering::Relaxed);
            // scope (decimate)
            if i % 64 == 0 {
                let mut sc = SCOPE.lock().unwrap();
                sc.push(chan[i]);
                if sc.len() > 1024 { let _ = sc.drain(0..(sc.len()-1024)); }
            }
        }
        if buffer.samples() > 0 {
            let in_rms = (in_acc / buffer.samples() as f32).sqrt();
            let out_rms = (out_acc / buffer.samples() as f32).sqrt();
            let smooth = 0.9f32;
            self.in_rms_s = smooth * self.in_rms_s + (1.0 - smooth) * in_rms;
            self.out_rms_s = smooth * self.out_rms_s + (1.0 - smooth) * out_rms;
            IN_RMS.store(self.in_rms_s, std::sync::atomic::Ordering::Relaxed);
            OUT_RMS.store(self.out_rms_s, std::sync::atomic::Ordering::Relaxed);
        }
        ProcessStatus::Normal
    }

    fn editor(&mut self, _context: &mut impl GuiContext<Self>) -> Option<Box<dyn Editor>> {
        let params = self.params.clone();
        create_egui_editor(self.egui_state.clone(), move |egui_ctx: &egui::Context, _| {
            egui_ctx.set_visuals(egui::Visuals::dark());
            egui::CentralPanel::default().show(egui_ctx, |ui| {
                ui.heading("Warrior Vocal Repair");
                ui.horizontal(|ui| {
                    ui.toggle_value(&mut params.bypass.value_mut(), "Bypass");
                    ui.label(" | ");
                    ui.add(egui::Slider::new(&mut params.mix.value_mut(), 0.0..=1.0).text("Mix"));
                });
                ui.horizontal(|ui| {
                    ui.label("A/B:");
                    let mut ab = AB_STATE.lock().unwrap();
                    if ui.selectable_label(ab.active_a, "A").clicked() { ab.active_a = true; }
                    if ui.selectable_label(!ab.active_a, "B").clicked() { ab.active_a = false; }
                    if ui.button("Store").clicked() {
                        let snap = VocalSnapshot { correction: params.correction.value(), de_ess: params.de_ess.value(), denoise: params.denoise.value(), declick: params.declick.value(), mix: params.mix.value(), bypass: params.bypass.value() };
                        if ab.active_a { ab.a = snap; } else { ab.b = snap; }
                    }
                    if ui.button("Recall").clicked() {
                        let snap = if ab.active_a { ab.a } else { ab.b };
                        apply_snapshot(&params, snap);
                    }
                    if ui.button("Swap A↔B").clicked() { std::mem::swap(&mut ab.a, &mut ab.b); }
                    if ui.button("Copy to other").clicked() {
                        if ab.active_a { ab.b = ab.a; } else { ab.a = ab.b; }
                    }
                });
                ui.add(egui::Slider::new(&mut params.correction.value_mut(), 0.0..=1.0).text("Pitch Correction"));
                ui.add(egui::Slider::new(&mut params.de_ess.value_mut(), 0.0..=1.0).text("De-Esser"));
                ui.add(egui::Slider::new(&mut params.denoise.value_mut(), 0.0..=1.0).text("Denoise"));
                ui.add(egui::Slider::new(&mut params.declick.value_mut(), 0.0..=1.0).text("De-Click"));
                ui.separator();
                ui.horizontal(|ui| {
                    ui.label("Preset:");
                    let mut selected: usize = 0;
                    egui::ComboBox::from_label("")
                        .selected_text(PRESETS[selected].0)
                        .show_ui(ui, |ui| {
                            for (i, (name, _)) in PRESETS.iter().enumerate() {
                                if ui.selectable_label(i == selected, *name).clicked() {
                                    selected = i;
                                }
                            }
                        });
                    if ui.button("Load").clicked() {
                        let snap = PRESETS[selected].1;
                        apply_snapshot(&params, snap);
                    }
                });
                ui.horizontal(|ui| {
                    // meters
                    let in_rms = IN_RMS.load(std::sync::atomic::Ordering::Relaxed);
                    let out_rms = OUT_RMS.load(std::sync::atomic::Ordering::Relaxed);
                    let ess_gr = ESS_GR_DB.load(std::sync::atomic::Ordering::Relaxed);
                    let den_gr = DEN_GR_DB.load(std::sync::atomic::Ordering::Relaxed);
                    meter(ui, "In", in_rms);
                    meter(ui, "Out", out_rms);
                    gr_meter(ui, "De-Ess dB", ess_gr);
                    gr_meter(ui, "Denoise dB", den_gr);
                });
                ui.separator();
                // scope
                use egui::plot::{Line, Plot, PlotPoints};
                let scope_pts: Vec<[f64;2]> = {
                    let sc = SCOPE.lock().unwrap();
                    sc.iter().enumerate().map(|(i,&v)| [i as f64, v as f64]).collect()
                };
                let line = Line::new(PlotPoints::from_iter(scope_pts));
                Plot::new("scope").height(160.0).view_aspect(4.0).show(ui, |plot_ui| { plot_ui.line(line); });
                ui.separator();
                // pitch readout
                let f0 = LAST_F0.load(std::sync::atomic::Ordering::Relaxed);
                let (note, cents) = if f0 > 0.0 { freq_to_note_cents(f0) } else { ("--".to_string(), 0.0) };
                ui.label(format!("Pitch: {:.1} Hz  [{}  {:+.1} cents]", f0, note, cents));
            });
        })
    }
}

fn meter(ui: &mut egui::Ui, label: &str, rms: f32) {
    let db = 20.0 * rms.max(1e-6).log10();
    let norm = ((db + 60.0)/60.0).clamp(0.0, 1.0);
    ui.vertical(|ui| {
        ui.label(label);
        let (r, _) = ui.allocate_exact_size(egui::vec2(12.0, 80.0), egui::Sense::hover());
        let bg = ui.painter().rect_stroke(r, 2.0, egui::Stroke::new(1.0, egui::Color32::GRAY));
        let h = 80.0 * norm;
        let fill = egui::Rect::from_min_max(
            egui::pos2(r.left(), r.bottom() - h),
            egui::pos2(r.right(), r.bottom()),
        );
        ui.painter().rect_filled(fill, 2.0, egui::Color32::from_rgb(80, 200, 120));
    });
}

fn gr_meter(ui: &mut egui::Ui, label: &str, gr_db: f32) {
    let norm = ((-gr_db)/24.0).clamp(0.0, 1.0); // show reduction amount up to 24 dB
    ui.vertical(|ui| {
        ui.label(label);
        let (r, _) = ui.allocate_exact_size(egui::vec2(12.0, 80.0), egui::Sense::hover());
        let h = 80.0 * norm;
        let fill = egui::Rect::from_min_max(
            egui::pos2(r.left(), r.bottom() - h),
            egui::pos2(r.right(), r.bottom()),
        );
        ui.painter().rect_filled(fill, 2.0, egui::Color32::from_rgb(220, 120, 80));
    });
}

fn freq_to_note_cents(f: f32) -> (String, f32) {
    let a4 = 440.0f32;
    let semis = 12.0 * (f / a4).log2();
    let si = semis.round();
    let cents = (semis - si) * 100.0;
    let note_idx = ((si as i32 + 9).rem_euclid(12)) as usize; // A as 9 -> map to C index 0
    let octave = 4 + ((si as i32 + 9) / 12);
    let names = ["C","C#","D","D#","E","F","F#","G","G#","A","A#","B"];
    (format!("{}{}", names[note_idx], octave), cents)
}

fn apply_snapshot(params: &Arc<VocalParams>, s: VocalSnapshot) {
    *params.correction.value_mut() = s.correction;
    *params.de_ess.value_mut() = s.de_ess;
    *params.denoise.value_mut() = s.denoise;
    *params.declick.value_mut() = s.declick;
    *params.mix.value_mut() = s.mix;
    *params.bypass.value_mut() = s.bypass;
}

impl ClapPlugin for VocalPlugin {
    const CLAP_ID: &'static str = "com.warrior.vocal";
    const CLAP_DESCRIPTION: Option<&'static str> = Some("Vocal repair: pitch, de-ess, denoise, de-click");
    const CLAP_FEATURES: &'static [ClapFeature] = &[ClapFeature::AudioEffect, ClapFeature::Mono, ClapFeature::Restoration];
}

impl Vst3Plugin for VocalPlugin {
    const VST3_CLASS_ID: [u8; 16] = *b"WARRVOCAPLUG001";
    const VST3_SUBCATEGORIES: &'static [Vst3SubCategory] = &[Vst3SubCategory::Fx, Vst3SubCategory::Restoration];
}

nih_export_clap!(VocalPlugin);
nih_export_vst3!(VocalPlugin);
