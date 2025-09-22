// Proprietary — Warrior Audio (c) 2025
use once_cell::sync::Lazy;
use realfft::{RealFftPlanner, RealToComplex};
use std::sync::Mutex;

static PLANNER: Lazy<Mutex<RealFftPlanner<f32>>> = Lazy::new(|| Mutex::new(RealFftPlanner::new()));

pub struct FftCache {
    size: usize,
    pub r2c: std::sync::Arc<dyn RealToComplex<f32>>,
}

impl FftCache {
    pub fn new(size: usize) -> Self {
        let mut planner = PLANNER.lock().unwrap();
        let r2c = planner.plan_fft_forward(size);
        Self { size, r2c }
    }

    pub fn size(&self) -> usize { self.size }
}

pub fn hann(n: usize) -> Vec<f32> {
    (0..n)
        .map(|i| 0.5 * (1.0 - (2.0 * std::f32::consts::PI * i as f32 / (n as f32)).cos()))
        .collect()
}

pub fn magnitude_spectrum(cache: &FftCache, input: &[f32]) -> Vec<f32> {
    let n = cache.size;
    let mut buf = input.to_vec();
    buf.resize(n, 0.0);
    let mut spectrum = cache.r2c.make_output_vec();
    cache.r2c.process(&mut buf, &mut spectrum).ok();
    spectrum.iter().map(|c| (c.re * c.re + c.im * c.im).sqrt()).collect()
}
