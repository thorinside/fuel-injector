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
    uint16_t hit_positions_bar1[MAX_TICKS_PER_BAR];
    uint16_t hit_positions_bar2[MAX_TICKS_PER_BAR];
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

struct PatternLearner {
    FuelInjectorState state;
    int stable_bars_count;
    int required_stable_bars;
};

inline void recordHit(ChannelPattern& pattern, int bar_index, int tick_position) {
    if (tick_position >= 0 && tick_position < MAX_TICKS_PER_BAR) {
        if (bar_index == 0) {
            pattern.hit_positions_bar1[tick_position] = 1;
            pattern.hit_count_bar1++;
        } else if (bar_index == 1) {
            pattern.hit_positions_bar2[tick_position] = 1;
            pattern.hit_count_bar2++;
        }
    }
}

inline float calculatePatternSimilarity(const ChannelPattern& pattern) {
    if (pattern.hit_count_bar1 == 0 && pattern.hit_count_bar2 == 0) {
        return 100.0f;
    }
    
    int matching_hits = 0;
    int total_hits = 0;
    
    for (int i = 0; i < MAX_TICKS_PER_BAR; i++) {
        bool bar1_hit = pattern.hit_positions_bar1[i] > 0;
        bool bar2_hit = pattern.hit_positions_bar2[i] > 0;
        
        if (bar1_hit && bar2_hit) {
            matching_hits++;
        }
        if (bar1_hit || bar2_hit) {
            total_hits++;
        }
    }
    
    if (total_hits == 0) {
        return 100.0f;
    }
    
    return (matching_hits * 100.0f) / total_hits;
}

inline void updateLearningState(PatternLearner& learner, float similarity) {
    const float SIMILARITY_THRESHOLD = 90.0f;
    
    if (similarity >= SIMILARITY_THRESHOLD) {
        learner.stable_bars_count++;
        if (learner.stable_bars_count >= learner.required_stable_bars) {
            learner.state = LOCKED;
        }
    } else {
        learner.stable_bars_count = 0;
        learner.state = LEARNING;
    }
}

inline void shiftBarsForNewBar(ChannelPattern& pattern) {
    for (int i = 0; i < MAX_TICKS_PER_BAR; i++) {
        pattern.hit_positions_bar1[i] = pattern.hit_positions_bar2[i];
        pattern.hit_positions_bar2[i] = 0;
    }
    pattern.hit_count_bar1 = pattern.hit_count_bar2;
    pattern.hit_count_bar2 = 0;
}

inline bool detectPatternChange(const ChannelPattern& learned, const ChannelPattern& incoming) {
    const float CHANGE_THRESHOLD = 90.0f;
    
    int matching_hits = 0;
    int total_hits = 0;
    
    for (int i = 0; i < MAX_TICKS_PER_BAR; i++) {
        bool learned_hit = learned.hit_positions_bar1[i] > 0;
        bool incoming_hit = incoming.hit_positions_bar1[i] > 0;
        
        if (learned_hit && incoming_hit) {
            matching_hits++;
        }
        if (learned_hit || incoming_hit) {
            total_hits++;
        }
    }
    
    if (total_hits == 0) {
        return false;
    }
    
    float similarity = (matching_hits * 100.0f) / total_hits;
    return similarity < CHANGE_THRESHOLD;
}

inline void handlePatternChange(PatternLearner& learner) {
    learner.state = LEARNING;
    learner.stable_bars_count = 0;
}

inline int calculateMicrotimingRange(int ppqn) {
    return ppqn / 4;
}

inline int applyMicrotimingShift(int position, int shift, int adjacent_position) {
    int new_position = position + shift;
    
    if (new_position == adjacent_position) {
        if (shift > 0) {
            new_position = adjacent_position + 1;
        } else {
            new_position = adjacent_position - 1;
        }
    }
    
    if (new_position < 0) new_position = 0;
    if (new_position >= MAX_TICKS_PER_BAR) new_position = MAX_TICKS_PER_BAR - 1;
    
    return new_position;
}

inline bool shouldApplyInjection(uint8_t probability, uint8_t fuel, XorShift32& rng) {
    if (fuel == 0 || probability == 0) {
        return false;
    }
    
    int scaled_probability = (probability * fuel) / 100;
    int random_value = rng.next() % 100;
    
    return random_value < scaled_probability;
}

inline void selectHitsForOmission(ChannelPattern* pattern, uint8_t* omit_indices, uint8_t* omit_count, uint8_t fuel, XorShift32* rng, uint16_t pattern_length) {
    *omit_count = 0;
    
    uint8_t hit_count = 0;
    uint16_t hit_positions[MAX_TICKS_PER_BAR];
    uint16_t non_downbeat_positions[MAX_TICKS_PER_BAR];
    uint8_t non_downbeat_count = 0;
    
    for (int i = 0; i < pattern_length; i++) {
        if (pattern->hit_positions_bar1[i] > 0) {
            hit_positions[hit_count++] = i;
            if (i != 0) {
                non_downbeat_positions[non_downbeat_count++] = i;
            }
        }
    }
    
    if (hit_count == 0) {
        return;
    }
    
    uint8_t max_omissions = (hit_count + 3) / 4;
    if (max_omissions == 0) {
        return;
    }
    
    uint16_t* candidate_pool = non_downbeat_count > 0 ? non_downbeat_positions : hit_positions;
    uint8_t pool_size = non_downbeat_count > 0 ? non_downbeat_count : hit_count;
    
    for (uint8_t i = 0; i < max_omissions && i < pool_size; i++) {
        if (shouldApplyInjection(100, fuel, *rng)) {
            uint8_t random_index = rng->next() % pool_size;
            omit_indices[*omit_count] = candidate_pool[random_index];
            (*omit_count)++;
            
            for (uint8_t j = random_index; j < pool_size - 1; j++) {
                candidate_pool[j] = candidate_pool[j + 1];
            }
            pool_size--;
        }
    }
}

inline void applyOmissionInjection(bool* output_pattern, uint8_t* omit_indices, uint8_t omit_count) {
    for (uint8_t i = 0; i < omit_count; i++) {
        output_pattern[omit_indices[i]] = false;
    }
}

inline void selectHitsForRoll(ChannelPattern* pattern, uint8_t* roll_indices, uint8_t* roll_count, uint8_t* roll_subdivisions, uint8_t fuel, XorShift32* rng, uint16_t pattern_length) {
    *roll_count = 0;
    
    uint8_t hit_count = 0;
    uint16_t hit_positions[MAX_TICKS_PER_BAR];
    
    for (int i = 0; i < pattern_length; i++) {
        if (pattern->hit_positions_bar1[i] > 0) {
            hit_positions[hit_count++] = i;
        }
    }
    
    if (hit_count == 0) {
        return;
    }
    
    uint8_t subdivisions[] = {2, 3, 4};
    
    for (uint8_t i = 0; i < hit_count; i++) {
        if (shouldApplyInjection(100, fuel, *rng)) {
            roll_indices[*roll_count] = hit_positions[i];
            uint8_t subdiv_index = rng->next() % 3;
            roll_subdivisions[*roll_count] = subdivisions[subdiv_index];
            (*roll_count)++;
        }
    }
}

inline void applyRollInjection(bool* output_pattern, uint8_t* roll_indices, uint8_t roll_count, uint8_t* roll_subdivisions, uint16_t ppqn) {
    for (uint8_t i = 0; i < roll_count; i++) {
        uint16_t original_position = roll_indices[i];
        uint8_t subdivisions = roll_subdivisions[i];
        uint16_t spacing = ppqn / subdivisions;
        
        uint16_t beat_start = (original_position / ppqn) * ppqn;
        uint16_t beat_end = beat_start + ppqn;
        
        for (uint8_t j = 1; j < subdivisions; j++) {
            uint16_t new_position = original_position + (spacing * j);
            if (new_position < beat_end && new_position < MAX_TICKS_PER_BAR) {
                output_pattern[new_position] = true;
            }
        }
    }
}

#endif
