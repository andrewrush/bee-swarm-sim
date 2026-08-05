#include <iostream>
#include <fstream>
#include <vector>
#include <random>
#include <cmath>
#include <algorithm>
#include <iomanip>

using namespace std;

struct Vec2 {
    double x = 0.0;
    double y = 0.0;
};

double distance(Vec2 a, Vec2 b) {
    return hypot(a.x - b.x, a.y - b.y);
}

Vec2 moveTowards(Vec2 from, Vec2 to, double step) {
    double d = distance(from, to);
    if (d <= step) return to;
    Vec2 result;
    result.x = from.x + (to.x - from.x) / d * step;
    result.y = from.y + (to.y - from.y) / d * step;
    return result;
}

enum class Role { Scout, Worker };
enum class State { Resting, Exploring, Foraging, Returning, Dancing };

string roleToString(Role r) {
    return r == Role::Scout ? "scout" : "worker";
}

string stateToString(State s) {
    switch (s) {
        case State::Resting: return "resting";
        case State::Exploring: return "exploring";
        case State::Foraging: return "foraging";
        case State::Returning: return "returning";
        case State::Dancing: return "dancing";
    }
    return "unknown";
}

struct FlowerPatch {
    int id;
    Vec2 pos;
    double nectar;
    double initialNectar;
    bool discovered = false;
};

struct Bee {
    int id;
    Role role;
    State state = State::Resting;
    Vec2 pos;
    double energy = 100.0;
    double carrying = 0.0;
    int targetPatch = -1;
    double danceTimer = 0.0;
    bool foundNewPatch = false;
    int tripsCompleted = 0;
    double totalNectarDelivered = 0.0;
    double scoutDirX = 0.0;
    double scoutDirY = 0.0;
};

mt19937 rng(42);

void regeneratePatch(FlowerPatch &patch, double minDist, double maxDist) {
    double angle = uniform_real_distribution<double>(0.0, 2.0 * M_PI)(rng);
    double dist = minDist + uniform_real_distribution<double>(0.0, maxDist - minDist)(rng);
    patch.pos = {cos(angle) * dist, sin(angle) * dist};
    uniform_real_distribution<double> nectarDist(200.0, 800.0);
    patch.nectar = nectarDist(rng);
    patch.initialNectar = patch.nectar;
    patch.discovered = false;
}

int main() {
    const int NUM_BEES = 120;
    const int NUM_SCOUTS = 15;
    const int NUM_PATCHES = 10;
    const int TICKS = 300;
    const int TRACE_INTERVAL = 5;

    const double SPEED = 1.0;
    const double SCOUT_SPEED = 2.5;
    const double ENERGY_COST_MOVE = 0.15;
    const double ENERGY_RECOVER = 2.0;
    const double CARRY_CAPACITY = 10.0;
    const double DETECT_RADIUS = 12.0;
    const double PATCH_MIN_DIST = 30.0;
    const double PATCH_MAX_DIST = 80.0;

    const Vec2 hive{0.0, 0.0};
    uniform_real_distribution<double> nectarDist(200.0, 800.0);

    vector<FlowerPatch> patches;
    for (int i = 0; i < NUM_PATCHES; ++i) {
        FlowerPatch patch;
        patch.id = i;
        regeneratePatch(patch, PATCH_MIN_DIST, PATCH_MAX_DIST);
        patches.push_back(patch);
    }

    vector<Bee> bees;
    for (int i = 0; i < NUM_BEES; ++i) {
        Bee bee;
        bee.id = i;
        bee.pos = hive;
        bee.role = (i < NUM_SCOUTS) ? Role::Scout : Role::Worker;
        bees.push_back(bee);
    }

    vector<double> danceScore(NUM_PATCHES, 0.0);
    double hiveNectar = 0.0;
    double energySpent = 0.0;
    int totalTrips = 0;

    ofstream traceFile("trace.json");
    traceFile << "[\n";
    bool firstTrace = true;

    for (int tick = 0; tick < TICKS; ++tick) {
        // Dance decay
        for (double &score : danceScore) score *= 0.95;

        // Regenerate depleted patches individually (like snake apple)
        for (FlowerPatch &patch : patches) {
            if (patch.nectar <= 0.0) {
                regeneratePatch(patch, PATCH_MIN_DIST, PATCH_MAX_DIST);
            }
        }

        for (Bee &bee : bees) {
            double energyBefore = bee.energy;

            switch (bee.state) {
                case State::Resting: {
                    bee.energy = min(100.0, bee.energy + ENERGY_RECOVER);
                    if (bee.energy > 80.0) {
                        if (bee.role == Role::Scout) {
                            bee.state = State::Exploring;
                            bee.pos = hive;
                            bee.foundNewPatch = false;
                            bee.targetPatch = -1;
                            double angle = uniform_real_distribution<double>(0.0, 2.0 * M_PI)(rng);
                            bee.scoutDirX = cos(angle);
                            bee.scoutDirY = sin(angle);
                        } else {
                            double totalScore = 0.0;
                            for (double score : danceScore) totalScore += score;
                            if (totalScore > 0.1) {
                                uniform_real_distribution<double> pick(0.0, totalScore);
                                double r = pick(rng);
                                double acc = 0.0;
                                int chosen = -1;
                                for (int i = 0; i < NUM_PATCHES; ++i) {
                                    acc += danceScore[i];
                                    if (r <= acc) { chosen = i; break; }
                                }
                                if (chosen >= 0 && patches[chosen].nectar > 0.0) {
                                    bee.targetPatch = chosen;
                                    bee.state = State::Foraging;
                                    bee.pos = hive;
                                }
                            }
                        }
                    }
                    break;
                }

                case State::Exploring: {
                    normal_distribution<double> noise(0.0, 0.3);
                    bee.pos.x += (bee.scoutDirX + noise(rng)) * SCOUT_SPEED;
                    bee.pos.y += (bee.scoutDirY + noise(rng)) * SCOUT_SPEED;
                    bee.energy -= ENERGY_COST_MOVE * (SCOUT_SPEED / SPEED);

                    for (FlowerPatch &patch : patches) {
                        if (patch.nectar > 0.0 && distance(bee.pos, patch.pos) < DETECT_RADIUS) {
                            bee.targetPatch = patch.id;
                            patch.discovered = true;
                            bee.foundNewPatch = true;
                            bee.state = State::Returning;
                            break;
                        }
                    }

                    if (bee.energy < 20.0) {
                        bee.state = State::Returning;
                        bee.foundNewPatch = false;
                        bee.targetPatch = -1;
                    }
                    break;
                }

                case State::Foraging: {
                    if (bee.targetPatch < 0 || patches[bee.targetPatch].nectar <= 0.0) {
                        bee.state = State::Returning;
                        break;
                    }
                    bee.pos = moveTowards(bee.pos, patches[bee.targetPatch].pos, SPEED);
                    bee.energy -= ENERGY_COST_MOVE;

                    if (distance(bee.pos, patches[bee.targetPatch].pos) < 2.0) {
                        double take = min(CARRY_CAPACITY - bee.carrying, patches[bee.targetPatch].nectar);
                        take = min(take, 2.0);
                        patches[bee.targetPatch].nectar -= take;
                        bee.carrying += take;

                        if (bee.carrying >= CARRY_CAPACITY || patches[bee.targetPatch].nectar <= 0.0 || bee.energy < 25.0) {
                            bee.state = State::Returning;
                        }
                    }
                    if (bee.energy < 10.0) bee.state = State::Returning;
                    break;
                }

                case State::Returning: {
                    bee.pos = moveTowards(bee.pos, hive, SPEED);
                    bee.energy -= ENERGY_COST_MOVE;

                    if (distance(bee.pos, hive) < 2.0) {
                        hiveNectar += bee.carrying;
                        bee.totalNectarDelivered += bee.carrying;
                        bee.carrying = 0.0;
                        bee.tripsCompleted++;
                        totalTrips++;

                        if (bee.role == Role::Scout && bee.foundNewPatch && bee.targetPatch >= 0) {
                            bee.state = State::Dancing;
                            bee.danceTimer = 10.0;
                        } else {
                            bee.state = State::Resting;
                        }
                    }
                    break;
                }

                case State::Dancing: {
                    if (bee.targetPatch >= 0) {
                        FlowerPatch &patch = patches[bee.targetPatch];
                        double d = max(1.0, distance(hive, patch.pos));
                        double score = patch.nectar / (d + 1.0);
                        danceScore[bee.targetPatch] += score;
                    }
                    bee.danceTimer -= 1.0;
                    bee.energy -= 0.05;
                    if (bee.danceTimer <= 0.0) bee.state = State::Resting;
                    break;
                }
            }

            double spent = energyBefore - bee.energy;
            if (spent > 0.0) energySpent += spent;

            if (bee.energy <= 0.0) {
                bee.energy = 0.0;
                bee.carrying = 0.0;
                bee.state = State::Resting;
                bee.pos = hive;
            }
        }

        if (tick % 10 == 0) {
            int counts[5] = {0};
            for (Bee &bee : bees) {
                switch (bee.state) {
                    case State::Resting: counts[0]++; break;
                    case State::Exploring: counts[1]++; break;
                    case State::Foraging: counts[2]++; break;
                    case State::Returning: counts[3]++; break;
                    case State::Dancing: counts[4]++; break;
                }
            }
            int discovered = 0, depleted = 0;
            for (auto &p : patches) {
                if (p.discovered) discovered++;
                if (p.nectar <= 0.0) depleted++;
            }
            double efficiency = hiveNectar / max(1.0, energySpent);

            cout << "tick=" << tick
                 << " hive=" << fixed << setprecision(1) << hiveNectar
                 << " rest=" << counts[0]
                 << " explore=" << counts[1]
                 << " forage=" << counts[2]
                 << " return=" << counts[3]
                 << " dance=" << counts[4]
                 << " disc=" << discovered
                 << " depl=" << depleted
                 << " trips=" << totalTrips
                 << " eff=" << setprecision(3) << efficiency
                 << "\n";
        }

        if (tick % TRACE_INTERVAL == 0 || tick == TICKS - 1) {
            if (!firstTrace) traceFile << ",\n";
            firstTrace = false;

            traceFile << "  {\n";
            traceFile << "    \"tick\": " << tick << ",\n";
            traceFile << "    \"hive_nectar\": " << fixed << setprecision(2) << hiveNectar << ",\n";
            traceFile << "    \"energy_spent\": " << setprecision(2) << energySpent << ",\n";
            traceFile << "    \"total_trips\": " << totalTrips << ",\n";
            traceFile << "    \"efficiency\": " << setprecision(4) << (hiveNectar / max(1.0, energySpent)) << ",\n";

            traceFile << "    \"patches\": [\n";
            for (int i = 0; i < NUM_PATCHES; ++i) {
                traceFile << "      {\"id\": " << patches[i].id
                         << ", \"x\": " << setprecision(2) << patches[i].pos.x
                         << ", \"y\": " << setprecision(2) << patches[i].pos.y
                         << ", \"nectar\": " << setprecision(2) << patches[i].nectar
                         << ", \"discovered\": " << (patches[i].discovered ? "true" : "false")
                         << ", \"depleted\": " << (patches[i].nectar <= 0.0 ? "true" : "false") << "}";
                if (i < NUM_PATCHES - 1) traceFile << ",";
                traceFile << "\n";
            }
            traceFile << "    ],\n";

            traceFile << "    \"bees\": [\n";
            for (int i = 0; i < NUM_BEES; ++i) {
                traceFile << "      {\"id\": " << bees[i].id
                         << ", \"role\": \"" << roleToString(bees[i].role) << "\""
                         << ", \"state\": \"" << stateToString(bees[i].state) << "\""
                         << ", \"x\": " << setprecision(2) << bees[i].pos.x
                         << ", \"y\": " << setprecision(2) << bees[i].pos.y
                         << ", \"energy\": " << setprecision(1) << bees[i].energy
                         << ", \"carrying\": " << setprecision(1) << bees[i].carrying
                         << ", \"target\": " << bees[i].targetPatch << "}";
                if (i < NUM_BEES - 1) traceFile << ",";
                traceFile << "\n";
            }
            traceFile << "    ]\n";
            traceFile << "  }";
        }
    }

    traceFile << "\n]\n";
    traceFile.close();

    cout << "\n========================================\n";
    cout << "Final hive nectar: " << fixed << setprecision(1) << hiveNectar << "\n";
    cout << "Total energy spent: " << setprecision(1) << energySpent << "\n";
    cout << "Total trips: " << totalTrips << "\n";
    cout << "Efficiency: " << setprecision(4) << (hiveNectar / max(1.0, energySpent)) << " nectar/energy\n";
    cout << "Trace saved to trace.json\n";

    return 0;
}
