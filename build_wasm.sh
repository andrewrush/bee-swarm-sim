#!/data/data/com.termux/files/usr/bin/bash
# Build WebAssembly version using system emscripten
# Install: pkg install emscripten
# Then restart Termux session or run: emcc --clear-cache

set -e

if ! command -v emcc &> /dev/null; then
    echo "❌ emcc not found."
    echo "Install emscripten via pkg:"
    echo "   pkg install emscripten"
    echo "Then RESTART Termux session and run this script again."
    exit 1
fi

echo "🔧 Building WASM bee_sim..."

emcc wasm_api.cpp -o bee_sim.js \
    -s WASM=1 \
    -s EXPORTED_RUNTIME_METHODS='["ccall","cwrap","UTF8ToString"]' \
    -s EXPORTED_FUNCTIONS='["_sim_init","_sim_step","_sim_get_state_json","_sim_free_json"]' \
    -s ALLOW_MEMORY_GROWTH=1 \
    -s MODULARIZE=0 \
    -s SINGLE_FILE=0 \
    -O2 \
    -std=c++17

echo "✅ WASM build complete: bee_sim.js + bee_sim.wasm"
echo "📋 Add both files to git before pushing to GitHub Pages"
