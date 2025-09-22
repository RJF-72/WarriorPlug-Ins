// Proprietary — Warrior Audio (c) 2025
const MAX_DELAY: usize = 256; // ~5.8ms @44.1k

pub struct Crossfeed {
    amount: f32,
    delay_samps: usize,
    dl: [f32; MAX_DELAY],
    dr: [f32; MAX_DELAY],
    wl: usize,
    wr: usize,
    // simple first-order low-shelf on the cross path to reduce bass bleed
    shelf_zl: f32,
    shelf_zr: f32,
    shelf_a: f32,
}

impl Crossfeed {
    pub fn new() -> Self {
        Self {
            amount: 0.15,
            delay_samps: 32,
            dl: [0.0; MAX_DELAY],
            dr: [0.0; MAX_DELAY],
            wl: 0,
            wr: 0,
            shelf_zl: 0.0,
            shelf_zr: 0.0,
            shelf_a: 0.3,
        }
    }
    pub fn set_amount(&mut self, amt: f32) { self.amount = amt.clamp(0.0, 1.0) * 0.5; }
    pub fn set_delay_ms(&mut self, ms: f32, fs: f32) {
        let d = (ms * 1e-3 * fs).round() as usize;
        self.delay_samps = d.min(MAX_DELAY - 1).max(0);
    }
    #[inline]
    fn shelf(&mut self, x: f32, zr: &mut f32) -> f32 {
        // one-pole lowpass as crossfeed EQ
        let y = self.shelf_a * x + (1.0 - self.shelf_a) * *zr;
        *zr = y;
        y
    }
    pub fn process(&mut self, l: f32, r: f32) -> (f32, f32) {
        // write to delay lines
        self.dl[self.wl] = l;
        self.dr[self.wr] = r;
        let rl = (self.wr + MAX_DELAY - self.delay_samps) % MAX_DELAY;
        let rr = (self.wl + MAX_DELAY - self.delay_samps) % MAX_DELAY;
        let ld = self.dr[rl]; // delayed right into left
        let rd = self.dl[rr]; // delayed left into right
        self.wl = (self.wl + 1) % MAX_DELAY;
        self.wr = (self.wr + 1) % MAX_DELAY;

        let xl = self.shelf(ld, &mut self.shelf_zl);
        let xr = self.shelf(rd, &mut self.shelf_zr);
        let ll = l + self.amount * (xl - l);
        let rr = r + self.amount * (xr - r);
        (ll, rr)
    }
}
