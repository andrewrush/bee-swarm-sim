#include <iostream>
#include <fstream>
#include <cstring>
#include "bee_swarm_sim.hpp"

using namespace std;

void printUsage(const char* prog) {
    cout << "Usage: " << prog << " [options]\n"
         << "Options:\n"
         << "  --bees N          Total bees (default: 120)\n"
         << "  --scouts N        Number of scouts (default: 15)\n"
         << "  --patches N       Flower patches (default: 10)\n"
         << "  --ticks N         Simulation ticks (default: 500)\n"
         << "  --trace N         Trace interval (default: 5)\n"
         << "  --speed N         Bee speed (default: 3.0)\n"
         << "  --scout-speed N   Scout speed (default: 5.0)\n"
         << "  --upkeep N        Upkeep per bee per tick (default: 0.02)\n"
         << "  --seed N          Random seed (default: 42)\n"
         << "  --out FILE        Output trace file (default: trace.json)\n"
         << "  --help            Show this help\n";
}

SimParams parseArgs(int argc, char** argv) {
    SimParams p;
    for (int i = 1; i < argc; ++i) {
        string arg = argv[i];
        if (arg == "--help" || arg == "-h") { printUsage(argv[0]); exit(0); }
        else if (arg == "--bees" && i + 1 < argc) p.numBees = stoi(argv[++i]);
        else if (arg == "--scouts" && i + 1 < argc) p.numScouts = stoi(argv[++i]);
        else if (arg == "--patches" && i + 1 < argc) p.numPatches = stoi(argv[++i]);
        else if (arg == "--ticks" && i + 1 < argc) p.ticks = stoi(argv[++i]);
        else if (arg == "--trace" && i + 1 < argc) p.traceInterval = stoi(argv[++i]);
        else if (arg == "--speed" && i + 1 < argc) p.speed = stod(argv[++i]);
        else if (arg == "--scout-speed" && i + 1 < argc) p.scoutSpeed = stod(argv[++i]);
        else if (arg == "--upkeep" && i + 1 < argc) p.hiveUpkeepPerBee = stod(argv[++i]);
        else if (arg == "--seed" && i + 1 < argc) p.seed = stoul(argv[++i]);
        else if (arg == "--out" && i + 1 < argc) p.outFile = argv[++i];
    }
    return p;
}

int main(int argc, char** argv) {
    SimParams P = parseArgs(argc, argv);
    Simulation sim;
    sim.init(P);

    ofstream traceFile(P.outFile);
    traceFile << "[\n";
    bool firstTrace = true;

    for (int t = 0; t < P.ticks; ++t) {
        sim.step();

        if (t % 10 == 0) {
            int counts[5] = {0};
            for (const auto& b : sim.bees) {
                switch (b.state) {
                    case State::Resting: counts[0]++; break;
                    case State::Exploring: counts[1]++; break;
                    case State::Foraging: counts[2]++; break;
                    case State::Returning: counts[3]++; break;
                    case State::Dancing: counts[4]++; break;
                }
            }
            int disc = 0, depl = 0;
            for (const auto& p : sim.patches) {
                if (p.discovered) disc++;
                if (p.nectar <= 0.0) depl++;
            }
            cout << "tick=" << t
                 << " hive=" << fixed << setprecision(1) << sim.hiveNectar
                 << " rest=" << counts[0]
                 << " explore=" << counts[1]
                 << " forage=" << counts[2]
                 << " return=" << counts[3]
                 << " dance=" << counts[4]
                 << " disc=" << disc
                 << " depl=" << depl
                 << " trips=" << sim.totalTrips
                 << " eff=" << setprecision(3) << (sim.hiveNectar / max(1.0, sim.energySpent))
                 << "\n";
        }

        if (t % P.traceInterval == 0 || t == P.ticks - 1) {
            if (!firstTrace) traceFile << ",\n";
            firstTrace = false;
            traceFile << sim.getJson();
        }
    }

    traceFile << "\n]\n";
    traceFile.close();

    cout << "\n========================================\n";
    cout << "Final hive nectar: " << fixed << setprecision(1) << sim.hiveNectar << "\n";
    cout << "Total energy spent: " << setprecision(1) << sim.energySpent << "\n";
    cout << "Total trips: " << sim.totalTrips << "\n";
    cout << "Efficiency: " << setprecision(4) << (sim.hiveNectar / max(1.0, sim.energySpent)) << " nectar/energy\n";
    cout << "Trace saved to " << P.outFile << "\n";
    return 0;
}
