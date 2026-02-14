# Sand Table Simulation - React TypeScript

A modern React + TypeScript implementation of a sand table simulation with parametric pattern generation, inspired by Sisyphus tables.

## Features

- **10 Parametric Patterns**: Archimedean spiral, logarithmic spiral, Fermat's spiral, rose curves, Lissajous curves, epitrochoid, hypotrochoid, star patterns, expanding circles, and 3D spiral
- **Custom Color Sampling**: Upload images to sample colors for the sand visualization
- **Real-time Physics**: Sand accumulation and settling simulation
- **Modular Components**: Separated canvas, pattern selector, and color loader components for easy reuse

## Project Structure

```
src/
  ├── App.tsx                  # Main demo application
  ├── SandTableCanvas.tsx      # Core simulation canvas component
  ├── PatternSelector.tsx      # Pattern selection dropdown
  ├── ColorImageLoader.tsx     # Image upload for color sampling
  ├── sandSimulation.ts        # Sand physics and simulation logic
  ├── patternUtils.ts          # Pattern generation algorithms
  ├── types.ts                 # TypeScript type definitions
  └── main.tsx                 # React entry point
```

## Installation

```bash
npm install
```

## Development

```bash
npm run dev
```

Opens the development server at `http://localhost:5173`

## Building for Production

```bash
npm run build
```

The built files will be in the `dist/` directory.

## Using Components in Another Project

### SandTableCanvas Component

The main simulation component:

```tsx
import { SandTableCanvas } from './SandTableCanvas';
import { PatternType } from './types';

function MyComponent() {
  const [pattern, setPattern] = useState<PatternType>('spiral');
  const [colorImage, setColorImage] = useState<HTMLImageElement | null>(null);

  return (
    <SandTableCanvas
      pattern={pattern}
      colorImage={colorImage}
      width={800}
      height={800}
      maxRadius={250}
      options={{
        ballDiameter: 10,
        tableSize: 200,
        sandStartLevel: 5,
        maxSandLevel: 20,
      }}
    />
  );
}
```

### PatternSelector Component

Dropdown for pattern selection:

```tsx
import { PatternSelector } from './PatternSelector';

function MyComponent() {
  const [pattern, setPattern] = useState<PatternType>('spiral');

  return (
    <PatternSelector
      selectedPattern={pattern}
      onPatternChange={setPattern}
    />
  );
}
```

### ColorImageLoader Component

Image upload for color sampling:

```tsx
import { ColorImageLoader } from './ColorImageLoader';

function MyComponent() {
  const [colorImage, setColorImage] = useState<HTMLImageElement | null>(null);

  return (
    <ColorImageLoader
      onImageLoaded={setColorImage}
    />
  );
}
```

## Pattern Types

- `spiral` - Archimedean Spiral
- `logSpiral` - Logarithmic Spiral
- `fermatSpiral` - Fermat's Spiral
- `rose` - Rose Curve
- `lissajous` - Lissajous Curve
- `epitrochoid` - Epitrochoid
- `hypotrochoid` - Hypotrochoid
- `starfield` - Star Pattern
- `circles` - Expanding Circles
- `spiral3d` - 3D Spiral

## Technology Stack

- React 18.3.1
- TypeScript 5.9.3
- Vite 7.3.1
- HTML5 Canvas API

## License

See LICENSE file in the repository root.
