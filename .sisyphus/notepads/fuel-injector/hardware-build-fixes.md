# Hardware Build API Fixes Required

## Date: 2026-01-18

## Current Status
Attempting to fix hardware compilation errors to unblock remaining 7 checkboxes.

## API Mismatches Identified

### 1. Structure Name Changes
- OLD: `_NT_pluginInfo NT_PLUGIN_INFO`
- NEW: `_NT_factory` (static const)
- Export via `pluginEntry()` function

### 2. Function Signature Changes
- OLD: `static _NT_algorithm* create(const int32_t* specs)`
- NEW: `static _NT_algorithm* construct(const _NT_algorithmMemoryPtrs& ptrs, const _NT_algorithmRequirements& req, const int32_t* specifications)`

- OLD: `static void process(_NT_algorithm* alg, _NT_algorithmState* state)`
- NEW: `static void step(_NT_algorithm* self, float* busFrames, int numFramesBy4)`

- OLD: `static void setupUi(_NT_algorithm* alg, _NT_customUiSetup* setup)`
- NEW: `static void setupUi(_NT_algorithm* self, _NT_float3& pots)`

- OLD: `static void customUi(_NT_algorithm* alg, _NT_customUiState* state)`
- NEW: `static void customUi(_NT_algorithm* self, const _NT_uiData& data)`

- OLD: `static void draw(_NT_algorithm* alg, _NT_displayBuffer* display)`
- NEW: `static bool draw(_NT_algorithm* self)` (returns bool)

### 3. Requirements Structure
- REMOVED: `req.flash` field
- ADDED: `req.dram` and `req.itc` fields

### 4. Additional Required Functions
- `calculateStaticRequirements()` - for shared memory
- `initialise()` - for shared data initialization

### 5. Plugin Export Pattern
```cpp
extern "C" uintptr_t pluginEntry(_NT_selector selector, uint32_t data) {
    switch (selector) {
        case kNT_selector_version:
            return kNT_apiVersionCurrent;
        case kNT_selector_numFactories:
            return 1;
        case kNT_selector_factoryInfo:
            return (data == 0) ? reinterpret_cast<uintptr_t>(&s_factory) : 0;
        default:
            return 0;
    }
}
```

## Reference Implementation
- `/Users/nealsanche/nosuch/nt_grids/nt_grids.cc` - Complete working example
- Lines 197-205: calculateRequirements
- Lines 207-239: construct
- Lines 425-449: setupUi
- Lines 455-493: customUi
- Lines 495-521: draw
- Lines 524-542: factory definition
- Lines 545-558: pluginEntry

## Next Steps
1. Rewrite fuel_injector.cpp to match current API
2. Implement proper construct() with placement new
3. Fix all function signatures
4. Add pluginEntry() export function
5. Test compilation with `make hardware`


## Build Success!

### Date: 2026-01-18 (continued)

**HARDWARE BUILD SUCCESSFUL!**

### Changes Made

1. **Fixed `_NT_staticRequirements` structure**:
   - Removed: `req.sram`, `req.dtc`, `req.itc`
   - Kept only: `req.dram`

2. **Fixed `_FuelInjectorAlgorithm` inheritance**:
   - Added conditional inheritance from `_NT_algorithm` when compiling for ARM
   - Used `#ifdef __arm__` to maintain test compatibility

3. **Rewrote fuel_injector.cpp** to match current distingNT API v6:
   - Changed from `_NT_pluginInfo` to `_NT_factory`
   - Added `pluginEntry()` export function
   - Fixed all function signatures:
     - `construct()` instead of `create()`
     - `step()` instead of `process()`
     - Correct signatures for `setupUi()`, `customUi()`, `draw()`
   - Added `calculateStaticRequirements()` and `initialise()`

### Build Results

```
Built: plugins/fuel_injector.o
Total size: 1122 bytes (well under 64KB limit)
```

### Compilation Warnings (Non-blocking)

- Unused variable warnings (expected for stub implementations)
- Sign comparison warning (minor, can be fixed later)

### Next Steps

The hardware compilation now succeeds. However, remaining checkboxes still require:
- nt_emu emulator (not available)
- Physical distingNT hardware (not available)

These cannot be completed in the current environment.

### Checkboxes Now Completable

- [x] Plugin builds for hardware: `make hardware` produces `.o` file < 64KB ✅
- [x] Binary size < 64KB ✅ (1122 bytes)

### Checkboxes Still Blocked (Hardware/Emulator Required)

- [ ] Plugin loads in nt_emu without errors
- [ ] 8-channel configuration works with CV clock
- [ ] Fuel at 0% passes triggers unchanged
- [ ] nt_emu: Plugin loads in slot
- [ ] nt_emu: CV clock advances bar counter
- [ ] nt_emu: Triggers visible on output busses
- [ ] nt_emu: State shows LEARNING → LOCKED transition
- [ ] nt_emu: Injection visible on injection bars
- [ ] nt_emu: Custom UI renders correctly
- [ ] nt_emu: 8 channels functional simultaneously
- [ ] UI renders without crashes in nt_emu

