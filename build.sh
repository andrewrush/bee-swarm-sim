#!/data/data/com.termux/files/usr/bin/bash
set -e
echo "🔧 Building native bee_sim..."
clang++ -std=c++17 -O2 main.cpp -o bee_sim
echo "✅ Native build complete: ./bee_sim"
