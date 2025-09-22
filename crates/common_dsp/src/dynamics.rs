// Proprietary — Warrior Audio (c) 2025
pub struct SimpleCompressor {
    thresh: f32,
    ratio: f32,
    attack: f32,
    release: f32,
    env: f32,
}

impl SimpleCompressor {
    pub fn new() -> Self { Self { thresh: -12.0, ratio: 2.0, attack: 0.01, release: 0.1, env: 0.0 } }
    pub fn set(&mut self, thresh: f32, ratio: f32, attack: f32, release: f32) { self.thresh=thresh; self.ratio=ratio; self.attack=attack; self.release=release; }
    fn coef(t: f32, fs: f32) -> f32 { (-1.0/(t*fs)).exp() }
    pub fn process(&mut self, x: f32, fs: f32) -> f32 {
        let a = Self::coef(self.attack.max(1e-4), fs);
        let r = Self::coef(self.release.max(1e-4), fs);
        let rect = x.abs();
        self.env = if rect > self.env { a*self.env + (1.0-a)*rect } else { r*self.env + (1.0-r)*rect };
        let db = 20.0 * self.env.max(1e-9).log10();
        let gr_db = if db > self.thresh { (self.thresh + (db - self.thresh)/self.ratio) - db } else { 0.0 };
        let gr = 10f32.powf(gr_db/20.0);
        x * gr
    }
}
