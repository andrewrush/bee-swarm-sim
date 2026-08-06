#pragma once
#include <vector>
#include <random>
#include <cmath>
#include <algorithm>
#include <string>
#include <sstream>
#include <iomanip>

using namespace std;

struct Vec2 {
    double x = 0.0, y = 0.0;
};

inline double dist(Vec2 a, Vec2 b) {
    return hypot(a.x - b.x, a.y - b.y);
}

inline Vec2 moveTowards(Vec2 from, Vec2 to, double step) {
    double d = dist(from, to);
    if (d <= step) return to;
    return { from.x + (to.x - from.x) / d * step, from.y + (to.y - from.y) / d * step };
}

enum class Role { Scout, Worker };
enum class State { Resting, Exploring, Foraging, Returning, Dancing };

inline string roleStr(Role r) { return r == Role::Scout ? "scout" : "worker"; }
inline string stateStr(State s) {
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
    double scoutDirX = 0.0, scoutDirY = 0.0;
    vector<Vec2> trail;
    double prevX = 0, prevY = 0;
};

struct SimParams {
    int numBees = 120;
    int numScouts = 15;
    int numPatches = 10;
    int ticks = 500;
    int traceInterval = 5;
    double speed = 3.0;
    double scoutSpeed = 5.0;
    double energyCostMove = 0.12;
    double energyRecover = 2.0;
    double carryCapacity = 10.0;
    double detectRadius = 15.0;
    double patchMinDist = 60.0;
    double patchMaxDist = 150.0;
    double hiveUpkeepPerBee = 0.02;
    double starvingRecoverMult = 0.5;
    unsigned int seed = 42;
    std::string outFile = "trace.json";
};

class Simulation {
public:
    SimParams P;
    vector<FlowerPatch> patches;
    vector<Bee> bees;
    vector<double> danceScore;
    double hiveNectar = 800.0;
    double energySpent = 0.0;
    int totalTrips = 0;
    int tick = 0;
    mt19937 rng;
    Vec2 hive{0.0, 0.0};

    void init(const SimParams& params) {
        P = params;
        rng.seed(P.seed);
        patches.clear(); bees.clear();
        danceScore.assign(P.numPatches, 0.0);
        hiveNectar = 800.0;
        energySpent = 0.0;
        totalTrips = 0;
        tick = 0;

        for (int i = 0; i < P.numPatches; ++i) {
            FlowerPatch p; p.id = i;
            regeneratePatch(p);
            patches.push_back(p);
        }
        for (int i = 0; i < P.numBees; ++i) {
            Bee b;
            b.id = i;
            b.pos = hive;
            b.role = (i < P.numScouts) ? Role::Scout : Role::Worker;
            bees.push_back(b);
        }
    }

    void regeneratePatch(FlowerPatch& p) {
        uniform_real_distribution<double> angleDist(0.0, 2.0 * M_PI);
        uniform_real_distribution<double> distDist(P.patchMinDist, P.patchMaxDist);
        uniform_real_distribution<double> nectarDist(300.0, 800.0);
        double a = angleDist(rng);
        double d = distDist(rng);
        p.pos = { cos(a) * d, sin(a) * d };
        p.nectar = nectarDist(rng);
        p.initialNectar = p.nectar;
        p.discovered = false;
    }

    void step() {
        for (double& s : danceScore) s *= 0.95;

        double upkeep = min(hiveNectar, (double)P.numBees * P.hiveUpkeepPerBee);
        hiveNectar -= upkeep;
        bool hiveStarving = hiveNectar <= 0;

        for (FlowerPatch& patch : patches) {
            if (patch.nectar <= 0.0) {
                for (Bee& bee : bees) {
                    if (bee.targetPatch == patch.id) {
                        bee.targetPatch = -1;
                        bee.state = State::Returning;
                        bee.foundNewPatch = false;
                    }
                }
                danceScore[patch.id] = 0.0;
                regeneratePatch(patch);
            }
        }

        double totalDanceScore = 0.0;
        for (double s : danceScore) totalDanceScore += s;

        for (Bee& bee : bees) {
            double energyBefore = bee.energy;
            bee.prevX = bee.pos.x;
            bee.prevY = bee.pos.y;

            auto goal = evaluateGoal(bee, totalDanceScore, hiveStarving);
            executeGoal(bee, goal, totalDanceScore);

            switch (bee.state) {
                case State::Resting: {
                    double rr = hiveStarving ? P.energyRecover * P.starvingRecoverMult : P.energyRecover;
                    bee.energy = min(100.0, bee.energy + rr);
                    break;
                }
                case State::Exploring: {
                    normal_distribution<double> noise(0.0, 0.3);
                    bee.pos.x += (bee.scoutDirX + noise(rng)) * P.scoutSpeed;
                    bee.pos.y += (bee.scoutDirY + noise(rng)) * P.scoutSpeed;
                    bee.energy -= P.energyCostMove * (P.scoutSpeed / P.speed);
                    for (FlowerPatch& patch : patches) {
                        if (patch.nectar > 0.0 && dist(bee.pos, patch.pos) < P.detectRadius) {
                            bee.targetPatch = patch.id;
                            patch.discovered = true;
                            bee.foundNewPatch = true;
                            bee.state = State::Returning;
                            break;
                        }
                    }
                    if (bee.energy < 20.0 && bee.state == State::Exploring) {
                        bee.state = State::Returning;
                        bee.foundNewPatch = false;
                        bee.targetPatch = -1;
                    }
                    break;
                }
                case State::Foraging: {
                    if (bee.targetPatch < 0 || bee.targetPatch >= (int)patches.size() || patches[bee.targetPatch].nectar <= 0.0) {
                        bee.targetPatch = -1;
                        bee.state = State::Returning;
                        break;
                    }
                    bee.pos = moveTowards(bee.pos, patches[bee.targetPatch].pos, P.speed);
                    bee.energy -= P.energyCostMove;
                    if (dist(bee.pos, patches[bee.targetPatch].pos) < 2.0) {
                        double take = min(P.carryCapacity - bee.carrying, patches[bee.targetPatch].nectar);
                        take = min(take, 2.0);
                        patches[bee.targetPatch].nectar -= take;
                        bee.carrying += take;
                        if (bee.carrying >= P.carryCapacity || patches[bee.targetPatch].nectar <= 0.0 || bee.energy < 25.0)
                            bee.state = State::Returning;
                    }
                    if (bee.energy < 10.0) bee.state = State::Returning;
                    break;
                }
                case State::Returning: {
                    bee.pos = moveTowards(bee.pos, hive, P.speed);
                    bee.energy -= P.energyCostMove;
                    if (dist(bee.pos, hive) < 2.0) {
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
                    if (bee.targetPatch >= 0 && bee.targetPatch < (int)patches.size()) {
                        FlowerPatch& patch = patches[bee.targetPatch];
                        if (patch.nectar > 0) {
                            double d = max(1.0, dist(hive, patch.pos));
                            danceScore[bee.targetPatch] += patch.nectar / (d + 1.0);
                        }
                    }
                    bee.danceTimer -= 1.0;
                    bee.energy -= 0.05;
                    if (bee.danceTimer <= 0.0) bee.state = State::Resting;
                    break;
                }
            }

            if (bee.state != State::Resting && bee.state != State::Dancing) {
                bee.trail.push_back(bee.pos);
                if ((int)bee.trail.size() > 30) bee.trail.erase(bee.trail.begin());
            } else {
                bee.trail.clear();
            }

            double spent = energyBefore - bee.energy;
            if (spent > 0.0) energySpent += spent;
            if (bee.energy <= 0.0) {
                bee.energy = 0.0; bee.carrying = 0.0;
                bee.state = State::Resting;
                bee.pos = hive;
                bee.trail.clear();
            }
        }
        tick++;
    }

    string getJson() const {
        ostringstream oss;
        oss << fixed << setprecision(2);
        oss << "{\"tick\":" << tick
            << ",\"hive_nectar\":" << hiveNectar
            << ",\"energy_spent\":" << energySpent
            << ",\"total_trips\":" << totalTrips
            << ",\"efficiency\":" << setprecision(4) << (hiveNectar / max(1.0, energySpent))
            << ",\"avg_energy\":" << setprecision(2);
        double ae = 0;
        for (const auto& b : bees) ae += b.energy;
        oss << (ae / max(1, (int)bees.size()))
            << ",\"seed\":" << P.seed
            << ",\"patches\":[";
        for (size_t i = 0; i < patches.size(); ++i) {
            const auto& p = patches[i];
            oss << "{\"id\":" << p.id << ",\"x\":" << p.pos.x << ",\"y\":" << p.pos.y
                << ",\"nectar\":" << p.nectar << ",\"discovered\":" << (p.discovered ? "true" : "false")
                << ",\"depleted\":" << (p.nectar <= 0.0 ? "true" : "false") << "}";
            if (i + 1 < patches.size()) oss << ",";
        }
        oss << "],\"bees\":[";
        for (size_t i = 0; i < bees.size(); ++i) {
            const auto& b = bees[i];
            oss << "{\"id\":" << b.id << ",\"role\":\"" << roleStr(b.role) << "\""
                << ",\"state\":\"" << stateStr(b.state) << "\""
                << ",\"x\":" << b.pos.x << ",\"y\":" << b.pos.y
                << ",\"energy\":" << setprecision(1) << b.energy
                << ",\"carrying\":" << b.carrying
                << ",\"target\":" << b.targetPatch
                << ",\"tripsCompleted\":" << b.tripsCompleted
                << ",\"trail\":[";
            for (size_t t = 0; t < b.trail.size(); ++t) {
                oss << "{\"x\":" << b.trail[t].x << ",\"y\":" << b.trail[t].y << "}";
                if (t + 1 < b.trail.size()) oss << ",";
            }
            oss << "]}";
            if (i + 1 < bees.size()) oss << ",";
        }
        oss << "]}";
        return oss.str();
    }

private:
    struct Goal { int type; int priority; };
    Goal evaluateGoal(const Bee& bee, double totalDanceScore, bool hiveStarving) const {
        if (bee.energy < 15.0 && bee.state != State::Resting) return {0, 100}; // survive
        if (bee.carrying > 0) return {1, 90}; // deliver
        if (bee.state == State::Dancing) return {4, 10}; // rest
        if (bee.state == State::Resting) {
            double th = hiveStarving ? 25.0 : 80.0;
            if (bee.energy < th) return {4, 25};
        }
        if (hiveStarving && bee.energy > 30.0) {
            if (bee.role == Role::Scout) return {2, 55};
            if (totalDanceScore > 0.05) return {3, 60};
            return {2, 40};
        }
        if (bee.role == Role::Scout) return {2, 50};
        if (totalDanceScore > 0.1) return {3, 60};
        return {4, 20};
    }
    void executeGoal(Bee& bee, Goal g, double totalDanceScore) {
        switch (g.type) {
            case 0: // survive
                if (bee.state != State::Returning && bee.state != State::Resting)
                    bee.state = State::Returning;
                break;
            case 1: // deliver
                if (bee.state != State::Returning) bee.state = State::Returning;
                break;
            case 2: // explore
                if (bee.state == State::Resting) {
                    bee.state = State::Exploring;
                    bee.pos = hive;
                    bee.foundNewPatch = false;
                    bee.targetPatch = -1;
                    uniform_real_distribution<double> ad(0.0, 2.0 * M_PI);
                    double a = ad(rng);
                    bee.scoutDirX = cos(a); bee.scoutDirY = sin(a);
                }
                break;
            case 3: { // forage
                if (bee.state == State::Resting && totalDanceScore > 0.05) {
                    uniform_real_distribution<double> pick(0.0, totalDanceScore);
                    double r = pick(rng), acc = 0.0;
                    int chosen = -1;
                    for (int i = 0; i < (int)patches.size(); ++i) {
                        acc += danceScore[i];
                        if (r <= acc) { chosen = i; break; }
                    }
                    if (chosen >= 0 && patches[chosen].nectar > 0.0) {
                        bee.targetPatch = chosen;
                        bee.state = State::Foraging;
                        bee.pos = hive;
                    }
                }
                break;
            }
            case 4: break; // rest
        }
    }
};
