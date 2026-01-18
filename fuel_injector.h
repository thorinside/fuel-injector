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

inline bool detectRisingEdge(float current, float previous, float threshold) {
    return current > threshold && previous <= threshold;
}

inline int incrementTick(int current_tick) {
    return current_tick + 1;
}

inline int calculateBarPosition(int tick_count, int ppqn, int bar_length_qn) {
    int ticks_per_bar = ppqn * bar_length_qn;
    return tick_count % ticks_per_bar;
}

inline int calculateBarNumber(int tick_count, int ppqn, int bar_length_qn) {
    int ticks_per_bar = ppqn * bar_length_qn;
    return tick_count / ticks_per_bar;
}

inline bool isClockTimeout(int samples_since_clock, int timeout_threshold) {
    return samples_since_clock >= timeout_threshold;
}

struct MidiClockState {
    int midi_tick_count;
    bool midi_running;
};

inline void handleMidiRealtime(uint8_t byte, MidiClockState& state) {
    switch (byte) {
        case 0xF8:
            state.midi_tick_count++;
            break;
        case 0xFA:
            state.midi_tick_count = 0;
            state.midi_running = true;
            break;
        case 0xFC:
            state.midi_running = false;
            break;
        case 0xFF:
            state.midi_tick_count = 0;
            break;
    }
}

inline int convertMidiTicksToInternal(int midi_ticks, int internal_ppqn) {
    return midi_ticks * (internal_ppqn / 24);
}

#endif
