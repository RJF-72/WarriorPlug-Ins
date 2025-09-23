//! DreamMaker by Titan - Desktop App (Rust + egui)

use eframe::egui::{self, Color32, RichText, Widget};
use eframe::{self, App, Frame, NativeOptions};
use synth_engine::SynthParams;
use cpal::traits::{DeviceTrait, HostTrait, StreamTrait};
use std::sync::mpsc::{channel, Sender};
use hound;

#[derive(Clone, Copy)]
enum PlayTarget { Synth, Sampler, Both }

enum Msg {
    Params(SynthParams),
    NoteOn(u8, u8),
    NoteOff(u8),
    Transport(bool),
    Record(bool),
    StopAndRewind,
    CreateTrack,
    LoadWav { track: usize, left: Vec<f32>, right: Vec<f32>, start_frame: u64 },
    LoadSynthSample { root: u8, path: String },
    LoadSynthSfz { path: String },
    SetPlayTarget(PlayTarget),
}

struct DreamMakerApp {
    params: SynthParams,
    _stream: cpal::Stream,
    tx: Sender<Msg>,
    keys: [bool; 12], // C4..B4
    selected_track: usize,
    wav_path: String,
    sfz_list: Vec<String>,
    selected_sfz: usize,
    play_target: PlayTarget,
}

impl App for DreamMakerApp {
    fn update(&mut self, ctx: &egui::Context, frame: &mut Frame) {
        egui::TopBottomPanel::top("top").show(ctx, |ui| {
            ui.heading(RichText::new("DreamMaker by Titan").color(Color32::from_rgb(255, 200, 40)).strong());
        });

        egui::CentralPanel::default().show(ctx, |ui| {
            // Transport
            ui.horizontal(|ui| {
                if ui.button("▶ Play").clicked() { let _ = self.tx.send(Msg::Transport(true)); }
                if ui.button("■ Stop").clicked() { let _ = self.tx.send(Msg::Transport(false)); let _ = self.tx.send(Msg::StopAndRewind); }
                if ui.button("● Rec").clicked() { let _ = self.tx.send(Msg::Record(true)); }
            });
            ui.separator();
            // Presets
            ui.horizontal(|ui| {
                let mut selected = 0;
                let names = ["Init", "Bright Pluck", "Soft Pad"];
                egui::ComboBox::from_label("Preset")
                    .selected_text(names[selected])
                    .show_ui(ui, |ui| {
                        let mut apply = None;
                        if ui.selectable_label(false, names[0]).clicked() { apply = Some(0); }
                        if ui.selectable_label(false, names[1]).clicked() { apply = Some(1); }
                        if ui.selectable_label(false, names[2]).clicked() { apply = Some(2); }
                        if let Some(ix) = apply {
                            match ix {
                                0 => { self.params = SynthParams::default(); }
                                1 => { let mut p = SynthParams::default(); p.cutoff = 4000.0; p.attack=0.002; p.decay=0.12; p.sustain=0.2; p.release=0.15; p.unison=3; self.params=p; }
                                2 => { let mut p = SynthParams::default(); p.cutoff = 1200.0; p.attack=0.3; p.decay=1.0; p.sustain=0.8; p.release=1.5; p.unison=5; self.params=p; }
                                _ => {}
                            }
                        }
                    });
            });
            ui.horizontal(|ui| {
                ui.label("Play Target");
                let mut mode = self.play_target;
                egui::ComboBox::from_label("")
                    .selected_text(match mode { PlayTarget::Synth=>"Synth", PlayTarget::Sampler=>"Sampler", PlayTarget::Both=>"Both" })
                    .show_ui(ui, |ui| {
                        if ui.selectable_label(matches!(mode, PlayTarget::Synth), "Synth").clicked() { mode = PlayTarget::Synth; }
                        if ui.selectable_label(matches!(mode, PlayTarget::Sampler), "Sampler").clicked() { mode = PlayTarget::Sampler; }
                        if ui.selectable_label(matches!(mode, PlayTarget::Both), "Both").clicked() { mode = PlayTarget::Both; }
                    });
                if mode as u8 != self.play_target as u8 {
                    self.play_target = mode;
                    let _ = self.tx.send(Msg::SetPlayTarget(self.play_target));
                }
            });
            ui.horizontal(|ui| {
                ui.label("Cutoff");
                ui.add(egui::Slider::new(&mut self.params.cutoff, 60.0..=16000.0).logarithmic(true));
                ui.label("Attack");
                ui.add(egui::Slider::new(&mut self.params.attack, 0.001..=2.0));
                ui.label("Decay");
                ui.add(egui::Slider::new(&mut self.params.decay, 0.001..=3.0));
                ui.label("Sustain");
                ui.add(egui::Slider::new(&mut self.params.sustain, 0.0..=1.0));
                ui.label("Release");
                ui.add(egui::Slider::new(&mut self.params.release, 0.001..=4.0));
            });
            ui.separator();
            ui.horizontal(|ui| {
                ui.label("Gain dB");
                ui.add(egui::Slider::new(&mut self.params.gain_db, -24.0..=6.0));
                ui.label("Unison");
                ui.add(egui::Slider::new(&mut self.params.unison, 1..=7));
            });
            ui.separator();
            // Tracks & Import
            ui.horizontal(|ui| {
                ui.label("Track");
                ui.add(egui::DragValue::new(&mut self.selected_track).clamp_range(0..=63).speed(0.2));
                if ui.button("+ Add Track").clicked() { let _ = self.tx.send(Msg::CreateTrack); }
            });
            ui.horizontal(|ui| {
                ui.label("WAV path:");
                ui.text_edit_singleline(&mut self.wav_path);
                if ui.button("Load to Track").clicked() {
                    if let Ok((l, r, sr)) = load_wav_stereo(&self.wav_path) {
                        let _ = self.tx.send(Msg::LoadWav { track: self.selected_track, left: l, right: r, start_frame: 0 });
                    }
                }
            });
            ui.horizontal(|ui| {
                ui.label("Sample -> Synth (root note + WAV):");
                static mut ROOT: u8 = 60;
                let mut root = unsafe { ROOT };
                ui.add(egui::DragValue::new(&mut root).clamp_range(0..=127));
                if ui.button("Load into Synth").clicked() {
                    unsafe { ROOT = root; }
                    let _ = self.tx.send(Msg::LoadSynthSample { root, path: self.wav_path.clone() });
                }
            });
            ui.horizontal(|ui| {
                if ui.button("Scan SFZ").clicked() {
                    self.sfz_list = find_sfz_files();
                    self.selected_sfz = 0;
                }
                egui::ComboBox::from_label("SFZ")
                    .selected_text(self.sfz_list.get(self.selected_sfz).map(|s| s.as_str()).unwrap_or("<none>"))
                    .show_ui(ui, |ui| {
                        for (i, path) in self.sfz_list.iter().enumerate() {
                            if ui.selectable_label(self.selected_sfz==i, path).clicked() { self.selected_sfz = i; }
                        }
                    });
                if ui.button("Load SFZ into Synth").clicked() {
                    if let Some(p) = self.sfz_list.get(self.selected_sfz).cloned() {
                        let _ = self.tx.send(Msg::LoadSynthSfz { path: p });
                    }
                }
            });
            ui.separator();
            // Simple on-screen keyboard C4..B4
            let names = ["C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "A#", "B"];
            ui.horizontal(|ui| {
                for (i, name) in names.iter().enumerate() {
                    let mut on = self.keys[i];
                    if ui.toggle_value(&mut on, *name).changed() {
                        self.keys[i] = on;
                        let note = 60u8 + i as u8; // C4 base
                        let _ = if on { self.tx.send(Msg::NoteOn(note, 110)) } else { self.tx.send(Msg::NoteOff(note)) };
                    }
                }
            });
        });

        // Computer keyboard mapping (A W S E D F T G Y H U J -> chromatic from C)
        ctx.input(|i| {
            use egui::Key;
            let map: &[(Key, usize)] = &[
                (Key::A,0),(Key::W,1),(Key::S,2),(Key::E,3),(Key::D,4),(Key::F,5),
                (Key::T,6),(Key::G,7),(Key::Y,8),(Key::H,9),(Key::U,10),(Key::J,11)
            ];
            for (k, idx) in map {
                if i.key_pressed(*k) { self.keys[*idx] = true; let _ = self.tx.send(Msg::NoteOn(60+*idx as u8, 110)); }
                if i.key_released(*k) { self.keys[*idx] = false; let _ = self.tx.send(Msg::NoteOff(60+*idx as u8)); }
            }
        });

        let _ = self.tx.send(Msg::Params(self.params));
        ctx.request_repaint();
    }
}

fn main() -> eframe::Result<()> {
    // Setup audio
    let host = cpal::default_host();
    let device = host.default_output_device().expect("no output device");
    let config = device.default_output_config().expect("no default config");
    let sample_rate = config.sample_rate().0 as f32;

    let (tx, rx) = channel::<Msg>();
    let synth = unsafe { synth_engine::synth_new(sample_rate) };
    let mut playing = true;
    let mut recording = false;
    let mut timeline_pos: u64 = 0; // in frames
    let mut play_target = PlayTarget::Both;
    #[derive(Clone)]
    struct Clip { left: Vec<f32>, right: Vec<f32>, start: u64 }
    struct Track { clips: Vec<Clip> }
    let mut tracks: Vec<Track> = Vec::new();
    let stream = match config.sample_format() {
        cpal::SampleFormat::F32 => device.build_output_stream(&config.clone().into(), move |data: &mut [f32], _| {
            while let Ok(msg) = rx.try_recv() {
                match msg {
                    Msg::Params(p) => unsafe { synth_engine::synth_set_params(synth, p) },
                    Msg::NoteOn(n, v) => unsafe {
                        match play_target { 
                            PlayTarget::Synth => { synth_engine::synth_note_on(synth, n, v); },
                            PlayTarget::Sampler => { synth_engine::synth_instr_note_on(synth, n, v); },
                            PlayTarget::Both => { synth_engine::synth_note_on(synth, n, v); synth_engine::synth_instr_note_on(synth, n, v); },
                        }
                    },
                    Msg::NoteOff(n) => unsafe {
                        match play_target {
                            PlayTarget::Synth => { synth_engine::synth_note_off(synth, n); },
                            PlayTarget::Sampler => { synth_engine::synth_instr_note_off(synth, n); },
                            PlayTarget::Both => { synth_engine::synth_note_off(synth, n); synth_engine::synth_instr_note_off(synth, n); },
                        }
                    },
                    Msg::Transport(p) => { playing = p; },
                    Msg::Record(r) => { recording = r; },
                    Msg::StopAndRewind => { timeline_pos = 0; },
                    Msg::CreateTrack => { tracks.push(Track{ clips: vec![] }); },
                    Msg::LoadWav{ track, left, right, start_frame } => {
                        let t = track.min(tracks.len().saturating_sub(1));
                        if tracks.is_empty() { tracks.push(Track{ clips: vec![] }); }
                        let tgt = track.min(tracks.len()-1);
                        tracks[tgt].clips.push(Clip{ left, right, start: start_frame });
                    },
                    Msg::LoadSynthSample { root, path } => {
                        let bytes = path.as_bytes();
                        unsafe { synth_engine::synth_load_sample_instrument(synth, root, bytes.as_ptr(), bytes.len()); }
                    },
                    Msg::LoadSynthSfz { path } => {
                        let bytes = path.as_bytes();
                        unsafe { synth_engine::synth_load_sfz_instrument(synth, bytes.as_ptr(), bytes.len()); }
                    },
                    Msg::SetPlayTarget(pt) => { play_target = pt; },
                }
            }
            let frames = data.len()/2;
            let mut l = vec![0.0f32; frames];
            let mut r = vec![0.0f32; frames];
            if playing {
                unsafe { synth_engine::synth_render(synth, l.as_mut_ptr(), r.as_mut_ptr(), frames) };
            } else {
                // silence
            }
            // Mix tracks
            for i in 0..frames {
                let pos = timeline_pos + i as u64;
                for tr in &tracks {
                    for c in &tr.clips {
                        if pos >= c.start {
                            let idx = (pos - c.start) as usize;
                            if idx < c.left.len() { l[i] += c.left[idx]; r[i] += c.right[idx]; }
                        }
                    }
                }
            }
            if playing { timeline_pos += frames as u64; }
            for i in 0..frames { data[2*i] = l[i]; data[2*i+1] = r[i]; }
        }, move |err| eprintln!("audio stream error: {err}"), None),
        cpal::SampleFormat::I16 => device.build_output_stream(&config.clone().into(), move |data: &mut [i16], _| {
            while let Ok(msg) = rx.try_recv() { match msg {
                Msg::Params(p) => unsafe { synth_engine::synth_set_params(synth, p) },
                Msg::NoteOn(n, v) => unsafe { match play_target { PlayTarget::Synth => synth_engine::synth_note_on(synth, n, v), PlayTarget::Sampler => synth_engine::synth_instr_note_on(synth, n, v), PlayTarget::Both => { synth_engine::synth_note_on(synth, n, v); synth_engine::synth_instr_note_on(synth, n, v); } } },
                Msg::NoteOff(n) => unsafe { match play_target { PlayTarget::Synth => synth_engine::synth_note_off(synth, n), PlayTarget::Sampler => synth_engine::synth_instr_note_off(synth, n), PlayTarget::Both => { synth_engine::synth_note_off(synth, n); synth_engine::synth_instr_note_off(synth, n); } } },
                Msg::Transport(p) => { playing = p; },
                Msg::Record(r) => { recording = r; },
                Msg::StopAndRewind => { timeline_pos = 0; },
                Msg::CreateTrack => { tracks.push(Track{ clips: vec![] }); },
                Msg::LoadWav{ track, left, right, start_frame } => {
                    if tracks.is_empty() { tracks.push(Track{ clips: vec![] }); }
                    let tgt = track.min(tracks.len()-1);
                    tracks[tgt].clips.push(Clip{ left, right, start: start_frame });
                },
                Msg::LoadSynthSample { root, path } => { let bytes = path.as_bytes(); unsafe { synth_engine::synth_load_sample_instrument(synth, root, bytes.as_ptr(), bytes.len()); } },
                Msg::LoadSynthSfz { path } => { let bytes = path.as_bytes(); unsafe { synth_engine::synth_load_sfz_instrument(synth, bytes.as_ptr(), bytes.len()); } },
                Msg::SetPlayTarget(pt) => { play_target = pt; },
            } }
            let frames = data.len()/2; let mut l=vec![0.0f32;frames]; let mut r=vec![0.0f32;frames];
            if playing { unsafe { synth_engine::synth_render(synth, l.as_mut_ptr(), r.as_mut_ptr(), frames) }; }
            for i in 0..frames {
                let pos = timeline_pos + i as u64;
                for tr in &tracks { for c in &tr.clips { if pos >= c.start { let idx=(pos-c.start) as usize; if idx < c.left.len() { l[i]+=c.left[idx]; r[i]+=c.right[idx]; } } } }
            }
            if playing { timeline_pos += frames as u64; }
            for i in 0..frames { data[2*i] = (l[i].clamp(-1.0,1.0)*32767.0) as i16; data[2*i+1] = (r[i].clamp(-1.0,1.0)*32767.0) as i16; }
        }, move |err| eprintln!("audio stream error: {err}"), None),
        cpal::SampleFormat::U16 => device.build_output_stream(&config.into(), move |data: &mut [u16], _| {
            while let Ok(msg) = rx.try_recv() { match msg {
                Msg::Params(p) => unsafe { synth_engine::synth_set_params(synth, p) },
                Msg::NoteOn(n, v) => unsafe { match play_target { PlayTarget::Synth => synth_engine::synth_note_on(synth, n, v), PlayTarget::Sampler => synth_engine::synth_instr_note_on(synth, n, v), PlayTarget::Both => { synth_engine::synth_note_on(synth, n, v); synth_engine::synth_instr_note_on(synth, n, v); } } },
                Msg::NoteOff(n) => unsafe { match play_target { PlayTarget::Synth => synth_engine::synth_note_off(synth, n), PlayTarget::Sampler => synth_engine::synth_instr_note_off(synth, n), PlayTarget::Both => { synth_engine::synth_note_off(synth, n); synth_engine::synth_instr_note_off(synth, n); } } },
                Msg::Transport(p) => { playing = p; },
                Msg::Record(r) => { recording = r; },
                Msg::StopAndRewind => { timeline_pos = 0; },
                Msg::CreateTrack => { tracks.push(Track{ clips: vec![] }); },
                Msg::LoadWav{ track, left, right, start_frame } => {
                    if tracks.is_empty() { tracks.push(Track{ clips: vec![] }); }
                    let tgt = track.min(tracks.len()-1);
                    tracks[tgt].clips.push(Clip{ left, right, start: start_frame });
                },
                Msg::LoadSynthSample { root, path } => { let bytes = path.as_bytes(); unsafe { synth_engine::synth_load_sample_instrument(synth, root, bytes.as_ptr(), bytes.len()); } },
                Msg::LoadSynthSfz { path } => { let bytes = path.as_bytes(); unsafe { synth_engine::synth_load_sfz_instrument(synth, bytes.as_ptr(), bytes.len()); } },
                Msg::SetPlayTarget(pt) => { play_target = pt; },
            } }
            let frames = data.len()/2; let mut l=vec![0.0f32;frames]; let mut r=vec![0.0f32;frames];
            if playing { unsafe { synth_engine::synth_render(synth, l.as_mut_ptr(), r.as_mut_ptr(), frames) }; }
            for i in 0..frames { let pos = timeline_pos + i as u64; for tr in &tracks { for c in &tr.clips { if pos >= c.start { let idx=(pos-c.start) as usize; if idx < c.left.len() { l[i]+=c.left[idx]; r[i]+=c.right[idx]; } } } } }
            if playing { timeline_pos += frames as u64; }
            for i in 0..frames { let sl=((l[i].clamp(-1.0,1.0)*0.5+0.5)*65535.0) as u16; let sr=((r[i].clamp(-1.0,1.0)*0.5+0.5)*65535.0) as u16; data[2*i]=sl; data[2*i+1]=sr; }
        }, move |err| eprintln!("audio stream error: {err}"), None),
        _ => panic!("Unsupported sample format"),
    };
    let stream = stream.expect("Failed to build audio stream");
    stream.play().expect("failed to play stream");

    let app = DreamMakerApp { params: SynthParams::default(), _stream: stream, tx, keys: [false; 12], selected_track: 0, wav_path: String::new(), sfz_list: Vec::new(), selected_sfz: 0, play_target: PlayTarget::Both };
    let native_options = NativeOptions::default();
    eframe::run_native("DreamMaker by Titan", native_options, Box::new(|_| Box::new(app)))
}

fn load_wav_stereo(path: &str) -> anyhow::Result<(Vec<f32>, Vec<f32>, u32)> {
    let mut reader = hound::WavReader::open(path)?;
    let spec = reader.spec();
    let sr = spec.sample_rate;
    let channels = spec.channels;
    let mut left = Vec::new();
    let mut right = Vec::new();
    match spec.sample_format {
        hound::SampleFormat::Int => {
            let bits = spec.bits_per_sample;
            if bits <= 16 {
                for s in reader.samples::<i16>() {
                    let v = s? as f32 / 32768.0;
                    if channels == 1 { left.push(v); right.push(v); } else { if left.len() == right.len() { left.push(v); } else { right.push(v); } }
                }
            } else {
                for s in reader.samples::<i32>() {
                    let v = (s? as f64 / 2147483648.0) as f32;
                    if channels == 1 { left.push(v); right.push(v); } else { if left.len() == right.len() { left.push(v); } else { right.push(v); } }
                }
            }
        }
        hound::SampleFormat::Float => {
            for s in reader.samples::<f32>() {
                let v = s?;
                if channels == 1 { left.push(v); right.push(v); } else { if left.len() == right.len() { left.push(v); } else { right.push(v); } }
            }
        }
    }
    Ok((left, right, sr))
}

fn find_sfz_files() -> Vec<String> {
    let mut out = Vec::new();
    fn scan_dir(dir: &std::path::Path, out: &mut Vec<String>, depth: usize) {
        if depth == 0 { return; }
        if let Ok(rd) = std::fs::read_dir(dir) {
            for e in rd.flatten() {
                let p = e.path();
                if p.is_dir() { scan_dir(&p, out, depth-1); }
                else if let Some(ext) = p.extension() { if ext.to_string_lossy().eq_ignore_ascii_case("sfz") { out.push(p.to_string_lossy().to_string()); } }
            }
        }
    }
    // scan common subfolders to keep it fast
    let roots = [".", "Bass", "Drums", "Guitar", "Piano", "Playable RealDrums", "Playable RealTracks", "Strings", "Synth"];
    for r in roots { scan_dir(std::path::Path::new(r), &mut out, 2); }
    out.sort();
    out
}
