use wasm_bindgen::prelude::*;
use crate::sand_kernel::SandKernel;

#[wasm_bindgen]
pub struct SettlementEngine {
    settle_threshold: f64,
    blend_factor: f64,
}

#[wasm_bindgen]
impl SettlementEngine {
    #[wasm_bindgen(constructor)]
    pub fn new(settle_threshold: f64, blend_factor: f64) -> SettlementEngine {
        SettlementEngine {
            settle_threshold,
            blend_factor,
        }
    }
    
    /// Settle entire grid (full pass)
    #[wasm_bindgen]
    pub fn settle_full(&self, kernel: &mut SandKernel) {
        kernel.settle_internal(self.settle_threshold, self.blend_factor);
    }
    
    /// Settle localized region (optimized for dirty rectangles)
    #[wasm_bindgen]
    pub fn settle_region(
        &self,
        kernel: &mut SandKernel,
        min_x: usize,
        min_y: usize,
        max_x: usize,
        max_y: usize,
    ) {
        kernel.settle_region_internal(
            min_x,
            min_y,
            max_x,
            max_y,
            self.settle_threshold,
            self.blend_factor,
        );
    }    
    // Getters and setters for runtime adjustment
    #[wasm_bindgen]
    pub fn set_settle_threshold(&mut self, threshold: f64) {
        self.settle_threshold = threshold;
    }
    
    #[wasm_bindgen]
    pub fn get_settle_threshold(&self) -> f64 {
        self.settle_threshold
    }
    
    #[wasm_bindgen]
    pub fn set_blend_factor(&mut self, factor: f64) {
        self.blend_factor = factor;
    }
    
    #[wasm_bindgen]
    pub fn get_blend_factor(&self) -> f64 {
        self.blend_factor
    }}
