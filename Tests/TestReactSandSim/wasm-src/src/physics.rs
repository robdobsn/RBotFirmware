use wasm_bindgen::prelude::*;
use crate::sand_kernel::SandKernel;

#[wasm_bindgen]
pub struct PhysicsEngine {
    current_pos_x: f32,
    current_pos_y: f32,
    prev_pos_x: f32,
    prev_pos_y: f32,
    target_pos_x: f32,
    target_pos_y: f32,
    ball_diameter: f32,
    move_speed: f32,
}

#[wasm_bindgen]
impl PhysicsEngine {
    #[wasm_bindgen(constructor)]
    pub fn new(
        table_size: f32,
        ball_diameter: f32,
        move_speed: f32,
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
        }
    }
    
    /// Set target position for ball to move toward
    #[wasm_bindgen]
    pub fn set_target(&mut self, x: f32, y: f32) {
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
        
        // Normalized velocity and perpendicular direction
        let (vel_norm_x, vel_norm_y) = if vel_mag > 0.001 {
            (vel_x / vel_mag, vel_y / vel_mag)
        } else {
            (0.0, 0.0)
        };
        
        let perp_x = -vel_norm_y;
        let perp_y = vel_norm_x;
        
        // Displace sand in circular region around ball
        self.displace_sand(kernel, perp_x, perp_y);
    }
    
    /// Displace sand around ball position (private helper)
    fn displace_sand(
        &self,
        kernel: &mut SandKernel,
        perp_x: f32,
        perp_y: f32,
    ) {
        let radius = self.ball_diameter / 2.0;
        let center_radius = radius * 0.67;  // Trough ~1/3 of ball diameter (~67% of radius)
        let r_i32 = radius.ceil() as i32;
        
        // Iterate over circular region near ball
        for dy in -r_i32..=r_i32 {
            for dx in -r_i32..=r_i32 {
                let d = ((dx * dx + dy * dy) as f32).sqrt();
                
                if d <= radius {
                    let pos_x = self.current_pos_x + dx as f32;
                    let pos_y = self.current_pos_y + dy as f32;
                    
                    if d < center_radius {
                        // Center zone: create trough (width ~1/3 of ball diameter)
                        let trough_factor = 1.0 - (d / center_radius);
                        let sand_amount = -1.8 * trough_factor;  // Remove sand progressively
                        kernel.add_sand(pos_x, pos_y, sand_amount);
                    } else {
                        // Outer zone: push sand to sides (create ridges)
                        // Keep ridges closer to trough for more concentrated effect
                        let side_factor = (d - center_radius) / (radius - center_radius);
                        let offset_dist = (1.0 - side_factor) * 1.2;  // Reduced from 2.0
                        let sand_amount = 1.2 * (1.0 - d / radius);
                        
                        // Add sand perpendicular to motion (both sides)
                        let disp_x1 = pos_x + perp_x * offset_dist;
                        let disp_y1 = pos_y + perp_y * offset_dist;
                        kernel.add_sand(disp_x1, disp_y1, sand_amount);
                        
                        let disp_x2 = pos_x - perp_x * offset_dist;
                        let disp_y2 = pos_y - perp_y * offset_dist;
                        kernel.add_sand(disp_x2, disp_y2, sand_amount * 0.8);
                    }
                }
            }
        }
    }
    
    /// Get current ball position (for rendering)
    #[wasm_bindgen]
    pub fn get_position(&self) -> Vec<f32> {
        vec![self.current_pos_x, self.current_pos_y]
    }
    
    /// Reset to center
    #[wasm_bindgen]
    pub fn reset(&mut self, table_size: f32) {
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
    pub fn set_ball_diameter(&mut self, diameter: f32) {
        self.ball_diameter = diameter;
    }
    
    #[wasm_bindgen]
    pub fn get_ball_diameter(&self) -> f32 {
        self.ball_diameter
    }
    
    #[wasm_bindgen]
    pub fn set_move_speed(&mut self, speed: f32) {
        self.move_speed = speed;
    }
    
    #[wasm_bindgen]
    pub fn get_move_speed(&self) -> f32 {
        self.move_speed
    }
}
