#include "distingnt/api.h"
#include <cstring>
#include <new>
#include "fuel_injector.h"

// Define GUID for this algorithm
#define FUEL_INJECTOR_GUID NT_MULTICHAR('F', 'u', 'I', 'n')

// Specifications
enum { kSpecChannels };

static const _NT_specification specifications[] = {
    { .name = "Channels", .min = 1, .max = MAX_CHANNELS, .def = 4, .type = kNT_typeGeneric },
};

// Parameter indices for shared parameters
enum {
    kParamFuel,
    kParamPPQN,
    kParamBarLength,
    kParamInjectionInterval,
    kParamLearningBars,
    kParamProbMicrotiming,
    kParamProbOmission,
    kParamProbRoll,
    kParamProbDensity,
    kParamProbPermutation,
    kParamProbPolyrhythm,
    kParamClockSource,
    kParamClockInput,
    kParamResetInput,
    kNumSharedParams = 14
};

// Per-channel parameter offsets
enum {
    kChannelParamTrigIn = 0,
    kChannelParamTrigOut = 1,
    kChannelParamTrigOutMode = 2,
    kParamsPerChannel = 3
};

static const char* clockSourceStrings[] = { "CV", "MIDI", NULL };
static const char* ppqnStrings[] = { "1", "2", "4", "8", "16", "24", "48", NULL };
static const int ppqnValues[] = { 1, 2, 4, 8, 16, 24, 48 };

// Shared parameters (14 params: indices 0-13)
static const _NT_parameter sharedParameters[] = {
    { .name = "Fuel", .min = 0, .max = 100, .def = 100, .unit = kNT_unitPercent, .scaling = 0, .enumStrings = NULL },
    { .name = "PPQN", .min = 0, .max = 6, .def = 6, .unit = kNT_unitEnum, .scaling = 0, .enumStrings = ppqnStrings },
    { .name = "Bar Length", .min = 1, .max = 8, .def = 4, .unit = kNT_unitNone, .scaling = 0, .enumStrings = NULL },
    { .name = "Inj Interval", .min = 1, .max = 16, .def = 4, .unit = kNT_unitNone, .scaling = 0, .enumStrings = NULL },
    { .name = "Learn Bars", .min = 1, .max = 8, .def = 2, .unit = kNT_unitNone, .scaling = 0, .enumStrings = NULL },
    { .name = "P:Microtiming", .min = 0, .max = 100, .def = 50, .unit = kNT_unitPercent, .scaling = 0, .enumStrings = NULL },
    { .name = "P:Omission", .min = 0, .max = 100, .def = 30, .unit = kNT_unitPercent, .scaling = 0, .enumStrings = NULL },
    { .name = "P:Roll", .min = 0, .max = 100, .def = 40, .unit = kNT_unitPercent, .scaling = 0, .enumStrings = NULL },
    { .name = "P:Density", .min = 0, .max = 100, .def = 35, .unit = kNT_unitPercent, .scaling = 0, .enumStrings = NULL },
    { .name = "P:Permutation", .min = 0, .max = 100, .def = 25, .unit = kNT_unitPercent, .scaling = 0, .enumStrings = NULL },
    { .name = "P:Polyrhythm", .min = 0, .max = 100, .def = 20, .unit = kNT_unitPercent, .scaling = 0, .enumStrings = NULL },
    { .name = "Clock Source", .min = 0, .max = 1, .def = 0, .unit = kNT_unitEnum, .scaling = 0, .enumStrings = clockSourceStrings },
    NT_PARAMETER_CV_INPUT("Clock Input", 0, 1)
    NT_PARAMETER_CV_INPUT("Reset Input", 0, 2)
};

// Channel parameter template (3 params per channel)
static const _NT_parameter channelParamTemplate[] = {
    NT_PARAMETER_CV_INPUT("Trig In", 0, 3)
    NT_PARAMETER_CV_OUTPUT_WITH_MODE("Trig Out", 0, 15)
};

// Static requirements (shared memory)
static void fuel_injector_calculate_static_requirements(_NT_staticRequirements& req) {
    req.dram = 0;
}

// Initialize shared data
static void fuel_injector_initialise(_NT_staticMemoryPtrs& ptrs, const _NT_staticRequirements& req) {
    // No shared initialization needed
}

// Calculate per-instance requirements
static void fuel_injector_calculate_requirements(_NT_algorithmRequirements& req, const int32_t* specifications) {
    // Get channel count from specification, defaulting to 4
    int numChannels = specifications ? specifications[kSpecChannels] : 4;
    
    // Clamp to valid range [1, MAX_CHANNELS]
    if (numChannels < 1) numChannels = 1;
    if (numChannels > MAX_CHANNELS) numChannels = MAX_CHANNELS;
    
    // Compute dynamic parameter count
    const int kNumSharedParams = 14;  // Fuel through Reset Input
    const int kParamsPerChannel = 3;  // Trig In, Trig Out, Trig Out Mode
    int numParams = kNumSharedParams + numChannels * kParamsPerChannel;
    
    req.numParameters = numParams;
    req.sram = sizeof(_FuelInjectorAlgorithm)
                + numParams * sizeof(_NT_parameter)
                + 2 * sizeof(_NT_parameterPage)
                + 11 * sizeof(uint8_t)  // control page indices
                + (3 + numChannels * 3) * sizeof(uint8_t)  // routing page indices
                + numChannels * 48;  // parameter name strings ("Trig N In", "Trig N Out", "Trig N Out mode")
    req.dram = numChannels * sizeof(ChannelPattern)
                + numChannels * MAX_TICKS_PER_BAR * sizeof(bool);
    req.dtc = sizeof(_FuelInjector_DTC);
    req.itc = 0;
}

// Construct algorithm instance
static _NT_algorithm* fuel_injector_construct(const _NT_algorithmMemoryPtrs& ptrs, const _NT_algorithmRequirements& req, const int32_t* specifications) {
    // Get channel count
    int numChannels = specifications ? specifications[kSpecChannels] : 4;
    if (numChannels < 1) numChannels = 1;
    if (numChannels > MAX_CHANNELS) numChannels = MAX_CHANNELS;
    
    const int kNumSharedParams = 14;
    const int kParamsPerChannel = 3;
    int numParams = kNumSharedParams + numChannels * kParamsPerChannel;
    
    // Use placement new to construct algorithm in provided SRAM
    _FuelInjectorAlgorithm* alg = new (ptrs.sram) _FuelInjectorAlgorithm();
    
    // Set up memory pointers in SRAM after the struct
    uint8_t* mem = (uint8_t*)ptrs.sram + sizeof(_FuelInjectorAlgorithm);
    
    alg->params = (_NT_parameter*)mem;
    mem += numParams * sizeof(_NT_parameter);
    alg->numParams = numParams;
    
    alg->pages = (_NT_parameterPage*)mem;
    mem += 2 * sizeof(_NT_parameterPage);
    alg->numPages = 2;
    
    alg->controlPageParams = mem;
    mem += 11 * sizeof(uint8_t);
    
    alg->routingPageParams = mem;
    mem += (3 + numChannels * 3) * sizeof(uint8_t);
    
    char* paramNames = (char*)mem;
    mem += numChannels * 48;
    
    alg->numChannels = numChannels;

    if (ptrs.dram) {
        uint8_t* dram = (uint8_t*)ptrs.dram;
        alg->learned_patterns = (ChannelPattern*)dram;
        dram += numChannels * sizeof(ChannelPattern);
        alg->output_patterns = (bool (*)[MAX_TICKS_PER_BAR])dram;

        memset(alg->learned_patterns, 0, numChannels * sizeof(ChannelPattern));
        memset(alg->output_patterns, 0, numChannels * MAX_TICKS_PER_BAR * sizeof(bool));
    }
    
    memcpy(alg->params, sharedParameters, sizeof(sharedParameters));
    
    for (int c = 0; c < numChannels; ++c) {
        int base = kNumSharedParams + c * kParamsPerChannel;
        memcpy(&alg->params[base], channelParamTemplate, sizeof(channelParamTemplate));
        
        char* inName = paramNames + c * 48;
        char* outName = paramNames + c * 48 + 12;
        char* modeName = paramNames + c * 48 + 24;
        
        int len = 0;
        inName[len++] = 'T'; inName[len++] = 'r'; inName[len++] = 'i'; inName[len++] = 'g'; inName[len++] = ' ';
        inName[len++] = '1' + c;
        inName[len++] = ' '; inName[len++] = 'I'; inName[len++] = 'n'; inName[len] = '\0';
        
        len = 0;
        outName[len++] = 'T'; outName[len++] = 'r'; outName[len++] = 'i'; outName[len++] = 'g'; outName[len++] = ' ';
        outName[len++] = '1' + c;
        outName[len++] = ' '; outName[len++] = 'O'; outName[len++] = 'u'; outName[len++] = 't'; outName[len] = '\0';
        
        len = 0;
        modeName[len++] = 'T'; modeName[len++] = 'r'; modeName[len++] = 'i'; modeName[len++] = 'g'; modeName[len++] = ' ';
        modeName[len++] = '1' + c;
        modeName[len++] = ' '; modeName[len++] = 'O'; modeName[len++] = 'u'; modeName[len++] = 't';
        modeName[len++] = ' '; modeName[len++] = 'm'; modeName[len++] = 'o'; modeName[len++] = 'd'; modeName[len++] = 'e';
        modeName[len] = '\0';
        
        alg->params[base + 0].name = inName;
        alg->params[base + 1].name = outName;
        alg->params[base + 2].name = modeName;
        alg->params[base + 0].def = 3 + c;
        alg->params[base + 1].def = 15 + c;
    }
    
    // Build Control page indices
    for (int i = 0; i < 11; ++i) {
        alg->controlPageParams[i] = i;
    }
    
    // Build Routing page indices
    alg->routingPageParams[0] = 11;  // Clock Source
    alg->routingPageParams[1] = 12;  // Clock Input
    alg->routingPageParams[2] = 13;  // Reset Input
    for (int c = 0; c < numChannels; ++c) {
        int base = kNumSharedParams + c * kParamsPerChannel;
        alg->routingPageParams[3 + c * 3 + 0] = base + 0;  // Trig In
        alg->routingPageParams[3 + c * 3 + 1] = base + 1;  // Trig Out
        alg->routingPageParams[3 + c * 3 + 2] = base + 2;  // Trig Out Mode
    }
    
    // Set up pages
    alg->pages[0].name = "Control";
    alg->pages[0].numParams = 11;
    alg->pages[0].params = alg->controlPageParams;
    
    alg->pages[1].name = "Routing";
    alg->pages[1].numParams = 3 + numChannels * 3;
    alg->pages[1].params = alg->routingPageParams;
    
    // Set up parameter pages wrapper
    alg->paramPages.numPages = 2;
    alg->paramPages.pages = alg->pages;
    
    // Initialize inherited members
    alg->parameters = alg->params;
    alg->parameterPages = &alg->paramPages;
    
    // Initialize DTC (hot state)
    if (ptrs.dtc) {
        alg->dtc = (_FuelInjector_DTC*)ptrs.dtc;
        alg->dtc->state = LEARNING;
        alg->dtc->bar_counter = 0;
        alg->dtc->bars_since_lock = 0;
        alg->dtc->samples_since_clock = 0;
        alg->dtc->last_clock_period_samples = 0;
        alg->dtc->current_bar_position = 0;
        alg->dtc->clock_tick_counter = 0;
        alg->dtc->prev_clock_value = 0.0f;
        alg->dtc->prev_reset_value = 0.0f;
        alg->dtc->current_bar_index = 0;
        alg->dtc->is_injection_bar = false;
        alg->dtc->prng.state = 12345;
        alg->dtc->stable_bars_count = 0;
        alg->dtc->required_stable_bars = 2;
        for (int c = 0; c < MAX_CHANNELS; ++c) {
            alg->dtc->prev_trigger_value[c] = 0.0f;
            alg->dtc->trigger_active_steps_remaining[c] = 0;
        }
    }
    
    for (int c = 0; c < MAX_CHANNELS; ++c) {
        memset(&alg->patterns[c], 0, sizeof(ChannelPattern));
    }
    
    return reinterpret_cast<_NT_algorithm*>(alg);
}

static void fuel_injector_parameter_changed(_NT_algorithm* self_base, int p_idx) {
    _FuelInjectorAlgorithm* self = static_cast<_FuelInjectorAlgorithm*>(self_base);
    
    if (p_idx == kParamPPQN || p_idx == kParamBarLength) {
        self->dtc->state = LEARNING;
        self->dtc->bar_counter = 0;
        self->dtc->bars_since_lock = 0;
        self->dtc->samples_since_clock = 0;
        self->dtc->last_clock_period_samples = 0;
        self->dtc->current_bar_position = 0;
        self->dtc->clock_tick_counter = 0;
        self->dtc->current_bar_index = 0;
        self->dtc->stable_bars_count = 0;
        self->dtc->is_injection_bar = false;
        
        for (int c = 0; c < self->numChannels; ++c) {
            memset(&self->patterns[c], 0, sizeof(ChannelPattern));
            memset(&self->learned_patterns[c], 0, sizeof(ChannelPattern));
            self->dtc->trigger_active_steps_remaining[c] = 0;
        }
    }
}

// Main audio processing callback
static void fuel_injector_step(_NT_algorithm* self_base, float* busFrames, int numFramesBy4) {
    _FuelInjectorAlgorithm* self = static_cast<_FuelInjectorAlgorithm*>(self_base);
    
    int numFrames = numFramesBy4 * 4;
    int clockBus = self->v[kParamClockInput] - 1;
    int resetBus = self->v[kParamResetInput] - 1;
    int ppqn = ppqnValues[self->v[kParamPPQN]];
    int barLength = self->v[kParamBarLength];
    int maxBarLength = MAX_TICKS_PER_BAR / ppqn;
    if (barLength > maxBarLength) {
        barLength = maxBarLength;
    }
    if (barLength < 1) {
        barLength = 1;
    }
    int ticksPerBar = ppqn * barLength;
    int fuel = self->v[kParamFuel];
    int injectionInterval = self->v[kParamInjectionInterval];
    int learningBars = self->v[kParamLearningBars];
    
    // Output pulses in LOCKED/INJECTING are treated as triggers (~10ms max).
    // Passthrough (LEARNING) preserves incoming gate length by copying the input signal.
    int baseTriggerLengthSamples = (int)((10.0f / 1000.0f) * NT_globals.sampleRate);
    if (baseTriggerLengthSamples < 1) {
        baseTriggerLengthSamples = 1;
    }
    const float TRIGGER_THRESHOLD = 1.0f;
    const float TRIGGER_HIGH = 5.0f;
    
    // learningBars is the number of bars to observe; stability is checked across consecutive bar pairs.
    // e.g. learningBars=2 -> require 1 stable comparison (2 bars total).
    self->dtc->required_stable_bars = (learningBars > 1) ? (learningBars - 1) : 1;
    
    for (int frame = 0; frame < numFrames; frame++) {
        const bool clockEnabled = (self->v[kParamClockSource] == 0) && (clockBus >= 0);
        float clockValue = 0.0f;
        if (clockEnabled) {
            clockValue = busFrames[clockBus * numFrames + frame];
        }
        const bool clockEdge =
                clockEnabled && (clockValue >= TRIGGER_THRESHOLD) &&
                (self->dtc->prev_clock_value < TRIGGER_THRESHOLD);
        self->dtc->prev_clock_value = clockValue;

        if (clockEnabled) {
            self->dtc->samples_since_clock++;
            if (clockEdge) {
                self->dtc->last_clock_period_samples = self->dtc->samples_since_clock;
                self->dtc->samples_since_clock = 0;
            }
        } else {
            self->dtc->samples_since_clock = 0;
            self->dtc->last_clock_period_samples = 0;
        }

        float resetValue = 0.0f;
        if (resetBus >= 0) {
            resetValue = busFrames[resetBus * numFrames + frame];
        }
        const bool resetEdge =
                (resetBus >= 0) && (resetValue >= TRIGGER_THRESHOLD) &&
                (self->dtc->prev_reset_value < TRIGGER_THRESHOLD);
        self->dtc->prev_reset_value = resetValue;

        if (resetEdge) {
            self->dtc->state = LEARNING;
            self->dtc->bar_counter = 0;
            self->dtc->bars_since_lock = 0;
            self->dtc->samples_since_clock = 0;
            self->dtc->last_clock_period_samples = 0;
            self->dtc->current_bar_position = 0;
            self->dtc->clock_tick_counter = 0;
            self->dtc->current_bar_index = 0;
            self->dtc->stable_bars_count = 0;
            self->dtc->is_injection_bar = false;

            for (int c = 0; c < self->numChannels; ++c) {
                memset(&self->patterns[c], 0, sizeof(ChannelPattern));
                memset(&self->learned_patterns[c], 0, sizeof(ChannelPattern));
                self->dtc->trigger_active_steps_remaining[c] = 0;
            }
        }

        bool clockTick = false;
        int tickPos = static_cast<int>(self->dtc->current_bar_position);
        if (clockEdge) {
            clockTick = true;
            tickPos = static_cast<int>(self->dtc->clock_tick_counter);
            self->dtc->current_bar_position = static_cast<uint16_t>(tickPos);
        }
        
        // Process each channel's trigger input/output
        for (int c = 0; c < self->numChannels; ++c) {
            int base = kNumSharedParams + c * kParamsPerChannel;
            int trigInBus = self->v[base + kChannelParamTrigIn] - 1;
            int trigOutBus = self->v[base + kChannelParamTrigOut] - 1;
            bool replaceMode = self->v[base + kChannelParamTrigOutMode];
            
            float trigInValue = 0.0f;
            if (trigInBus >= 0) {
                trigInValue = busFrames[trigInBus * numFrames + frame];
            }
            float* trigOut =
                    (trigOutBus >= 0) ? &busFrames[trigOutBus * numFrames + frame] : nullptr;
            
            bool triggerDetected = (trigInValue >= TRIGGER_THRESHOLD && 
                                   self->dtc->prev_trigger_value[c] < TRIGGER_THRESHOLD);
            
            // Record hits relative to the most recent clock tick; do not require the trigger
            // to coincide sample-exactly with the clock edge.
            if (triggerDetected) {
                if (tickPos >= 0 && tickPos < ticksPerBar) {
                    ChannelPattern& p = self->patterns[c];
                    if (p.hit_positions_bar1[tickPos] == 0) {
                        recordHit(p, 0, tickPos);
                    }
                }
            }
            
            self->dtc->prev_trigger_value[c] = trigInValue;
            
            const bool playbackActive =
                    (fuel > 0) &&
                    (self->dtc->state != LEARNING) &&
                    clockEnabled &&
                    (self->learned_patterns != nullptr) &&
                    ((self->dtc->state != INJECTING) || (self->output_patterns != nullptr));

            if (playbackActive) {
                if (clockTick) {
                    bool hit = false;
                    if (tickPos >= 0 && tickPos < ticksPerBar) {
                        if (self->dtc->state == INJECTING) {
                            hit = (fuel > 0) && self->output_patterns[c][tickPos];
                        } else {
                            hit = self->learned_patterns[c].hit_positions_bar1[tickPos] > 0;
                        }
                    }
                    if (hit) {
                        int triggerLengthSamples = baseTriggerLengthSamples;
                        uint32_t clockPeriodSamples = self->dtc->last_clock_period_samples;
                        if (clockPeriodSamples > 0) {
                            uint32_t maxLen = clockPeriodSamples / 2;
                            if (maxLen < 1) {
                                maxLen = 1;
                            }
                            if ((uint32_t)triggerLengthSamples > maxLen) {
                                triggerLengthSamples = (int)maxLen;
                            }
                        }
                        self->dtc->trigger_active_steps_remaining[c] = (uint16_t)triggerLengthSamples;
                    }
                }

                bool shouldOutput = self->dtc->trigger_active_steps_remaining[c] > 0;
                if (shouldOutput) {
                    self->dtc->trigger_active_steps_remaining[c]--;
                }

                float outputValue = shouldOutput ? TRIGGER_HIGH : 0.0f;
                if (trigOut) {
                    if (replaceMode) {
                        *trigOut = outputValue;
                    } else {
                        *trigOut += outputValue;
                    }
                }
            } else {
                if (trigOut) {
                    if (!replaceMode && trigOutBus == trigInBus) {
                        // Avoid doubling when input and output share a bus in add mode.
                        *trigOut = trigInValue;
                    } else if (replaceMode) {
                        *trigOut = trigInValue;
                    } else {
                        *trigOut += trigInValue;
                    }
                }
            }
        }

        // Advance clock tick counter and handle end-of-bar transitions.
        bool barBoundary = false;
        FuelInjectorState endingState = self->dtc->state;
        if (clockTick) {
            self->dtc->clock_tick_counter++;
            if (self->dtc->clock_tick_counter >= (uint32_t)ticksPerBar) {
                self->dtc->clock_tick_counter = 0;
                self->dtc->bar_counter++;
                self->dtc->current_bar_position = 0;
                barBoundary = true;
            }
        }

        if (barBoundary) {
            // Learning: check whether the last two bars were similar enough to lock.
            if (endingState == LEARNING) {
                float minSimilarity = 100.0f;
                for (int c = 0; c < self->numChannels; ++c) {
                    float sim = calculatePatternSimilarity(self->patterns[c]);
                    if (sim < minSimilarity) minSimilarity = sim;
                }

                const float SIMILARITY_THRESHOLD = 90.0f;
                if (minSimilarity >= SIMILARITY_THRESHOLD) {
                    self->dtc->stable_bars_count++;
                    if (self->dtc->stable_bars_count >= self->dtc->required_stable_bars) {
                        self->dtc->state = LOCKED;
                        self->dtc->bars_since_lock = 0;
                        for (int c = 0; c < self->numChannels; ++c) {
                            // Snapshot the just-completed bar as the learned pattern.
                            memset(&self->learned_patterns[c], 0, sizeof(ChannelPattern));
                            memcpy(self->learned_patterns[c].hit_positions_bar1,
                                   self->patterns[c].hit_positions_bar1,
                                   sizeof(self->learned_patterns[c].hit_positions_bar1));
                            self->learned_patterns[c].hit_count_bar1 = self->patterns[c].hit_count_bar1;
                        }
                    }
                } else {
                    self->dtc->stable_bars_count = 0;
                }
            } else {
                // LOCKED/INJECTING: monitor for input pattern changes to trigger re-learning.
                bool patternChanged = false;
                for (int c = 0; c < self->numChannels; ++c) {
                    if (detectPatternChange(self->learned_patterns[c], self->patterns[c])) {
                        patternChanged = true;
                        break;
                    }
                }

                if (patternChanged) {
                    self->dtc->state = LEARNING;
                    self->dtc->stable_bars_count = 0;
                    self->dtc->bars_since_lock = 0;
                    self->dtc->is_injection_bar = false;
                    for (int c = 0; c < self->numChannels; ++c) {
                        self->dtc->trigger_active_steps_remaining[c] = 0;
                    }
                } else {
                    // Completed one bar while locked/injecting.
                    if (endingState == LOCKED || endingState == INJECTING) {
                        self->dtc->bars_since_lock++;
                    }

                    // Injection bar finished -> return to locked for the next bar.
                    if (endingState == INJECTING) {
                        self->dtc->state = LOCKED;
                        self->dtc->is_injection_bar = false;
                        for (int c = 0; c < self->numChannels; ++c) {
                            self->dtc->trigger_active_steps_remaining[c] = 0;
                        }
                    }

                    // Schedule an injection for the *next* bar.
                    // We use (bar_counter + 1) as the next bar number (1-indexed).
                    if (fuel > 0 &&
                        self->dtc->state == LOCKED &&
                        shouldInjectThisBar(self->dtc->bar_counter + 1, injectionInterval)) {
                        self->dtc->state = INJECTING;
                        self->dtc->is_injection_bar = true;

                        for (int c = 0; c < self->numChannels; ++c) {
                            for (int i = 0; i < ticksPerBar; i++) {
                                self->output_patterns[c][i] = self->learned_patterns[c].hit_positions_bar1[i] > 0;
                            }

                            uint8_t probMicrotiming = self->v[kParamProbMicrotiming];
                            uint8_t probOmission = self->v[kParamProbOmission];
                            uint8_t probRoll = self->v[kParamProbRoll];
                            uint8_t probDensity = self->v[kParamProbDensity];
                            uint8_t probPermutation = self->v[kParamProbPermutation];
                            uint8_t probPolyrhythm = self->v[kParamProbPolyrhythm];

                            if (shouldApplyInjection(probMicrotiming, fuel, self->dtc->prng)) {
                                int range = calculateMicrotimingRange(ppqn);
                                for (int i = 0; i < ticksPerBar; i++) {
                                    if (self->output_patterns[c][i]) {
                                        int shift = (self->dtc->prng.next() % (range * 2 + 1)) - range;
                                        int adjacent =
                                                (i > 0 && self->output_patterns[c][i - 1]) ? i - 1 :
                                                (i < ticksPerBar - 1 && self->output_patterns[c][i + 1]) ? i + 1 : -1;
                                        int newPos = applyMicrotimingShift(i, shift, adjacent);
                                        if (newPos != i && newPos >= 0 && newPos < ticksPerBar) {
                                            self->output_patterns[c][i] = false;
                                            self->output_patterns[c][newPos] = true;
                                        }
                                    }
                                }
                            }

                            if (shouldApplyInjection(probOmission, fuel, self->dtc->prng)) {
                                uint8_t omitIndices[MAX_TICKS_PER_BAR];
                                uint8_t omitCount = 0;
                                selectHitsForOmission(&self->learned_patterns[c], omitIndices, &omitCount, fuel, &self->dtc->prng, ticksPerBar);
                                applyOmissionInjection(self->output_patterns[c], omitIndices, omitCount);
                            }

                            if (shouldApplyInjection(probRoll, fuel, self->dtc->prng)) {
                                uint8_t rollIndices[MAX_TICKS_PER_BAR];
                                uint8_t rollCount = 0;
                                uint8_t rollSubdivisions[MAX_TICKS_PER_BAR];
                                selectHitsForRoll(&self->learned_patterns[c], rollIndices, &rollCount, rollSubdivisions, fuel, &self->dtc->prng, ticksPerBar);
                                applyRollInjection(self->output_patterns[c], rollIndices, rollCount, rollSubdivisions, ppqn);
                            }

                            if (shouldApplyInjection(probDensity, fuel, self->dtc->prng)) {
                                uint8_t burstBeatIndices[MAX_TICKS_PER_BAR / 48];
                                uint8_t burstCount = 0;
                                selectBeatsForDensityBurst(&self->learned_patterns[c], burstBeatIndices, &burstCount, fuel, &self->dtc->prng, ticksPerBar, ppqn);
                                applyDensityBurstInjection(self->output_patterns[c], burstBeatIndices, burstCount, ppqn);
                            }

	                            if (shouldApplyInjection(probPermutation, fuel, self->dtc->prng)) {
	                                const uint16_t eighthNoteTicks = (ppqn >= 2) ? (uint16_t)(ppqn / 2) : 0;
	                                if (eighthNoteTicks > 0) {
	                                    const uint8_t segmentCount = (uint8_t)(ticksPerBar / (int)eighthNoteTicks);
	                                    uint8_t permutation[16];
	                                    generatePermutation(permutation, segmentCount, &self->dtc->prng);
	                                    bool permutedPattern[MAX_TICKS_PER_BAR];
	                                    memset(permutedPattern, 0, ticksPerBar * sizeof(bool));
	                                    applyPermutationInjection(self->output_patterns[c], permutedPattern, permutation, (uint16_t)ppqn, (uint16_t)ticksPerBar);
	                                    memcpy(self->output_patterns[c], permutedPattern, ticksPerBar * sizeof(bool));
	                                }
	                            }

                            if (shouldApplyInjection(probPolyrhythm, fuel, self->dtc->prng)) {
                                uint8_t polyType = selectPolyrhythmType(&self->dtc->prng);
                                applyPolyrhythmInjection(self->output_patterns[c], polyType, ppqn, barLength);
                            }
                        }
                    }
                }
            }

            // Rotate the recording buffers for the next bar: bar1 -> bar2, clear bar1.
            for (int c = 0; c < self->numChannels; ++c) {
                shiftBarsForNewBar(self->patterns[c]);
            }
        }
    }
}

// Custom UI check
static uint32_t fuel_injector_has_custom_ui(_NT_algorithm* self_base) {
    // Return 0 for now (no custom UI overrides)
    // Could return kNT_potButtonL | kNT_potButtonC | kNT_potButtonR to override pots
    return 0;
}

// Setup UI pots
static void fuel_injector_setup_ui(_NT_algorithm* self_base, _NT_float3& pots) {
    _FuelInjectorAlgorithm* self = static_cast<_FuelInjectorAlgorithm*>(self_base);
    
    // Map pot to Fuel parameter (0-100 -> 0.0-1.0)
    pots[0] = self->v[kParamFuel] / 100.0f;
    pots[1] = 0.0f;
    pots[2] = 0.0f;
}

// Custom UI handling
static void fuel_injector_custom_ui(_NT_algorithm* self_base, const _NT_uiData& data) {
    _FuelInjectorAlgorithm* self = static_cast<_FuelInjectorAlgorithm*>(self_base);
    (void)self;
    (void)data;
}

// Draw custom display
static bool fuel_injector_draw(_NT_algorithm* self_base) {
    _FuelInjectorAlgorithm* self = static_cast<_FuelInjectorAlgorithm*>(self_base);
    if (!self || !self->dtc) {
        return false;
    }

    const _FuelInjector_DTC* dtc = self->dtc;

    const char* stateStr = "LEARN";
    switch (dtc->state) {
        case LEARNING: stateStr = "LEARN"; break;
        case LOCKED: stateStr = "LOCK"; break;
        case INJECTING: stateStr = "INJ"; break;
    }

    // Leave room for the host's parameter line (top of screen).
    NT_drawText(2, 20, "Fuel Injector", 15, kNT_textLeft, kNT_textTiny);

    char numBuf[12] = {0};
    char lineBuf[32] = {0};

    // State
    NT_drawText(2, 28, stateStr, 15, kNT_textLeft, kNT_textTiny);

    // Bar
    int nlen = NT_intToString(numBuf, (int32_t)dtc->bar_counter);
    int pos = 0;
    const char* prefix = "Bar:";
    while (prefix[pos] && pos < (int)sizeof(lineBuf) - 1) {
        lineBuf[pos] = prefix[pos];
        pos++;
    }
    for (int i = 0; i < nlen && pos < (int)sizeof(lineBuf) - 1; i++) {
        lineBuf[pos++] = numBuf[i];
    }
    lineBuf[pos] = '\0';
    NT_drawText(48, 28, lineBuf, 15, kNT_textLeft, kNT_textTiny);

    // Tick
    memset(lineBuf, 0, sizeof(lineBuf));
    nlen = NT_intToString(numBuf, (int32_t)dtc->current_bar_position);
    pos = 0;
    prefix = "Tick:";
    while (prefix[pos] && pos < (int)sizeof(lineBuf) - 1) {
        lineBuf[pos] = prefix[pos];
        pos++;
    }
    for (int i = 0; i < nlen && pos < (int)sizeof(lineBuf) - 1; i++) {
        lineBuf[pos++] = numBuf[i];
    }
    lineBuf[pos] = '\0';
    NT_drawText(120, 28, lineBuf, 15, kNT_textLeft, kNT_textTiny);

    // Learned hit count for channel 1 (if available).
    if (self->learned_patterns != nullptr) {
        memset(lineBuf, 0, sizeof(lineBuf));
        nlen = NT_intToString(numBuf, (int32_t)self->learned_patterns[0].hit_count_bar1);
        pos = 0;
        prefix = "Ch1Hits:";
        while (prefix[pos] && pos < (int)sizeof(lineBuf) - 1) {
            lineBuf[pos] = prefix[pos];
            pos++;
        }
        for (int i = 0; i < nlen && pos < (int)sizeof(lineBuf) - 1; i++) {
            lineBuf[pos++] = numBuf[i];
        }
        lineBuf[pos] = '\0';
        NT_drawText(2, 36, lineBuf, 12, kNT_textLeft, kNT_textTiny);
    } else {
        NT_drawText(2, 36, "No DRAM", 12, kNT_textLeft, kNT_textTiny);
    }

    // Clock period (in samples) and current pulse remaining for channel 1.
    memset(lineBuf, 0, sizeof(lineBuf));
    nlen = NT_intToString(numBuf, (int32_t)dtc->last_clock_period_samples);
    pos = 0;
    prefix = "ClkPer:";
    while (prefix[pos] && pos < (int)sizeof(lineBuf) - 1) {
        lineBuf[pos] = prefix[pos];
        pos++;
    }
    for (int i = 0; i < nlen && pos < (int)sizeof(lineBuf) - 1; i++) {
        lineBuf[pos++] = numBuf[i];
    }
    lineBuf[pos] = '\0';
    NT_drawText(2, 44, lineBuf, 12, kNT_textLeft, kNT_textTiny);

    memset(lineBuf, 0, sizeof(lineBuf));
    nlen = NT_intToString(numBuf, (int32_t)dtc->trigger_active_steps_remaining[0]);
    pos = 0;
    prefix = "Rem1:";
    while (prefix[pos] && pos < (int)sizeof(lineBuf) - 1) {
        lineBuf[pos] = prefix[pos];
        pos++;
    }
    for (int i = 0; i < nlen && pos < (int)sizeof(lineBuf) - 1; i++) {
        lineBuf[pos++] = numBuf[i];
    }
    lineBuf[pos] = '\0';
    NT_drawText(2, 52, lineBuf, 12, kNT_textLeft, kNT_textTiny);

    return false;
}



// Factory definition
static const _NT_factory s_fuel_injector_factory = {
    .guid = FUEL_INJECTOR_GUID,
    .name = "Fuel Injector",
    .description = "Multi-channel trigger processor with pattern learning and injection",
    .numSpecifications = ARRAY_SIZE(specifications),
    .specifications = specifications,
    .calculateStaticRequirements = fuel_injector_calculate_static_requirements,
    .initialise = fuel_injector_initialise,
    .calculateRequirements = fuel_injector_calculate_requirements,
    .construct = fuel_injector_construct,
    .parameterChanged = fuel_injector_parameter_changed,
    .step = fuel_injector_step,
    .draw = fuel_injector_draw,
    .midiRealtime = NULL,
    .midiMessage = NULL,
    .tags = kNT_tagUtility,
    .hasCustomUi = fuel_injector_has_custom_ui,
    .customUi = fuel_injector_custom_ui,
    .setupUi = fuel_injector_setup_ui,
    .serialise = NULL,
    .deserialise = NULL,
    .midiSysEx = NULL,
    .parameterUiPrefix = NULL,
    .parameterString = NULL
};

// Plugin entry point
extern "C" uintptr_t pluginEntry(_NT_selector selector, uint32_t data) {
    switch (selector) {
        case kNT_selector_version:
            return kNT_apiVersionCurrent;
        case kNT_selector_numFactories:
            return 1;
        case kNT_selector_factoryInfo:
            return (data == 0) ? reinterpret_cast<uintptr_t>(&s_fuel_injector_factory) : 0;
        default:
            return 0;
    }
}
