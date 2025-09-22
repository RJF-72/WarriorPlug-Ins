// Proprietary — Warrior Audio (c) 2025
use num_traits::Float;

#[derive(Copy, Clone, Debug, Default)]
pub struct Biquad<T: Float> {
    b0: T,
    b1: T,
    b2: T,
    a1: T,
    a2: T,
    z1: T,
    z2: T,
}

impl<T: Float> Biquad<T> {
    pub fn new() -> Self { Self { b0: T::one(), b1: T::zero(), b2: T::zero(), a1: T::zero(), a2: T::zero(), z1: T::zero(), z2: T::zero() } }

    pub fn reset(&mut self) { self.z1 = T::zero(); self.z2 = T::zero(); }

    pub fn set_coefficients(&mut self, b0: T, b1: T, b2: T, a1: T, a2: T) {
        self.b0 = b0; self.b1 = b1; self.b2 = b2; self.a1 = a1; self.a2 = a2;
    }

    pub fn process(&mut self, x: T) -> T {
        let y = self.b0 * x + self.z1;
        self.z1 = self.b1 * x - self.a1 * y + self.z2;
        self.z2 = self.b2 * x - self.a2 * y;
        y
    }
}

pub enum BiquadType { LowShelf, HighShelf, Peaking, LowPass, HighPass }

pub fn coeffs_peaking(fs: f32, f0: f32, q: f32, gain_db: f32) -> (f32,f32,f32,f32,f32) {
    let a = 10f32.powf(gain_db / 40.0);
    let w0 = 2.0 * std::f32::consts::PI * f0 / fs;
    let alpha = w0.sin() / (2.0 * q.max(1e-3));
    let cosw = w0.cos();
    let b0 = 1.0 + alpha * a;
    let b1 = -2.0 * cosw;
    let b2 = 1.0 - alpha * a;
    let a0 = 1.0 + alpha / a;
    let a1 = -2.0 * cosw;
    let a2 = 1.0 - alpha / a;
    (b0/a0, b1/a0, b2/a0, a1/a0, a2/a0)
}

pub fn coeffs_lowshelf(fs: f32, f0: f32, slope: f32, gain_db: f32) -> (f32,f32,f32,f32,f32) {
    let a = 10f32.powf(gain_db / 40.0);
    let w0 = 2.0 * std::f32::consts::PI * f0 / fs;
    let cosw = w0.cos();
    let sinw = w0.sin();
    let alpha = sinw/2.0 * ((a + 1.0/a) * (1.0/slope - 1.0) + 2.0).sqrt();
    let b0 = a*((a+1.0) - (a-1.0)*cosw + 2.0*alpha);
    let b1 = 2.0*a*((a-1.0) - (a+1.0)*cosw);
    let b2 = a*((a+1.0) - (a-1.0)*cosw - 2.0*alpha);
    let a0 = (a+1.0) + (a-1.0)*cosw + 2.0*alpha;
    let a1 = -2.0*((a-1.0) + (a+1.0)*cosw);
    let a2 = (a+1.0) + (a-1.0)*cosw - 2.0*alpha;
    (b0/a0, b1/a0, b2/a0, a1/a0, a2/a0)
}
