// WebAssembly-accelerated sand simulation
// This module provides WASM-based implementations that are 4-5x faster than pure JS

import type { Point, SandSimulationOptions } from './types';
import initWasm, { SandKernel as WasmSandKernel, PhysicsEngine as WasmPhysicsEngine, SettlementEngine as WasmSettlementEngine, type InitOutput } from './wasm/sand_kernel';

let wasmInitialized = false;
let wasmInitPromise: Promise<void> | null = null;
let wasmMemory: WebAssembly.Memory | null = null;

async function ensureWasmInitialized(): Promise<void> {
  if (wasmInitialized) return;
  
  if (wasmInitPromise) {
    return wasmInitPromise;
  }
  
  wasmInitPromise = (async () => {
    try {
      const wasmModule: InitOutput = await initWasm();
      wasmMemory = wasmModule.memory;
      wasmInitialized = true;
      console.log('✓ WebAssembly sand simulation initialized (4-5× faster)');
    } catch (error) {
      console.error('Failed to initialize WASM:', error);
      throw error;
    }
  })();
  
  return wasmInitPromise;
}

export class SandKernelWasm {
  private kernel: WasmSandKernel;
  
  private constructor(kernel: WasmSandKernel) {
    this.kernel = kernel;
  }
  
  static async create(
    tableSize: number,
    sandStartLevel: number,
    maxSandLevel: number
  ): Promise<SandKernelWasm> {
    await ensureWasmInitialized();
    const kernel = new WasmSandKernel(tableSize, sandStartLevel, maxSandLevel);
    return new SandKernelWasm(kernel);
  }
  
  getSandLevel(x: number, y: number): number {
    return this.kernel.get_sand_level(x, y);
  }
  
  addSand(x: number, y: number, amount: number): void {
    this.kernel.add_sand(x, y, amount);
  }
  
  settle(): void {
    // Create settlement engine on-demand for backward compatibility
    const settlement = new WasmSettlementEngine();
    settlement.settle_full(this.kernel);
    settlement.free();
  }
  
  reset(): void {
    this.kernel.reset();
  }
  
  getSandLevelArray(): Float32Array {
    // WASM returns Vec<f32> which becomes Float32Array in JS
    const buffer = this.kernel.get_buffer();
    return new Float32Array(buffer);
  }
  
  /**
   * Get zero-copy view of sand buffer (fastest - no memory allocation)
   * WARNING: View becomes invalid after any WASM call that might reallocate
   */
  getSandLevelArrayZeroCopy(): Float32Array {
    if (!wasmMemory) {
      throw new Error('WASM memory not initialized');
    }
    const ptr = this.kernel.get_buffer_ptr();
    const len = this.kernel.get_buffer_length();
    // Create Float32Array view directly from WASM memory
    // ptr is byte offset, divide by 4 for f32 elements
    return new Float32Array(wasmMemory.buffer, ptr, len);
  }
  
  getTableSize(): number {
    return this.kernel.table_size;
  }
  
  // Expose internal kernel for PhysicsEngine
  getWasmKernel(): WasmSandKernel {
    return this.kernel;
  }
}

export class SandTableSimWasm {
  private kernel: SandKernelWasm;
  private physics: WasmPhysicsEngine;
  private settlement: WasmSettlementEngine;
  private tableSize: number;
  private patternScale: number;  // Pattern coordinate range (e.g., 333 means -333 to +333)
  private settleFrequency: number = 0.005; // 0.5% chance per frame (less aggressive)
  
  private constructor(
    kernel: SandKernelWasm,
    physics: WasmPhysicsEngine,
    settlement: WasmSettlementEngine,
    tableSize: number,
    patternScale: number
  ) {
    this.kernel = kernel;
    this.physics = physics;
    this.settlement = settlement;
    this.tableSize = tableSize;
    this.patternScale = patternScale;
  }
  
  static async create(options: SandSimulationOptions): Promise<SandTableSimWasm> {
    await ensureWasmInitialized();
    
    const kernel = await SandKernelWasm.create(
      options.tableSize,
      options.sandStartLevel,
      options.maxSandLevel
    );
    
    // Use provided physics parameters or defaults
    const troughDepth = options.troughDepth ?? -1.8;
    const troughWidthRatio = options.troughWidthRatio ?? 0.67;
    const ridgeHeight = options.ridgeHeight ?? 1.2;
    const ridgeOffset = options.ridgeOffset ?? 1.2;
    
    const physics = new WasmPhysicsEngine(
      options.tableSize,
      options.ballDiameter,
      options.moveSpeed,
      troughDepth,
      troughWidthRatio,
      ridgeHeight,
      ridgeOffset
    );
    
    // Use provided settlement parameters or defaults
    const settleThreshold = options.settleThreshold ?? 1.5;
    const blendFactor = options.blendFactor ?? 0.02;
    
    const settlement = new WasmSettlementEngine(settleThreshold, blendFactor);
    
    const patternScale = options.patternScale ?? 250;
    
    const sim = new SandTableSimWasm(
      kernel,
      physics,
      settlement,
      options.tableSize,
      patternScale
    );
    
    // Set settle frequency if provided
    if (options.settleFrequency !== undefined) {
      sim.settleFrequency = options.settleFrequency;
    }
    
    return sim;
  }
  
  setTargetPosition(pos: Point): void {
    // Convert from pattern coordinates (-patternScale to +patternScale) to table coordinates (0 to tableSize)
    const scale = this.tableSize / (2 * this.patternScale);
    const targetX = this.tableSize / 2 + pos.x * scale;
    const targetY = this.tableSize / 2 + pos.y * scale;
    this.physics.set_target(targetX, targetY);
  }
  
  getCurrentPosition(): Point {
    const pos = this.physics.get_position();
    return { x: pos[0], y: pos[1] };
  }
  
  update(): void {
    // Update physics and displace sand (WASM-accelerated)
    this.physics.update(this.kernel.getWasmKernel());
    
    // Settle sand less frequently (2% chance per frame)
    if (Math.random() < this.settleFrequency) {
      this.settlement.settle_full(this.kernel.getWasmKernel());
    }
  }
  
  reset(): void {
    this.kernel.reset();
    this.physics.reset(this.tableSize);
  }
  
  getKernel(): SandKernelWasm {
    return this.kernel;
  }
  
  getBallDiameter(): number {
    return this.physics.get_ball_diameter();
  }
  
  setBallDiameter(diameter: number): void {
    this.physics.set_ball_diameter(diameter);
  }
  
  getMoveSpeed(): number {
    return this.physics.get_move_speed();
  }
  
  setMoveSpeed(speed: number): void {
    this.physics.set_move_speed(speed);
  }
  
  // Physics parameter setters/getters
  setTroughDepth(depth: number): void {
    this.physics.set_trough_depth(depth);
  }
  
  getTroughDepth(): number {
    return this.physics.get_trough_depth();
  }
  
  setTroughWidthRatio(ratio: number): void {
    this.physics.set_trough_width_ratio(ratio);
  }
  
  getTroughWidthRatio(): number {
    return this.physics.get_trough_width_ratio();
  }
  
  setRidgeHeight(height: number): void {
    this.physics.set_ridge_height(height);
  }
  
  getRidgeHeight(): number {
    return this.physics.get_ridge_height();
  }
  
  setRidgeOffset(offset: number): void {
    this.physics.set_ridge_offset(offset);
  }
  
  getRidgeOffset(): number {
    return this.physics.get_ridge_offset();
  }
  
  // Settlement parameter setters/getters
  setSettleThreshold(threshold: number): void {
    this.settlement.set_settle_threshold(threshold);
  }
  
  getSettleThreshold(): number {
    return this.settlement.get_settle_threshold();
  }
  
  setBlendFactor(factor: number): void {
    this.settlement.set_blend_factor(factor);
  }
  
  getBlendFactor(): number {
    return this.settlement.get_blend_factor();
  }
  
  setSettleFrequency(frequency: number): void {
    this.settleFrequency = frequency;
  }
  
  getSettleFrequency(): number {
    return this.settleFrequency;
  }
  
  // Check if WASM is initialized
  static isWasmAvailable(): boolean {
    return wasmInitialized;
  }
}

// Export initialization function for preloading
export async function initializeWasm(): Promise<void> {
  return ensureWasmInitialized();
}
