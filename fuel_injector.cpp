#include <distingnt/api.h>
#include "fuel_injector.h"

enum { kSpecChannels };

static const _NT_specification specifications[] = {
    { .name = "Channels", .min = 1, .max = MAX_CHANNELS, .def = 4, .type = kNT_typeGeneric },
};

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
    kNumControlParams,
};

enum {
    kParamClockSource,
    kParamClockInput,
    kParamResetInput,
    kParamTriggerIn = 0,
    kParamTriggerOut,
    kParamsPerChannel,
};

static const char* clockSourceStrings[] = { "CV", "MIDI", NULL };

static const _NT_parameter controlParameters[] = {
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
};

static const _NT_parameter routingParamTemplate[] = {
    { .name = "Trigger In", .min = 1, .max = 16, .def = 1, .unit = kNT_unitNone, .scaling = 0, .enumStrings = NULL },
    { .name = "Trigger Out", .min = 1, .max = 16, .def = 1, .unit = kNT_unitNone, .scaling = 0, .enumStrings = NULL },
};

static const uint8_t pageControl[] = {
    kParamFuel, kParamPPQN, kParamBarLength, kParamInjectionInterval, kParamLearningBars,
    kParamProbMicrotiming, kParamProbOmission, kParamProbRoll,
    kParamProbDensity, kParamProbPermutation, kParamProbPolyrhythm
};

static void calculateRequirements(_NT_algorithmRequirements& req, const int32_t* specs) {
    int ch = specs ? specs[kSpecChannels] : 4;
    int np = kNumControlParams + 3 + ch * kParamsPerChannel;
    int npg = 2;
    
    req.numParameters = np;
    req.sram = sizeof(_FuelInjectorAlgorithm) + 
               np * sizeof(_NT_parameter) + 
               npg * sizeof(_NT_parameterPage) + 
               ch * MAX_CHANNELS;
    req.dtc = sizeof(_FuelInjector_DTC);
    req.flash = 0;
}

static _NT_algorithm* create(const int32_t* specs) {
    int numChannels = specs ? specs[kSpecChannels] : 4;
    
    _FuelInjectorAlgorithm* p = new _FuelInjectorAlgorithm();
    if (!p) return NULL;
    
    return p;
}

static void destroy(_NT_algorithm* p) {
    delete (_FuelInjectorAlgorithm*)p;
}

static void parameterChanged(_NT_algorithm* alg, int index, int32_t value) {
    _FuelInjectorAlgorithm* p = (_FuelInjectorAlgorithm*)alg;
    
    if (index == kParamFuel) {
    } else if (index == kParamPPQN) {
    } else if (index == kParamBarLength) {
    } else if (index == kParamInjectionInterval) {
    } else if (index == kParamLearningBars) {
    }
}

static void reset(_NT_algorithm* alg) {
    _FuelInjectorAlgorithm* p = (_FuelInjectorAlgorithm*)alg;
    if (p->dtc) {
        p->dtc->state = LEARNING;
        p->dtc->bar_counter = 0;
        p->dtc->current_bar_position = 0;
    }
}

static void process(_NT_algorithm* alg, _NT_algorithmState* state) {
}

_NT_PLUGIN_EXPORT const _NT_pluginInfo NT_PLUGIN_INFO = {
    .apiVersion = NT_API_VERSION,
    .guid = "FuIn",
    .name = "Fuel Injector",
    .description = "Multi-channel trigger processor with pattern learning and injection",
    .author = "Neal Sanche",
    .specifications = specifications,
    .numSpecifications = ARRAY_SIZE(specifications),
    .calculateRequirements = calculateRequirements,
    .create = create,
    .destroy = destroy,
    .reset = reset,
    .process = process,
    .parameterChanged = parameterChanged,
};
