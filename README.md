# 🐝 Bee Swarm Simulator

Bio-inspired multi-agent simulation of bee foraging behavior.

This project models a distributed colony of autonomous agents performing:

- Environmental exploration
- Resource gathering
- Task allocation through waggle dance communication
- Energy-aware scheduling
- Decentralized decision making

The simulator is written in C++ and designed to run in lightweight environments.

## 🚀 Quick Start

```bash
# Compile
clang++ -std=c++17 -O2 main.cpp -o bee_sim

# Run simulation
./bee_sim

# View web visualization
# Open https://andrewrush.github.io/bee-swarm-sim/ after pushing to GitHub Pages
```

## 📊 Live Demo

**[View Simulation Visualization](https://andrewrush.github.io/bee-swarm-sim/)**

## 🏗 Architecture

| Component | Description |
|-----------|-------------|
| `Hive` | Central resource accumulation base |
| `Scout` | Autonomous agents exploring for flower patches |
| `Worker` | Agents recruited via waggle dance to forage |
| `FlowerPatch` | Distributed resource sources |
| `WaggleDance` | Decentralized task allocation mechanism |

## 📈 Metrics

- Total nectar collected
- Energy efficiency (nectar per energy spent)
- Agent state distribution
- Patch discovery rate

## 📱 Mobile & Infinite Mode

- Responsive layout for portrait and landscape orientations
- Touch gestures: pan (1 finger), zoom (pinch), hover (tooltip)
- Infinite simulation mode: new flower patches regenerate automatically when depleted
- Sidebar collapses to drawer on mobile portrait

## 📝 License

MIT
