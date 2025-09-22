# WarriorPlug-Ins

This workspace contains multiple, separate commercial audio plugins for the Titan7 DAW. Each plugin lives in its own folder, ships separately, and has its own pricing, SKU, and license terms.

Plugins:
- `plugins/warrior_headphone` — Warrior Headphone (SKU: WH-01)
- `plugins/warrior_vocal` — Warrior Vocal Repair (SKU: WV-01)
- `plugins/warrior_usb` — Warrior USB Instrument (SKU: WU-01)
- `plugins/titan7_mix_master` — Titan7 Mixing & Mastering Suite (SKU: T7-MM-01)
 - `apps/dreammaker` — DreamMaker by Titan desktop app (SKU: DM-TITAN-001)

Core crates:
- `crates/common_dsp` — shared DSP utilities
- `crates/synth_engine` — polyphonic synth engine with C ABI

Licensing and Copyright:
- Each plugin folder includes a dedicated `LICENSE.txt` (EULA) and `COPYRIGHT.txt`.
- Each `Cargo.toml` declares a `license-file` and embeds pricing/SKU under `[package.metadata.warrior]`.
- Copyright © 2025 Warrior Audio. All rights reserved. Redistribution is prohibited.

Build notes:
- This is a Rust audio plugin workspace using `nih-plug` with egui UIs.
- To build VST3/CLAP, install Rust and run a workspace build. Platform-specific packaging is handled per plugin.
 - To run DreamMaker desktop app: `cargo run -p dreammaker` (requires Rust and a working audio backend via CPAL: PulseAudio/ALSA on Linux, CoreAudio on macOS, WASAPI on Windows).