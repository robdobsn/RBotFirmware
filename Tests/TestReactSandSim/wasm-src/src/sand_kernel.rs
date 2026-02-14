use wasm_bindgen::prelude::*;

#[wasm_bindgen]
pub struct SandKernel {
    table_size: usize,
    sand_level: Vec<f32>,
    sand_start_level: f32,
    max_sand_level: f32,
}

#[wasm_bindgen]
impl SandKernel {
    #[wasm_bindgen(constructor)]
    pub fn new(
        table_size: usize,
        sand_start_level: f32,
        max_sand_level: f32,
    ) -> Result<SandKernel, JsValue> {
        // Enable better error messages in debug mode
        #[cfg(feature = "console_error_panic_hook")]
        console_error_panic_hook::set_once();
        
        let size = table_size * table_size;
        let sand_level = vec![sand_start_level; size];
        
        Ok(SandKernel {
            table_size,
            sand_level,
            sand_start_level,
            max_sand_level,
        })
    }
    
    /// Get sand level at specific coordinates
    #[wasm_bindgen]
    pub fn get_sand_level(&self, x: f32, y: f32) -> f32 {
        let ix = x.floor() as usize;
        let iy = y.floor() as usize;
        
        if ix >= self.table_size || iy >= self.table_size {
            return 0.0;
        }
        
        self.sand_level[iy * self.table_size + ix]
    }
    
    /// Add sand at specific coordinates (can be negative to remove)
    #[wasm_bindgen]
    pub fn add_sand(&mut self, x: f32, y: f32, amount: f32) {
        let ix = x.floor() as usize;
        let iy = y.floor() as usize;
        
        if ix >= self.table_size || iy >= self.table_size {
            return;
        }
        
        let idx = iy * self.table_size + ix;
        let new_level = self.sand_level[idx] + amount;
        self.sand_level[idx] = new_level.min(self.max_sand_level).max(0.0);
    }
    
    /// Reset grid to initial state
    #[wasm_bindgen]
    pub fn reset(&mut self) {
        self.sand_level.fill(self.sand_start_level);
    }
    
    /// Get table size
    #[wasm_bindgen(getter)]
    pub fn table_size(&self) -> usize {
        self.table_size
    }
    
    /// Get pointer to internal buffer for JavaScript access
    #[wasm_bindgen]
    pub fn get_buffer(&self) -> Vec<f32> {
        self.sand_level.clone()
    }
    
    /// Get buffer pointer for zero-copy access from JavaScript
    /// WARNING: The pointer is only valid until the next WASM call that might reallocate
    #[wasm_bindgen]
    pub fn get_buffer_ptr(&self) -> *const f32 {
        self.sand_level.as_ptr()
    }
    
    /// Get buffer length (number of float32 elements)
    #[wasm_bindgen]
    pub fn get_buffer_length(&self) -> usize {
        self.sand_level.len()
    }
    
    /// Internal method: settle the sand (called by SettlementEngine)
    pub(crate) fn settle_internal(&mut self, settle_threshold: f32, blend_factor: f32) {
        let current_levels = self.sand_level.clone();
        
        // Skip borders (y = 0, y = table_size-1)
        for y in 1..(self.table_size - 1) {
            for x in 1..(self.table_size - 1) {
                let idx = y * self.table_size + x;
                let level = current_levels[idx];
                
                // Sample 4-connected neighbors
                let north = current_levels[(y - 1) * self.table_size + x];
                let south = current_levels[(y + 1) * self.table_size + x];
                let west = current_levels[y * self.table_size + (x - 1)];
                let east = current_levels[y * self.table_size + (x + 1)];
                
                let avg_neighbor = (north + south + west + east) / 4.0;
                
                // Only smooth significant differences
                if (level - avg_neighbor).abs() > settle_threshold {
                    self.sand_level[idx] = 
                        level * (1.0 - blend_factor) + 
                        avg_neighbor * blend_factor;
                }
            }
        }
    }
    
    /// Internal method: settle localized region (called by SettlementEngine)
    pub(crate) fn settle_region_internal(
        &mut self,
        min_x: usize,
        min_y: usize,
        max_x: usize,
        max_y: usize,
        settle_threshold: f32,
        blend_factor: f32,
    ) {
        let current_levels = self.sand_level.clone();
        
        let min_x = min_x.max(1);
        let min_y = min_y.max(1);
        let max_x = max_x.min(self.table_size - 1);
        let max_y = max_y.min(self.table_size - 1);
        
        for y in min_y..max_y {
            for x in min_x..max_x {
                let idx = y * self.table_size + x;
                let level = current_levels[idx];
                
                let north = current_levels[(y - 1) * self.table_size + x];
                let south = current_levels[(y + 1) * self.table_size + x];
                let west = current_levels[y * self.table_size + (x - 1)];
                let east = current_levels[y * self.table_size + (x + 1)];
                
                let avg_neighbor = (north + south + west + east) / 4.0;
                
                if (level - avg_neighbor).abs() > settle_threshold {
                    self.sand_level[idx] = 
                        level * (1.0 - blend_factor) + 
                        avg_neighbor * blend_factor;
                }
            }
        }
    }
}

#[cfg(test)]
mod tests {
    use super::*;
    
    #[test]
    fn test_kernel_initialization() {
        let kernel = SandKernel::new(200, 5.0, 20.0).unwrap();
        assert_eq!(kernel.table_size, 200);
        assert_eq!(kernel.get_sand_level(100.0, 100.0), 5.0);
    }
    
    #[test]
    fn test_add_sand() {
        let mut kernel = SandKernel::new(200, 5.0, 20.0).unwrap();
        kernel.add_sand(100.0, 100.0, 2.0);
        assert_eq!(kernel.get_sand_level(100.0, 100.0), 7.0);
    }
    
    #[test]
    fn test_sand_clamping() {
        let mut kernel = SandKernel::new(200, 5.0, 20.0).unwrap();
        kernel.add_sand(100.0, 100.0, 100.0);
        assert_eq!(kernel.get_sand_level(100.0, 100.0), 20.0);
    }
    
    #[test]
    fn test_negative_sand() {
        let mut kernel = SandKernel::new(200, 5.0, 20.0).unwrap();
        kernel.add_sand(100.0, 100.0, -10.0);
        assert_eq!(kernel.get_sand_level(100.0, 100.0), 0.0);
    }
}
