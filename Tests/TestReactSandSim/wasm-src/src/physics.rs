use wasm_bindgen::prelude::*;
use crate::sand_kernel::SandKernel;

#[wasm_bindgen]
pub struct PhysicsEngine {
    current_pos_x: f64,
    current_pos_y: f64,
    prev_pos_x: f64,
    prev_pos_y: f64,
    target_pos_x: f64,
    target_pos_y: f64,
    ball_diameter: f64,
    move_speed: f64,
    // Physics parameters (configurable)
    trough_depth: f64,        // How deep the trough is (e.g., -1.8)
    trough_width_ratio: f64,  // Trough width as ratio of radius (e.g., 0.67)
    ridge_height: f64,        // Height of ridges (e.g., 1.2)
    ridge_offset: f64,        // Distance to push sand sideways (e.g., 1.2)
}

#[wasm_bindgen]
impl PhysicsEngine {
    #[wasm_bindgen(constructor)]
    pub fn new(
        table_size: f64,
        ball_diameter: f64,
        move_speed: f64,
        trough_depth: f64,
        trough_width_ratio: f64,
        ridge_height: f64,
        ridge_offset: f64,
    ) -> PhysicsEngine {
        let center = table_size / 2.0;
        
        PhysicsEngine {
            current_pos_x: center,
            current_pos_y: center,
            prev_pos_x: center,
            prev_pos_y: center,
            target_pos_x: center,
            target_pos_y: center,
            ball_diameter,
            move_speed,
            trough_depth,
            trough_width_ratio,
            ridge_height,
            ridge_offset,
        }
    }
    
    /// Set target position for ball to move toward
    #[wasm_bindgen]
    pub fn set_target(&mut self, x: f64, y: f64) {
        self.target_pos_x = x;
        self.target_pos_y = y;
    }
    
    /// Update ball position and displace sand
    #[wasm_bindgen]
    pub fn update(&mut self, kernel: &mut SandKernel) {
        // Store previous position for velocity calculation
        self.prev_pos_x = self.current_pos_x;
        self.prev_pos_y = self.current_pos_y;
        
        // Calculate direction to target
        let dx = self.target_pos_x - self.current_pos_x;
        let dy = self.target_pos_y - self.current_pos_y;
        let dist = (dx * dx + dy * dy).sqrt();
        
        // Move toward target with speed limiting
        if dist > self.move_speed {
            self.current_pos_x += (dx / dist) * self.move_speed;
            self.current_pos_y += (dy / dist) * self.move_speed;
        } else {
            self.current_pos_x = self.target_pos_x;
            self.current_pos_y = self.target_pos_y;
        }
        
        // Calculate velocity for displacement
        let vel_x = self.current_pos_x - self.prev_pos_x;
        let vel_y = self.current_pos_y - self.prev_pos_y;
        let vel_mag = (vel_x * vel_x + vel_y * vel_y).sqrt();
        
        // Only displace sand if ball actually moved
        if vel_mag < 0.001 {
            return;
        }
        
        // Normalized velocity and perpendicular direction
        let vel_norm_x = vel_x / vel_mag;
        let vel_norm_y = vel_y / vel_mag;
        let perp_x = -vel_norm_y;
        let perp_y = vel_norm_x;
        
        // Apply displacement based on distance traveled (physics-accurate)
        // Sample displacement at regular intervals along the path
        // This makes the simulation independent of move speed and frame rate
        let displacement_interval = 1.0; // Apply displacement every 1 pixel traveled
        let num_samples = (vel_mag / displacement_interval).ceil() as i32;
        
        if num_samples > 0 {
            // Interpolate along the path and apply displacement at each sample
            for i in 0..num_samples {
                let t = (i as f64 + 0.5) / num_samples as f64; // Sample at midpoint of each segment
                let sample_x = self.prev_pos_x + vel_x * t;
                let sample_y = self.prev_pos_y + vel_y * t;
                
                // Apply displacement at this sample point (scaled by distance per sample)
                let scale_factor = 1.0 / num_samples as f64;
                self.displace_sand_at(kernel, sample_x, sample_y, perp_x, perp_y, scale_factor);
            }
        }
    }
    
    /// Displace sand at a specific position with sub-pixel precision (private helper)
    fn displace_sand_at(
        &self,
        kernel: &mut SandKernel,
        center_x: f64,
        center_y: f64,
        perp_x: f64,
        perp_y: f64,
        scale_factor: f64,
    ) {
        let radius = self.ball_diameter / 2.0;
        let center_radius = radius * self.trough_width_ratio;
        let r_i32 = radius.ceil() as i32;
        
        // Iterate over circular region near ball with sub-pixel precision
        for dy in -r_i32..=r_i32 {
            for dx in -r_i32..=r_i32 {
                let d = ((dx * dx + dy * dy) as f64).sqrt();
                
                if d <= radius {
                    let pos_x = center_x + dx as f64;
                    let pos_y = center_y + dy as f64;
                    
                    if d < center_radius {
                        // Center zone: create trough
                        let trough_factor = 1.0 - (d / center_radius);
                        let sand_amount = self.trough_depth * trough_factor * scale_factor;
                        kernel.add_sand(pos_x, pos_y, sand_amount);
                    } else {
                        // Outer zone: push sand to sides (create ridges)
                        // Only apply perpendicular offset to positions that are already
                        // perpendicular to the motion direction (avoids interference on curves)
                        let side_factor = (d - center_radius) / (radius - center_radius);
                        let sand_amount = self.ridge_height * (1.0 - d / radius) * scale_factor;
                        
                        // Calculate how perpendicular this position is to motion
                        // (dot product with perpendicular direction)
                        let dx_f = dx as f64;
                        let dy_f = dy as f64;
                        let perp_alignment = (dx_f * perp_x + dy_f * perp_y).abs() / d.max(0.1);
                        
                        // Only push sand perpendicular if position is mostly perpendicular to motion
                        // This prevents interference patterns on curves
                        if perp_alignment > 0.5 {
                            let offset_dist = (1.0 - side_factor) * self.ridge_offset;
                            
                            // Add sand perpendicular to motion (both sides)
                            let disp_x1 = pos_x + perp_x * offset_dist;
                            let disp_y1 = pos_y + perp_y * offset_dist;
                            kernel.add_sand(disp_x1, disp_y1, sand_amount * perp_alignment);
                            
                            let disp_x2 = pos_x - perp_x * offset_dist;
                            let disp_y2 = pos_y - perp_y * offset_dist;
                            kernel.add_sand(disp_x2, disp_y2, sand_amount * perp_alignment * 0.8);
                        }
                    }
                }
            }
        }
    }
    
    /// Get current ball position (for rendering)
    #[wasm_bindgen]
    pub fn get_position(&self) -> Vec<f64> {
        vec![self.current_pos_x, self.current_pos_y]
    }
    
    /// Reset to center
    #[wasm_bindgen]
    pub fn reset(&mut self, table_size: f64) {
        let center = table_size / 2.0;
        self.current_pos_x = center;
        self.current_pos_y = center;
        self.prev_pos_x = center;
        self.prev_pos_y = center;
        self.target_pos_x = center;
        self.target_pos_y = center;
    }
    
    // Getters and setters for dynamic control
    #[wasm_bindgen]
    pub fn set_ball_diameter(&mut self, diameter: f64) {
        self.ball_diameter = diameter;
    }
    
    #[wasm_bindgen]
    pub fn get_ball_diameter(&self) -> f64 {
        self.ball_diameter
    }
    
    #[wasm_bindgen]
    pub fn set_move_speed(&mut self, speed: f64) {
        self.move_speed = speed;
    }
    
    #[wasm_bindgen]
    pub fn get_move_speed(&self) -> f64 {
        self.move_speed
    }
    
    // Physics parameter getters and setters
    #[wasm_bindgen]
    pub fn set_trough_depth(&mut self, depth: f64) {
        self.trough_depth = depth;
    }
    
    #[wasm_bindgen]
    pub fn get_trough_depth(&self) -> f64 {
        self.trough_depth
    }
    
    #[wasm_bindgen]
    pub fn set_trough_width_ratio(&mut self, ratio: f64) {
        self.trough_width_ratio = ratio;
    }
    
    #[wasm_bindgen]
    pub fn get_trough_width_ratio(&self) -> f64 {
        self.trough_width_ratio
    }
    
    #[wasm_bindgen]
    pub fn set_ridge_height(&mut self, height: f64) {
        self.ridge_height = height;
    }
    
    #[wasm_bindgen]
    pub fn get_ridge_height(&self) -> f64 {
        self.ridge_height
    }
    
    #[wasm_bindgen]
    pub fn set_ridge_offset(&mut self, offset: f64) {
        self.ridge_offset = offset;
    }
    
    #[wasm_bindgen]
    pub fn get_ridge_offset(&self) -> f64 {
        self.ridge_offset
    }
}
