// Proprietary — Warrior Audio (c) 2025
pub struct Yin {
    buf: Vec<f32>,
    tau_max: usize,
    threshold: f32,
}

impl Yin {
    pub fn new(frame: usize, tau_max: usize, threshold: f32) -> Self {
        Self { buf: vec![0.0; frame], tau_max, threshold }
    }

    pub fn detect(&self, x: &[f32], fs: f32) -> Option<f32> {
        let n = x.len().min(self.buf.len());
        if n < 32 { return None; }
        let mut d: Vec<f32> = vec![0.0; self.tau_max.min(n)];
        // difference function
        for tau in 1..d.len() {
            let mut sum = 0.0;
            for i in 0..(n - tau) {
                let diff = x[i] - x[i + tau];
                sum += diff * diff;
            }
            d[tau] = sum;
        }
        // cumulative mean normalized difference
        let mut cmnd: Vec<f32> = vec![0.0; d.len()];
        let mut running_sum = 0.0;
        for tau in 1..d.len() - 1 {
            running_sum += d[tau];
            cmnd[tau] = d[tau] * tau as f32 / running_sum.max(1e-9);
        }
        // absolute threshold
        for tau in 2..cmnd.len() - 1 {
            if cmnd[tau] < self.threshold {
                // parabolic interpolation around tau for better precision
                let y1 = cmnd[tau - 1];
                let y2 = cmnd[tau];
                let y3 = cmnd[tau + 1];
                let denom = (y1 - 2.0 * y2 + y3);
                let tau_refined = if denom.abs() > 1e-9 {
                    tau as f32 + 0.5 * (y1 - y3) / denom
                } else {
                    tau as f32
                };
                let f0 = fs / tau_refined.max(1.0);
                if f0.is_finite() && f0 > 40.0 && f0 < 1000.0 {
                    return Some(f0);
                } else { return None; }
            }
        }
        None
    }
}
