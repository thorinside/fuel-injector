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

// Parameter indices (implicitly defined by array order)
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
    // Channel 1
    kParamCh1TrigIn,
    kParamCh1TrigOut,
    kParamCh1TrigOutMode,
    // Channel 2
    kParamCh2TrigIn,
    kParamCh2TrigOut,
    kParamCh2TrigOutMode,
    // Channel 3
    kParamCh3TrigIn,
    kParamCh3TrigOut,
    kParamCh3TrigOutMode,
    // Channel 4
    kParamCh4TrigIn,
    kParamCh4TrigOut,
    kParamCh4TrigOutMode,
    // Channel 5
    kParamCh5TrigIn,
    kParamCh5TrigOut,
    kParamCh5TrigOutMode,
    // Channel 6
    kParamCh6TrigIn,
    kParamCh6TrigOut,
    kParamCh6TrigOutMode,
    // Channel 7
    kParamCh7TrigIn,
    kParamCh7TrigOut,
    kParamCh7TrigOutMode,
    // Channel 8
    kParamCh8TrigIn,
    kParamCh8TrigOut,
    kParamCh8TrigOutMode,
    kNumParameters
};

static const char* clockSourceStrings[] = { "CV", "MIDI", NULL };

// Parameter definitions
static const _NT_parameter s_parameters[] = {
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
    // Channel 1
    NT_PARAMETER_CV_INPUT("Ch1 Trig In", 0, 3)
    NT_PARAMETER_CV_OUTPUT_WITH_MODE("Ch1 Trig Out", 0, 15)
    // Channel 2
    NT_PARAMETER_CV_INPUT("Ch2 Trig In", 0, 4)
    NT_PARAMETER_CV_OUTPUT_WITH_MODE("Ch2 Trig Out", 0, 16)
    // Channel 3
    NT_PARAMETER_CV_INPUT("Ch3 Trig In", 0, 5)
    NT_PARAMETER_CV_OUTPUT_WITH_MODE("Ch3 Trig Out", 0, 17)
    // Channel 4
    NT_PARAMETER_CV_INPUT("Ch4 Trig In", 0, 6)
    NT_PARAMETER_CV_OUTPUT_WITH_MODE("Ch4 Trig Out", 0, 18)
    // Channel 5
    NT_PARAMETER_CV_INPUT("Ch5 Trig In", 0, 7)
    NT_PARAMETER_CV_OUTPUT_WITH_MODE("Ch5 Trig Out", 0, 19)
    // Channel 6
    NT_PARAMETER_CV_INPUT("Ch6 Trig In", 0, 8)
    NT_PARAMETER_CV_OUTPUT_WITH_MODE("Ch6 Trig Out", 0, 20)
    // Channel 7
    NT_PARAMETER_CV_INPUT("Ch7 Trig In", 0, 9)
    NT_PARAMETER_CV_OUTPUT_WITH_MODE("Ch7 Trig Out", 0, 21)
    // Channel 8
    NT_PARAMETER_CV_INPUT("Ch8 Trig In", 0, 10)
    NT_PARAMETER_CV_OUTPUT_WITH_MODE("Ch8 Trig Out", 0, 22)
};

// Parameter pages
static const uint8_t s_page_control[] = {
    kParamFuel, kParamPPQN, kParamBarLength, kParamInjectionInterval, kParamLearningBars,
    kParamProbMicrotiming, kParamProbOmission, kParamProbRoll,
    kParamProbDensity, kParamProbPermutation, kParamProbPolyrhythm
};

static const uint8_t s_page_routing[] = {
    kParamClockSource, kParamClockInput, kParamResetInput,
    kParamCh1TrigIn, kParamCh1TrigOut, kParamCh1TrigOutMode,
    kParamCh2TrigIn, kParamCh2TrigOut, kParamCh2TrigOutMode,
    kParamCh3TrigIn, kParamCh3TrigOut, kParamCh3TrigOutMode,
    kParamCh4TrigIn, kParamCh4TrigOut, kParamCh4TrigOutMode,
    kParamCh5TrigIn, kParamCh5TrigOut, kParamCh5TrigOutMode,
    kParamCh6TrigIn, kParamCh6TrigOut, kParamCh6TrigOutMode,
    kParamCh7TrigIn, kParamCh7TrigOut, kParamCh7TrigOutMode,
    kParamCh8TrigIn, kParamCh8TrigOut, kParamCh8TrigOutMode
};

static const _NT_parameterPage s_pages[] = {
    { .name = "Control", .numParams = ARRAY_SIZE(s_page_control), .params = s_page_control },
    { .name = "Routing", .numParams = ARRAY_SIZE(s_page_routing), .params = s_page_routing },
};

static const _NT_parameterPages parameterPages = {
    .numPages = ARRAY_SIZE(s_pages),
    .pages = s_pages,
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
    int numChannels = specifications ? specifications[kSpecChannels] : 4;
    
    req.numParameters = kNumParameters;
    req.sram = sizeof(_FuelInjectorAlgorithm);
    req.dram = 0;
    req.dtc = sizeof(_FuelInjector_DTC);
    req.itc = 0;
}

// Construct algorithm instance
static _NT_algorithm* fuel_injector_construct(const _NT_algorithmMemoryPtrs& ptrs, const _NT_algorithmRequirements& req, const int32_t* specifications) {
    // Use placement new to construct algorithm in provided SRAM
    _FuelInjectorAlgorithm* alg = new (ptrs.sram) _FuelInjectorAlgorithm();
    
    // Initialize inherited members
    alg->parameters = s_parameters;
    alg->parameterPages = &parameterPages;
    
    // Initialize DTC (hot state)
    if (ptrs.dtc) {
        alg->dtc = (_FuelInjector_DTC*)ptrs.dtc;
        alg->dtc->state = LEARNING;
        alg->dtc->bar_counter = 0;
        alg->dtc->current_bar_position = 0;
        alg->dtc->clock_tick_counter = 0;
        alg->dtc->prev_clock_value = 0.0f;
        alg->dtc->prev_reset_value = 0.0f;
    }
    
    return reinterpret_cast<_NT_algorithm*>(alg);
}

// Parameter changed callback
static void fuel_injector_parameter_changed(_NT_algorithm* self_base, int p_idx) {
    _FuelInjectorAlgorithm* self = static_cast<_FuelInjectorAlgorithm*>(self_base);
    
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
    
    // TODO: Implement actual processing
    // For now, just pass through
    int numFrames = numFramesBy4 * 4;
    
    // Process clock input
    int clockBus = self->v[kParamClockInput] - 1;
    int resetBus = self->v[kParamResetInput] - 1;
    
    for (int frame = 0; frame < numFrames; frame++) {
        // Clock detection (CV mode)
        if (self->v[kParamClockSource] == 0) {  // CV
            float clockValue = busFrames[clockBus * numFrames + frame];
            
            // Rising edge detection at 1.0V threshold
            if (clockValue >= 1.0f && self->dtc->prev_clock_value < 1.0f) {
                // Clock tick detected
                self->dtc->clock_tick_counter++;
                
                // Calculate bar position
                int ppqn = self->v[kParamPPQN];
                int barLength = self->v[kParamBarLength];
                int ticksPerBar = ppqn * barLength;
                
                if (self->dtc->clock_tick_counter >= ticksPerBar) {
                    self->dtc->clock_tick_counter = 0;
                    self->dtc->bar_counter++;
                }
                
                self->dtc->current_bar_position = self->dtc->clock_tick_counter;
            }
            
            self->dtc->prev_clock_value = clockValue;
        }
        
        // Reset detection
        float resetValue = busFrames[resetBus * numFrames + frame];
        if (resetValue >= 1.0f && self->dtc->prev_reset_value < 1.0f) {
            // Reset triggered
            self->dtc->state = LEARNING;
            self->dtc->bar_counter = 0;
            self->dtc->current_bar_position = 0;
            self->dtc->clock_tick_counter = 0;
        }
        self->dtc->prev_reset_value = resetValue;
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
    
    // TODO: Implement custom UI controls
}

// Draw custom display
static bool fuel_injector_draw(_NT_algorithm* self_base) {
    _FuelInjectorAlgorithm* self = static_cast<_FuelInjectorAlgorithm*>(self_base);
    
    // Determine state text
    const char* state_text = "LEARNING";
    if (self->dtc) {
        if (self->dtc->state == LOCKED) {
            state_text = "LOCKED";
        } else if (self->dtc->state == INJECTING) {
            state_text = "INJECTING";
        }
    }
    
    // Draw state (using platform adapter would require access to it)
    // For now, return false to show standard parameter display
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
    .setupUi = fuel_injector_setup_ui
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
