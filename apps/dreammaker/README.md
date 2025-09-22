DreamMaker by Titan (Desktop App)

Overview
- Native desktop app built in Rust using eframe/egui for UI and the shared synth_engine for sound generation.

Build & Run
- Ensure Rust is installed.
- From the repository root, build: cargo build -p dreammaker --release
- Run the app: cargo run -p dreammaker

Notes
- Audio output wiring via cpal will be added next.
- Parameters map 1:1 to SynthParams in synth_engine.
