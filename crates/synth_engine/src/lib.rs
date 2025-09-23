// Warrior Synth Engine — Proprietary (c) 2025 Warrior Audio

use std::f32::consts::PI;
use std::path::{Path, PathBuf};
use std::fs;
use anyhow;

#[repr(C)]
#[derive(Clone, Copy)]
pub struct SynthParams {
    pub gain_db: f32,
    pub cutoff: f32,
    pub resonance: f32,
    pub attack: f32,
    pub decay: f32,
    pub sustain: f32,
    pub release: f32,
    pub unison: i32,
}

impl Default for SynthParams {
    fn default() -> Self {
        Self { gain_db: -6.0, cutoff: 1200.0, resonance: 0.2, attack: 0.005, decay: 0.1, sustain: 0.7, release: 0.2, unison: 1 }
    }
}

#[derive(Clone, Copy)]
struct Voice {
    active: bool,
    note: u8,
    phase: f32,
    vel: f32,
    env: f32,
    env_state: u8, // 0=a,1=d,2=s,3=r
    filt_z1: f32,
}

impl Default for Voice { fn default() -> Self { Self { active: false, note: 0, phase: 0.0, vel: 0.0, env: 0.0, env_state: 3, filt_z1: 0.0 } } }

pub struct SynthEngine {
    fs: f32,
    params: SynthParams,
    voices: [Voice; 32],
    // Simple sample-based instrument support
    instr: Option<Instrument>,
}

impl SynthEngine {
    pub fn new(fs: f32) -> Self { Self { fs, params: SynthParams::default(), voices: [Voice::default(); 32], instr: None } }
    pub fn set_params(&mut self, p: &SynthParams) { self.params = *p; }
    fn env_step(params: &SynthParams, fs: f32, v: &mut Voice) {
        let a = (1.0 - (-1.0/(params.attack*fs)).exp()).clamp(0.0, 1.0);
        let d = (1.0 - (-1.0/(params.decay*fs)).exp()).clamp(0.0, 1.0);
        let r = (1.0 - (-1.0/(params.release*fs)).exp()).clamp(0.0, 1.0);
        match v.env_state {
            0 => { v.env += a * (1.0 - v.env); if v.env >= 0.999 { v.env_state = 1; } }
            1 => { v.env += d * (params.sustain - v.env); if (v.env - params.sustain).abs() < 1e-3 { v.env_state = 2; } }
            2 => {}
            _ => { v.env += r * (0.0 - v.env); if v.env < 1e-4 { v.active = false; }
            }
        }
    }
    fn note_freq(note: u8) -> f32 { 440.0 * 2f32.powf((note as f32 - 69.0)/12.0) }
    pub fn note_on(&mut self, note: u8, vel: u8) {
        if let Some(v) = self.voices.iter_mut().find(|v| !v.active) {
            v.active = true; v.note = note; v.vel = vel as f32 / 127.0; v.phase = 0.0; v.env=0.0; v.env_state=0; v.filt_z1=0.0;
        }
    }
    pub fn note_off(&mut self, note: u8) {
        for v in &mut self.voices { if v.active && v.note == note { v.env_state = 3; } }
    }
    pub fn render(&mut self, out_l: &mut [f32], out_r: &mut [f32]) {
        let n = out_l.len().min(out_r.len());
        for i in 0..n { out_l[i] = 0.0; out_r[i] = 0.0; }
        let g = 10f32.powf(self.params.gain_db/20.0);
        // Synth voices
        for v in &mut self.voices {
            if !v.active { continue; }
            let f0 = Self::note_freq(v.note);
            let uni = self.params.unison.max(1) as usize;
            for i in 0..n {
                Self::env_step(&self.params, self.fs, v);
                let mut s = 0.0;
                for k in 0..uni {
                    let det = (k as f32 - (uni as f32 - 1.0)/2.0) * 0.003;
                    v.phase += 2.0*PI*(f0*(1.0+det))/self.fs;
                    if v.phase > 2.0*PI { v.phase -= 2.0*PI; }
                    s += (v.phase).sin();
                }
                s /= uni as f32;
                // one-pole lowpass filter
                let rc = 1.0/(2.0*PI*self.params.cutoff.max(20.0));
                let a = (1.0/(1.0 + (rc*self.fs).recip())).clamp(0.0, 1.0);
                v.filt_z1 = a*s + (1.0 - a)*v.filt_z1;
                let y = v.filt_z1 * v.env * v.vel * g;
                out_l[i] += y;
                out_r[i] += y;
            }
        }
        // Sample instrument rendering
        if let Some(instr) = &mut self.instr {
            instr.render(n, out_l, out_r);
        }
    }
}

// C ABI for universal integration
#[repr(C)]
#[derive(Copy, Clone)]
pub struct SynthHandle(*mut SynthEngine);

// SAFETY: SynthEngine operations are designed to be thread-safe for audio processing
unsafe impl Send for SynthHandle {}

#[no_mangle]
pub extern "C" fn synth_new(fs: f32) -> SynthHandle {
    let b = Box::new(SynthEngine::new(fs));
    SynthHandle(Box::into_raw(b))
}

#[cfg(test)]
mod tests {
    use super::*;
    #[test]
    fn env_attacks() {
        let mut e = SynthEngine::new(48000.0);
        e.note_on(60, 100);
        let mut l = vec![0.0f32; 1024];
        let mut r = vec![0.0f32; 1024];
        e.render(&mut l, &mut r);
        assert!(l.iter().any(|&x| x.abs() > 0.0));
    }
}

#[no_mangle]
pub extern "C" fn synth_free(h: SynthHandle) {
    if !h.0.is_null() { unsafe { drop(Box::from_raw(h.0)); } }
}

#[no_mangle]
pub extern "C" fn synth_set_params(h: SynthHandle, p: SynthParams) {
    if h.0.is_null() { return; }
    unsafe { (*h.0).set_params(&p); }
}

#[no_mangle]
pub extern "C" fn synth_note_on(h: SynthHandle, note: u8, vel: u8) {
    if h.0.is_null() { return; }
    unsafe { (*h.0).note_on(note, vel); }
}

#[no_mangle]
pub extern "C" fn synth_note_off(h: SynthHandle, note: u8) {
    if h.0.is_null() { return; }
    unsafe { (*h.0).note_off(note); }
}

#[no_mangle]
pub extern "C" fn synth_render(h: SynthHandle, out_l: *mut f32, out_r: *mut f32, frames: usize) {
    if h.0.is_null() || out_l.is_null() || out_r.is_null() { return; }
    unsafe {
        let out_l = std::slice::from_raw_parts_mut(out_l, frames);
        let out_r = std::slice::from_raw_parts_mut(out_r, frames);
        (*h.0).render(out_l, out_r);
    }
}

// --- Minimal sample instrument (WAV per keyrange) ---

struct Sample {
    left: Vec<f32>,
    right: Vec<f32>,
    root_note: u8,
}

struct PlayingSample {
    idx: usize,
    pos: f32,
    note: u8,
    vel: f32,
    done: bool,
}

struct Instrument {
    samples: Vec<Sample>,
    playing: Vec<PlayingSample>,
}

impl Instrument {
    fn note_on(&mut self, note: u8, vel: u8) {
        // Choose the closest root_note sample
        if let Some((idx, _)) = self.samples.iter().enumerate().min_by_key(|(_, s)| (s.root_note as i32 - note as i32).abs()) {
            self.playing.push(PlayingSample{ idx, pos: 0.0, note, vel: vel as f32 / 127.0, done: false });
        }
    }
    fn note_off(&mut self, _note: u8) {
        // For now, let samples play to end; envelopes/fades can be added later.
    }
    fn render(&mut self, n: usize, out_l: &mut [f32], out_r: &mut [f32]) {
        for p in &mut self.playing {
            let s = &self.samples[p.idx];
            // naive resampling factor based on semitone ratio from root
            let ratio = 2f32.powf((p.note as f32 - s.root_note as f32)/12.0);
            for i in 0..n {
                let ip = p.pos as usize;
                if ip >= s.left.len() { p.done = true; break; }
                let l = s.left[ip];
                let r = s.right[ip];
                out_l[i] += l * p.vel;
                out_r[i] += r * p.vel;
                p.pos += ratio;
            }
        }
        self.playing.retain(|p| !p.done);
    }
}

fn load_wav(path: &str) -> anyhow::Result<(Vec<f32>, Vec<f32>)> {
    let mut reader = hound::WavReader::open(path)?;
    let spec = reader.spec();
    let channels = spec.channels;
    let mut left = Vec::new();
    let mut right = Vec::new();
    match spec.sample_format {
        hound::SampleFormat::Int => {
            let bits = spec.bits_per_sample;
            if bits <= 16 {
                for s in reader.samples::<i16>() { let v = s? as f32 / 32768.0; if channels == 1 { left.push(v); right.push(v); } else { if left.len()==right.len() { left.push(v); } else { right.push(v); } } }
            } else {
                for s in reader.samples::<i32>() { let v = (s? as f64 / 2147483648.0) as f32; if channels == 1 { left.push(v); right.push(v); } else { if left.len()==right.len() { left.push(v); } else { right.push(v); } } }
            }
        }
        hound::SampleFormat::Float => {
            for s in reader.samples::<f32>() { let v = s?; if channels == 1 { left.push(v); right.push(v); } else { if left.len()==right.len() { left.push(v); } else { right.push(v); } } }
        }
    }
    Ok((left, right))
}

#[no_mangle]
pub extern "C" fn synth_load_sample_instrument(h: SynthHandle, root_note: u8, wav_path: *const u8, wav_len: usize) -> bool {
    if h.0.is_null() || wav_path.is_null() { return false; }
    let path = unsafe { std::str::from_utf8_unchecked(std::slice::from_raw_parts(wav_path, wav_len)) };
    let (l, r) = match load_wav(path) { Ok(x) => x, Err(_) => return false };
    unsafe {
        let se = &mut *h.0;
        if se.instr.is_none() { se.instr = Some(Instrument{ samples: vec![], playing: vec![] }); }
        if let Some(instr) = &mut se.instr {
            instr.samples.push(Sample{ left: l, right: r, root_note });
        }
    }
    true
}

#[no_mangle]
pub extern "C" fn synth_instr_note_on(h: SynthHandle, note: u8, vel: u8) {
    if h.0.is_null() { return; }
    unsafe { if let Some(instr) = &mut (*h.0).instr { instr.note_on(note, vel); } }
}

#[no_mangle]
pub extern "C" fn synth_instr_note_off(h: SynthHandle, note: u8) {
    if h.0.is_null() { return; }
    unsafe { if let Some(instr) = &mut (*h.0).instr { instr.note_off(note); } }
}

// Load a very simple SFZ: supports per-region tokens: sample=, key=, pitch_keycenter=
fn load_sfz_to_instrument(sfz_path: &str) -> anyhow::Result<Instrument> {
    let text = fs::read_to_string(sfz_path)?;
    let base_dir = Path::new(sfz_path).parent().unwrap_or(Path::new("."));
    let mut samples: Vec<Sample> = Vec::new();
    let mut cur_sample: Option<PathBuf> = None;
    let mut cur_key: Option<u8> = None;
    let mut cur_pkc: Option<u8> = None;
    for line in text.lines() {
        let line = line.trim();
        if line.is_empty() || line.starts_with("//") || line.starts_with("#") { continue; }
        if line.starts_with("<region>") {
            // commit previous pending region if complete
            if let Some(path) = cur_sample.take() {
                let root = cur_pkc.or(cur_key).unwrap_or(60);
                if let Ok((l,r)) = load_wav(path.to_string_lossy().as_ref()) {
                    samples.push(Sample{ left: l, right: r, root_note: root });
                }
            }
            cur_sample = None; cur_key=None; cur_pkc=None;
            continue;
        }
        for tok in line.split_whitespace() {
            if let Some(rest) = tok.strip_prefix("sample=") {
                let p = base_dir.join(rest);
                cur_sample = Some(p);
            } else if let Some(rest) = tok.strip_prefix("key=") {
                if let Ok(v) = rest.parse::<u8>() { cur_key = Some(v); }
            } else if let Some(rest) = tok.strip_prefix("pitch_keycenter=") {
                if let Ok(v) = rest.parse::<u8>() { cur_pkc = Some(v); }
            }
        }
    }
    // commit last region
    if let Some(path) = cur_sample.take() {
        let root = cur_pkc.or(cur_key).unwrap_or(60);
        if let Ok((l,r)) = load_wav(path.to_string_lossy().as_ref()) {
            samples.push(Sample{ left: l, right: r, root_note: root });
        }
    }
    Ok(Instrument{ samples, playing: Vec::new() })
}

#[no_mangle]
pub extern "C" fn synth_load_sfz_instrument(h: SynthHandle, sfz_path: *const u8, sfz_len: usize) -> bool {
    if h.0.is_null() || sfz_path.is_null() { return false; }
    let path = unsafe { std::str::from_utf8_unchecked(std::slice::from_raw_parts(sfz_path, sfz_len)) };
    match load_sfz_to_instrument(path) {
        Ok(instr) => unsafe { (*h.0).instr = Some(instr); true },
        Err(_) => false,
    }
}
