# 🐝 Bee Swarm Simulator

Bio-inspired multi-agent simulation of bee foraging behavior.

**[🌐 Live Demo → https://andrewrush.github.io/bee-swarm-sim/](https://andrewrush.github.io/bee-swarm-sim/)**

This project models a distributed colony of autonomous agents performing:

- Environmental exploration
- Resource gathering
- Task allocation through waggle dance communication
- Energy-aware scheduling
- Decentralized decision making

The simulator is written in C++ and designed to run in lightweight environments (Termux on Android).

---

## 🚀 Quick Start

### Compile & Run (Termux)
```bash
# Standard build
clang++ -std=c++17 -O2 main.cpp -o bee_sim

# Or use the build script
chmod +x build.sh
./build.sh

# Run with default parameters
./bee_sim

# Run with custom parameters
./bee_sim --bees 200 --scouts 30 --patches 15 --ticks 1000 --seed 123 --upkeep 0.01
```

### Build WebAssembly (optional)
```bash
chmod +x build_wasm.sh
./build_wasm.sh
```
This generates `bee_sim.js` + `bee_sim.wasm`. Add both to git before pushing to GitHub Pages.

### View Web Visualization
Open **[https://andrewrush.github.io/bee-swarm-sim/](https://andrewrush.github.io/bee-swarm-sim/)** in your browser.

The web visualizer supports:
- **Built-in JS simulation** — runs directly in browser (fallback)
- **WASM engine** — if `bee_sim.wasm` is deployed, uses compiled C++ core
- **Load trace.json** — load output from C++ simulation
- **Touch controls** — pinch-to-zoom, pan, tap for tooltips
- **Live charts** — nectar, efficiency, state distribution
- **Trail visualization** — see where bees have flown
- **Velocity vectors** — show direction of movement
- **Auto-camera** — camera follows the swarm center

---

## 📊 Live Demo

**[🌐 Open GitHub Pages → https://andrewrush.github.io/bee-swarm-sim/](https://andrewrush.github.io/bee-swarm-sim/)**

---

## 🏗 Architecture

| File | Description |
|------|-------------|
| `bee_swarm_sim.hpp` | Core simulation engine (header-only) |
| `main.cpp` | CLI executable |
| `wasm_api.cpp` | Emscripten bridge for browser |
| `index.html` | Web visualizer with WASM/JS fallback |
| `build.sh` | Native build script |
| `build_wasm.sh` | WASM build script |

| Component | Description |
|-----------|-------------|
| `Hive` | Central resource accumulation base |
| `Scout` | Autonomous agents exploring for flower patches |
| `Worker` | Agents recruited via waggle dance to forage |
| `FlowerPatch` | Distributed resource sources |
| `WaggleDance` | Decentralized task allocation mechanism |

---

## ⚙️ CLI Parameters

```
./bee_sim [options]

Options:
  --bees N          Total bees (default: 120)
  --scouts N        Number of scouts (default: 15)
  --patches N       Flower patches (default: 10)
  --ticks N         Simulation ticks (default: 500)
  --trace N         Trace interval (default: 5)
  --speed N         Bee speed (default: 3.0)
  --scout-speed N   Scout speed (default: 5.0)
  --upkeep N        Upkeep per bee per tick (default: 0.02)
  --seed N          Random seed (default: 42)
  --out FILE        Output trace file (default: trace.json)
  --help            Show help
```

---

## 📈 Metrics

- Total nectar collected
- Energy efficiency (nectar per energy spent)
- Agent state distribution
- Patch discovery rate
- Average agent energy
- Flight trails (visualization)

---

## 📝 License

MIT
