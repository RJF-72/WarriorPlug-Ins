// Proprietary — Warrior Audio (c) 2025
use serde::{Deserialize, Serialize};

#[derive(Clone, Serialize, Deserialize, Default, Debug)]
pub struct CalibrationCurve {
    pub name: String,
    pub points: Vec<(f32, f32)>, // (frequency Hz, gain dB)
}

impl CalibrationCurve {
    pub fn gain_at(&self, f: f32) -> f32 {
        if self.points.is_empty() { return 0.0; }
        let mut last = self.points[0];
        for p in &self.points {
            if p.0 >= f { // linear interp
                let t = if p.0 - last.0 != 0.0 { (f - last.0) / (p.0 - last.0) } else { 0.0 };
                return last.1 + t * (p.1 - last.1);
            }
            last = *p;
        }
        last.1
    }
}

pub fn stock_curves() -> Vec<CalibrationCurve> {
    vec![
        CalibrationCurve { name: "HD800S".to_string(), points: vec![ (20.0,-3.0),(60.0,-2.0),(100.0,-1.0),(200.0,0.5),(1_000.0,1.5),(3_000.0,2.0),(6_000.0,1.0),(10_000.0,0.0),(16_000.0,-1.0) ] },
        CalibrationCurve { name: "DT1990Pro".to_string(), points: vec![ (20.0,-2.0),(80.0,-1.0),(200.0,0.0),(1_000.0,1.0),(4_000.0,3.0),(8_000.0,2.0),(12_000.0,0.5),(16_000.0,0.0) ] },
        CalibrationCurve { name: "LCD-X".to_string(), points: vec![ (20.0,1.0),(60.0,0.5),(200.0,0.0),(1_000.0,0.5),(4_000.0,-0.5),(8_000.0,-1.0),(12_000.0,0.0),(16_000.0,0.5) ] },
    ]
}
