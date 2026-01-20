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

// Shared parameters (14 params: indices 0-13)
static const _NT_parameter sharedParameters[] = {
    { .name = "Fuel", .min = 0, .max = 100, .def = 100, .unit = kNT_unitPercent, .scaling = 0, .enumStrings = NULL },
    { .name = "PPQN", .min = 24, .max = 96, .def = 48, .unit = kNT_unitNone, .scaling = 0, .enumStrings = NULL },
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
                + (3 + numChannels * 3) * sizeof(uint8_t);  // routing page indices
    req.dram = 0;
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
    
    alg->numChannels = numChannels;
    
    // Copy shared parameters
    memcpy(alg->params, sharedParameters, sizeof(sharedParameters));
    
    // Build per-channel parameters
    for (int c = 0; c < numChannels; ++c) {
        int base = kNumSharedParams + c * kParamsPerChannel;
        memcpy(&alg->params[base], channelParamTemplate, sizeof(channelParamTemplate));
        
        // Set default bus assignments
        alg->params[base + 0].def = 3 + c;   // Trig In
        alg->params[base + 1].def = 15 + c;  // Trig Out
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
        alg->dtc->current_bar_position = 0;
        alg->dtc->clock_tick_counter = 0;
        alg->dtc->prev_clock_value = 0.0f;
        alg->dtc->prev_reset_value = 0.0f;
        alg->dtc->current_bar_index = 0;
        alg->dtc->is_injection_bar = false;
        alg->dtc->prng.state = 12345;
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

// Parameter changed callback
static void fuel_injector_parameter_changed(_NT_algorithm* self_base, int p_idx) {
    _FuelInjectorAlgorithm* self = static_cast<_FuelInjectorAlgorithm*>(self_base);
    (void)self;
    
    // Handle parameter changes
    switch (p_idx) {
        case kParamFuel:
        case kParamPPQN:
        case kParamBarLength:
        case kParamInjectionInterval:
        case kParamLearningBars:
            // Parameters are already in self->v[], no additional processing needed for now
            break;
    }
}

// Main audio processing callback
static void fuel_injector_step(_NT_algorithm* self_base, float* busFrames, int numFramesBy4) {
    _FuelInjectorAlgorithm* self = static_cast<_FuelInjectorAlgorithm*>(self_base);
    
    int numFrames = numFramesBy4 * 4;
    int clockBus = self->v[kParamClockInput] - 1;
    int resetBus = self->v[kParamResetInput] - 1;
    int ppqn = self->v[kParamPPQN];
    int barLength = self->v[kParamBarLength];
    int ticksPerBar = ppqn * barLength;
    
    const int TRIGGER_PULSE_SAMPLES = 5;
    const float TRIGGER_THRESHOLD = 1.0f;
    const float TRIGGER_HIGH = 5.0f;
    
    for (int frame = 0; frame < numFrames; frame++) {
        bool clockTick = false;
        
        // Clock detection (CV mode)
        if (self->v[kParamClockSource] == 0) {
            float clockValue = busFrames[clockBus * numFrames + frame];
            
            if (clockValue >= TRIGGER_THRESHOLD && self->dtc->prev_clock_value < TRIGGER_THRESHOLD) {
                clockTick = true;
                self->dtc->clock_tick_counter++;
                
                if (self->dtc->clock_tick_counter >= (uint32_t)ticksPerBar) {
                    self->dtc->clock_tick_counter = 0;
                    self->dtc->bar_counter++;
                    self->dtc->current_bar_index = (self->dtc->current_bar_index + 1) % 2;
                    
                    for (int c = 0; c < self->numChannels; ++c) {
                        shiftBarsForNewBar(self->patterns[c]);
                    }
                }
                
                self->dtc->current_bar_position = self->dtc->clock_tick_counter;
            }
            
            self->dtc->prev_clock_value = clockValue;
        }
        
        // Reset detection
        float resetValue = busFrames[resetBus * numFrames + frame];
        if (resetValue >= TRIGGER_THRESHOLD && self->dtc->prev_reset_value < TRIGGER_THRESHOLD) {
            self->dtc->state = LEARNING;
            self->dtc->bar_counter = 0;
            self->dtc->current_bar_position = 0;
            self->dtc->clock_tick_counter = 0;
            self->dtc->current_bar_index = 0;
            
            for (int c = 0; c < self->numChannels; ++c) {
                memset(&self->patterns[c], 0, sizeof(ChannelPattern));
            }
        }
        self->dtc->prev_reset_value = resetValue;
        
        // Process each channel's trigger input/output
        for (int c = 0; c < self->numChannels; ++c) {
            int base = kNumSharedParams + c * kParamsPerChannel;
            int trigInBus = self->v[base + kChannelParamTrigIn] - 1;
            int trigOutBus = self->v[base + kChannelParamTrigOut] - 1;
            bool replaceMode = self->v[base + kChannelParamTrigOutMode];
            
            float trigInValue = busFrames[trigInBus * numFrames + frame];
            float* trigOut = &busFrames[trigOutBus * numFrames + frame];
            
            // Detect trigger input rising edge
            bool triggerDetected = (trigInValue >= TRIGGER_THRESHOLD && 
                                   self->dtc->prev_trigger_value[c] < TRIGGER_THRESHOLD);
            
            if (triggerDetected && clockTick) {
                int tickPos = self->dtc->current_bar_position;
                recordHit(self->patterns[c], self->dtc->current_bar_index, tickPos);
            }
            
            self->dtc->prev_trigger_value[c] = trigInValue;
            
            // Generate output trigger
            bool shouldOutput = false;
            
            if (triggerDetected) {
                self->dtc->trigger_active_steps_remaining[c] = TRIGGER_PULSE_SAMPLES;
            }
            
            if (self->dtc->trigger_active_steps_remaining[c] > 0) {
                shouldOutput = true;
                self->dtc->trigger_active_steps_remaining[c]--;
            }
            
            // Write output
            float outputValue = shouldOutput ? TRIGGER_HIGH : 0.0f;
            if (replaceMode) {
                *trigOut = outputValue;
            } else {
                *trigOut += outputValue;
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
    (void)self_base;
    return false;
}

// Parameter UI prefix callback for channel numbering
static int fuel_injector_parameter_ui_prefix(_NT_algorithm* self_base, int p, char* buff) {
    const int kNumSharedParams = 14;
    const int kParamsPerChannel = 3;
    
    if (p >= kNumSharedParams) {
        int channel = (p - kNumSharedParams) / kParamsPerChannel;
        int len = NT_intToString(buff, 1 + channel);
        buff[len++] = ':';
        buff[len] = 0;
        return len;
    }
    
    return 0;
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
    .parameterUiPrefix = fuel_injector_parameter_ui_prefix,
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
