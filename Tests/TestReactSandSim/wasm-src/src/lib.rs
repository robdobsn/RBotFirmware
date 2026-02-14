mod sand_kernel;
mod physics;
mod settlement;

pub use sand_kernel::SandKernel;
pub use physics::PhysicsEngine;
pub use settlement::SettlementEngine;

use wasm_bindgen::prelude::*;

/// Initialize WASM module (called automatically by wasm-bindgen)
#[wasm_bindgen(start)]
pub fn init() {
    // Set up panic hook for better error messages
    #[cfg(feature = "console_error_panic_hook")]
    console_error_panic_hook::set_once();
}

/// Get WASM module version
#[wasm_bindgen]
pub fn version() -> String {
    env!("CARGO_PKG_VERSION").to_string()
}
