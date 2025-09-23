// Warrior USB Instrument (SKU: WU-01) — Proprietary
// Copyright (c) 2025 Warrior Audio. All rights reserved.
// Licensed under the accompanying LICENSE.txt in this crate. Redistribution prohibited.

use nih_plug::prelude::*;
use nih_plug_egui::{create_egui_editor, EguiState, egui};
use common_dsp::{Biquad, coeffs_lowshelf, coeffs_peaking, SimpleCompressor};
use realfft::num_complex::Complex32;
use common_dsp::{FftCache, hann};
use atomic_float::AtomicF32;
use once_cell::sync::Lazy;
use std::sync::Arc;

static IN_RMS: Lazy<AtomicF32> = Lazy::new(|| AtomicF32::new(0.0));
static OUT_RMS: Lazy<AtomicF32> = Lazy::new(|| AtomicF32::new(0.0));

#[derive(Debug, Copy, Clone, PartialEq, Eq, Enum)]
enum Instrument { 
    #[id = "auto"]
    Auto,
    #[id = "guitar"] 
    Guitar,
    #[id = "bass"]
    Bass,
    #[id = "vocal"]
    VocalMic,
    #[id = "keys"]
    Keys 
}

#[derive(Params)]
struct UsbParams {
    #[id = "inst"]
    inst: EnumParam<Instrument>,
    #[id = "gain"]
    out_gain: FloatParam,
    #[id = "ull"]
    ultra_low_latency: BoolParam,
    #[id = "drive"]
    drive: FloatParam,
    #[id = "verb"]
    reverb: FloatParam,
    #[id = "comp"]
    comp: FloatParam,
    #[id = "tone"]
    tone: FloatParam,
}

impl Default for UsbParams {
    fn default() -> Self {
        Self {
            inst: EnumParam::new("Instrument", Instrument::Auto),
            out_gain: FloatParam::new("Output Gain", 0.0, FloatRange::Linear { min: -24.0, max: 24.0 }).with_unit(" dB"),
            ultra_low_latency: BoolParam::new("Ultra Low Latency", false),
            drive: FloatParam::new("Drive", 0.4, FloatRange::Linear { min: 0.0, max: 1.0 }),
            reverb: FloatParam::new("Reverb", 0.2, FloatRange::Linear { min: 0.0, max: 1.0 }),
            comp: FloatParam::new("Compression", 0.4, FloatRange::Linear { min: 0.0, max: 1.0 }),
            tone: FloatParam::new("Tone", 0.5, FloatRange::Linear { min: 0.0, max: 1.0 }),
        }
    }
}

struct UsbPlugin {
    params: Arc<UsbParams>,
    egui_state: EguiState,
    fs: f32,
    // analysis
    fft: FftCache,
    window: Vec<f32>,
    ana_buf: Vec<f32>,
    ana_pos: usize,
    centroid: f32,
    // chain state
    comp: SimpleCompressor,
    eq1: Biquad<f32>,
    eq2: Biquad<f32>,
    cab_lpf: Biquad<f32>,
    rev: SimpleReverb,
}

impl Default for UsbPlugin {
    fn default() -> Self {
        let n = 1024;
        Self {
            params: Arc::new(UsbParams::default()),
            egui_state: Arc::new(EguiState::from_size(760, 460)),
            fs: 44100.0,
            fft: FftCache::new(n),
            window: hann(n),
            ana_buf: vec![0.0; n],
            ana_pos: 0,
            centroid: 0.0,
            comp: SimpleCompressor::new(),
            eq1: Biquad::new(),
            eq2: Biquad::new(),
            cab_lpf: Biquad::new(),
            rev: SimpleReverb::new(),
        }
    }
}

impl Plugin for UsbPlugin {
    const NAME: &'static str = "Warrior USB Instrument";
    const VENDOR: &'static str = "Warrior";
    const URL: &'static str = "https://example.com";
    const EMAIL: &'static str = "support@example.com";

    const VERSION: &'static str = "0.1.0";
    const AUDIO_IO_LAYOUTS: &'static [AudioIOLayout] = &[AudioIOLayout { main_input_channels: NonZeroU32::new(1), main_output_channels: NonZeroU32::new(1), ..AudioIOLayout::const_default() } ];

    type Params = UsbParams;
    fn params(&self) -> Arc<Self::Params> { self.params.clone() }

    fn initialize(&mut self, _io: &AudioIOLayout, buffer_config: &BufferConfig, _ctx: &mut impl InitContext<Self>) -> bool {
        self.fs = buffer_config.sample_rate as f32;
        self.rev.set_fs(self.fs);
        true
    }

    fn process(&mut self, buffer: &mut Buffer, _aux: &mut AuxiliaryBuffers, _ctx: &mut impl ProcessContext<Self>) -> ProcessStatus {
        let fs = self.fs;
        let out_gain = self.params.out_gain.value();
        let drive = self.params.drive.value();
        let tone = self.params.tone.value();
        let comp_amt = self.params.comp.value();
        let verb = self.params.reverb.value();
        let ull = self.params.ultra_low_latency.value();

        // analysis for auto instrument detection
        let chan = buffer.as_mut_slice().get_mut(0).unwrap();
        let n = chan.len();
        let mut in_acc = 0.0;
        let mut out_acc = 0.0;
        for i in 0..n {
            let x = chan[i];
            in_acc += x * x;
            // gather analysis buffer windowed
            if self.ana_pos < self.ana_buf.len() {
                self.ana_buf[self.ana_pos] = x * self.window[self.ana_pos];
                self.ana_pos += 1;
                if self.ana_pos == self.ana_buf.len() {
                    let mut spec = self.fft.r2c.make_output_vec();
                    let mut buf = self.ana_buf.clone();
                    self.fft.r2c.process(&mut buf, &mut spec).ok();
                    // spectral centroid
                    let mut num = 0.0f32; let mut den = 0.0f32;
                    for (k, c) in spec.iter().enumerate() {
                        let mag = (c.re * c.re + c.im * c.im).sqrt();
                        let f = k as f32 * fs / (self.ana_buf.len() as f32);
                        num += f * mag; den += mag;
                    }
                    self.centroid = if den > 0.0 { num/den } else { 0.0 };
                    self.ana_pos = 0;
                }
            }
        }
        // decide instrument
        let mut inst = match self.params.inst.value() {
            1 => Instrument::Guitar,
            2 => Instrument::Bass,
            3 => Instrument::VocalMic,
            4 => Instrument::Keys,
            _ => Instrument::Auto,
        };
        if matches!(inst, Instrument::Auto) {
            inst = if self.centroid < 200.0 { Instrument::Bass }
                else if self.centroid < 1200.0 { Instrument::Guitar }
                else if self.centroid < 2500.0 { Instrument::Keys }
                else { Instrument::VocalMic };
        }

        // set up EQ and shaping per instrument
        match inst {
            Instrument::Guitar => {
                let (b0,b1,b2,a1,a2) = coeffs_peaking(fs, 800.0, 0.7, (tone-0.5)*6.0);
                self.eq1.set_coefficients(b0,b1,b2,a1,a2);
                let (b0,b1,b2,a1,a2) = coeffs_peaking(fs, 3500.0, 0.8, (tone-0.5)*-8.0);
                self.eq2.set_coefficients(b0,b1,b2,a1,a2);
                let (b0,b1,b2,a1,a2) = coeffs_peaking(fs, 5000.0, 0.6, -12.0); // cab lpf-ish
                self.cab_lpf.set_coefficients(b0,b1,b2,a1,a2);
            }
            Instrument::Bass => {
                let (b0,b1,b2,a1,a2) = coeffs_lowshelf(fs, 80.0, 1.0, 6.0 * tone);
                self.eq1.set_coefficients(b0,b1,b2,a1,a2);
                let (b0,b1,b2,a1,a2) = coeffs_peaking(fs, 1200.0, 1.2, -4.0 * (1.0-tone));
                self.eq2.set_coefficients(b0,b1,b2,a1,a2);
                let (b0,b1,b2,a1,a2) = coeffs_peaking(fs, 6000.0, 0.7, -8.0);
                self.cab_lpf.set_coefficients(b0,b1,b2,a1,a2);
            }
            Instrument::VocalMic => {
                let (b0,b1,b2,a1,a2) = coeffs_peaking(fs, 200.0, 0.7, -3.0 * (1.0-tone));
                self.eq1.set_coefficients(b0,b1,b2,a1,a2);
                let (b0,b1,b2,a1,a2) = coeffs_peaking(fs, 3500.0, 1.0, 3.0 * tone);
                self.eq2.set_coefficients(b0,b1,b2,a1,a2);
                self.cab_lpf.set_coefficients(1.0,0.0,0.0,0.0,0.0);
            }
            Instrument::Keys => {
                let (b0,b1,b2,a1,a2) = coeffs_peaking(fs, 500.0, 0.8, 2.0 * tone);
                self.eq1.set_coefficients(b0,b1,b2,a1,a2);
                let (b0,b1,b2,a1,a2) = coeffs_peaking(fs, 2500.0, 1.0, 1.5 * (tone-0.5));
                self.eq2.set_coefficients(b0,b1,b2,a1,a2);
                self.cab_lpf.set_coefficients(1.0,0.0,0.0,0.0,0.0);
            }
            Instrument::Auto => {}
        }

        // compression settings vary by instrument
        match inst {
            Instrument::Bass => self.comp.set(-18.0 + -6.0*comp_amt, 3.0 + 3.0*comp_amt, 0.01, 0.12),
            Instrument::Guitar => self.comp.set(-14.0 + -4.0*comp_amt, 2.5 + 2.0*comp_amt, 0.005, 0.08),
            Instrument::VocalMic => self.comp.set(-16.0 + -6.0*comp_amt, 2.0 + 3.0*comp_amt, 0.003, 0.15),
            Instrument::Keys | Instrument::Auto => self.comp.set(-12.0 + -4.0*comp_amt, 2.0 + 2.0*comp_amt, 0.005, 0.12),
        }

        for i in 0..n {
            let mut x = chan[i];
            // instrument-specific drive/amp
            if !ull {
                if matches!(inst, Instrument::Guitar | Instrument::Bass) {
                    x = amp_drive(x, drive);
                }
            }
            // EQ
            x = self.eq1.process(x);
            x = self.eq2.process(x);
            // cab or lpf
            x = self.cab_lpf.process(x);
            // comp
            if comp_amt > 0.01 { x = self.comp.process(x, fs); }
            // reverb
            if verb > 0.01 && !ull { x = self.rev.process(x, verb, tone); }
            // output gain
            let out = x * 10f32.powf(out_gain/20.0);
            chan[i] = out;
            out_acc += out*out;
        }

        if n>0 { IN_RMS.store(((in_acc/n as f32).sqrt()), std::sync::atomic::Ordering::Relaxed); OUT_RMS.store(((out_acc/n as f32).sqrt()), std::sync::atomic::Ordering::Relaxed); }
        ProcessStatus::Normal
    }

    fn editor(&mut self, _context: &mut impl GuiContext<Self>) -> Option<Box<dyn Editor>> {
        let params = self.params.clone();
        create_egui_editor(self.egui_state.clone(), move |egui_ctx: &egui::Context, _| {
            egui_ctx.set_visuals(egui::Visuals::dark());
            egui::CentralPanel::default().show(egui_ctx, |ui| {
                ui.heading("Warrior USB Instrument");
                ui.horizontal(|ui| {
                    ui.label("Instrument:");
                    egui::ComboBox::from_label("")
                        .selected_text(params.inst.to_string())
                        .show_ui(ui, |ui| {
                            for (i, label) in ["Auto","Guitar","Bass","Vocal Mic","Keys"].iter().enumerate() {
                                ui.selectable_value(&mut params.inst.value_mut(), i as i32, *label);
                            }
                        });
                    ui.checkbox(&mut params.ultra_low_latency.value_mut(), "Ultra Low Latency");
                });
                ui.add(egui::Slider::new(&mut params.drive.value_mut(), 0.0..=1.0).text("Drive"));
                ui.add(egui::Slider::new(&mut params.tone.value_mut(), 0.0..=1.0).text("Tone"));
                ui.add(egui::Slider::new(&mut params.comp.value_mut(), 0.0..=1.0).text("Compression"));
                ui.add(egui::Slider::new(&mut params.reverb.value_mut(), 0.0..=1.0).text("Reverb"));
                ui.add(egui::Slider::new(&mut params.out_gain.value_mut(), -24.0..=24.0).text("Output Gain dB"));
                ui.separator();
                let in_rms = IN_RMS.load(std::sync::atomic::Ordering::Relaxed);
                let out_rms = OUT_RMS.load(std::sync::atomic::Ordering::Relaxed);
                ui.horizontal(|ui| { meter(ui, "In", in_rms); meter(ui, "Out", out_rms); });
            });
        })
    }
}

fn amp_drive(x: f32, drive: f32) -> f32 {
    // asymmetric tanh waveshaper with drive
    let g = 1.0 + 20.0 * drive;
    let y = (g * x).tanh();
    // slight asymmetry
    let a = 0.1 * drive;
    y + a * y * y * x.signum()
}

fn meter(ui: &mut egui::Ui, label: &str, rms: f32) {
    let db = 20.0 * rms.max(1e-6).log10();
    let norm = ((db + 60.0)/60.0).clamp(0.0, 1.0);
    ui.vertical(|ui| {
        ui.label(label);
        let (r, _) = ui.allocate_exact_size(egui::vec2(12.0, 80.0), egui::Sense::hover());
        let h = 80.0 * norm;
        let fill = egui::Rect::from_min_max(
            egui::pos2(r.left(), r.bottom() - h),
            egui::pos2(r.right(), r.bottom()),
        );
        ui.painter().rect_filled(fill, 2.0, egui::Color32::from_rgb(80, 200, 120));
    });
}

struct SimpleReverb {
    fs: f32,
    d1: Delay, d2: Delay, d3: Delay,
    ap1: Allpass, ap2: Allpass,
}
impl SimpleReverb { fn new() -> Self { Self { fs: 44100.0, d1: Delay::new(1277), d2: Delay::new(1499), d3: Delay::new(1999), ap1: Allpass::new(347), ap2: Allpass::new(113), } }
    fn set_fs(&mut self, fs: f32) { self.fs = fs; }
    fn process(&mut self, x: f32, mix: f32, tone: f32) -> f32 {
        // 3 comb + 2 allpass Schroeder; damp tone
        let damp = 0.3 + 0.6 * (1.0 - tone);
        let mut y = self.d1.process(x + 0.3 * y_safe(&self.d1));
        y = self.d2.process(y + 0.3 * y_safe(&self.d2));
        y = self.d3.process(y + 0.3 * y_safe(&self.d3));
        y = self.ap1.process(y, 0.7, damp);
        y = self.ap2.process(y, 0.7, damp);
        (1.0 - mix) * x + mix * y
    }
}

fn y_safe(d: &Delay) -> f32 { d.last }

struct Delay { buf: Vec<f32>, idx: usize, last: f32 }
impl Delay { fn new(n: usize) -> Self { Self { buf: vec![0.0; n.next_power_of_two()], idx: 0, last: 0.0 } }
    fn process(&mut self, x: f32) -> f32 { let y = self.buf[self.idx]; self.buf[self.idx] = x; self.idx = (self.idx+1) & (self.buf.len()-1); self.last = y; y } }

struct Allpass { buf: Vec<f32>, idx: usize }
impl Allpass { fn new(n: usize) -> Self { Self { buf: vec![0.0; n.next_power_of_two()], idx: 0 } }
    fn process(&mut self, x: f32, g: f32, damp: f32) -> f32 {
        let y = -g * x + self.buf[self.idx];
        let v = x + g * y;
        self.buf[self.idx] = damp * v + (1.0 - damp) * self.buf[self.idx];
        self.idx = (self.idx + 1) & (self.buf.len() - 1);
        y
    }
}

impl ClapPlugin for UsbPlugin {
    const CLAP_ID: &'static str = "com.warrior.usb";
    const CLAP_DESCRIPTION: Option<&'static str> = Some("USB instrument processing: detection, amp/EQ/comp/reverb");
    const CLAP_FEATURES: &'static [ClapFeature] = &[ClapFeature::AudioEffect, ClapFeature::Mono, ClapFeature::Utility];
}

impl Vst3Plugin for UsbPlugin {
    const VST3_CLASS_ID: [u8; 16] = *b"WARRUSBIPLUG001";
    const VST3_SUBCATEGORIES: &'static [Vst3SubCategory] = &[Vst3SubCategory::Fx, Vst3SubCategory::Tools];
}

nih_export_clap!(UsbPlugin);
nih_export_vst3!(UsbPlugin);
