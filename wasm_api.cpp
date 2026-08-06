#include <emscripten.h>
#include <string>
#include <cstring>
#include "bee_swarm_sim.hpp"

static Simulation g_sim;
static std::string g_json;

extern "C" {

EMSCRIPTEN_KEEPALIVE
void sim_init(int bees, int scouts, int patches, int seed) {
    SimParams p;
    p.numBees = bees;
    p.numScouts = scouts;
    p.numPatches = patches;
    p.seed = (unsigned int)seed;
    g_sim.init(p);
}

EMSCRIPTEN_KEEPALIVE
void sim_step() {
    g_sim.step();
}

EMSCRIPTEN_KEEPALIVE
const char* sim_get_state_json() {
    g_json = g_sim.getJson();
    return g_json.c_str();
}

EMSCRIPTEN_KEEPALIVE
void sim_free_json(const char* ptr) {
    // no-op; memory managed by g_json
}

EMSCRIPTEN_KEEPALIVE
int sim_get_tick() { return g_sim.tick; }

EMSCRIPTEN_KEEPALIVE
double sim_get_hive_nectar() { return g_sim.hiveNectar; }

EMSCRIPTEN_KEEPALIVE
double sim_get_energy_spent() { return g_sim.energySpent; }

EMSCRIPTEN_KEEPALIVE
int sim_get_total_trips() { return g_sim.totalTrips; }

}
