#ifndef FUEL_INJECTOR_H
#define FUEL_INJECTOR_H

#include <cstdint>

constexpr int MAX_CHANNELS = 8;
constexpr int MAX_PPQN = 48;
constexpr int MAX_TICKS_PER_BAR = 336;

enum FuelInjectorState {
    LEARNING,
    LOCKED,
    INJECTING
};

enum ClockSource {
    CV,
    MIDI
};

enum InjectionType {
    MICROTIMING,
    OMISSION,
    ROLL,
    DENSITY,
    PERMUTATION,
    POLYRHYTHM,
    INJECTION_TYPE_COUNT
};

struct XorShift32 {
    uint32_t state;
    
    uint32_t next() {
        state ^= state << 13;
        state ^= state >> 17;
        state ^= state << 5;
        return state;
    }
};

struct ChannelPattern {
    uint16_t hit_positions[MAX_TICKS_PER_BAR];
    uint16_t hit_count_bar1;
    uint16_t hit_count_bar2;
    uint8_t timing_variance[MAX_TICKS_PER_BAR / 8];
    bool has_stable_pattern;
};

struct InjectionConfig {
    uint8_t probabilities[6];
};

struct _FuelInjector_DTC {
    uint32_t clock_tick_counter;
    uint32_t bar_counter;
    uint16_t current_bar_position;
    uint8_t trigger_active_steps_remaining[MAX_CHANNELS];
    float prev_clock_value;
    float prev_reset_value;
    XorShift32 prng;
    FuelInjectorState state;
    bool is_injection_bar;
};

struct _FuelInjectorAlgorithm {
    ChannelPattern patterns[MAX_CHANNELS];
    InjectionConfig injection_config;
    _FuelInjector_DTC* dtc;
    
    _FuelInjectorAlgorithm() : dtc(nullptr) {}
};

#endif
