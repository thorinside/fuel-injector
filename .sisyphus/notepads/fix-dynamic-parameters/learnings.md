# Learnings: fix-dynamic-parameters

## [2026-01-20T05:17] Session: ses_42d3e0955ffe2QvQAeHfvH3vkq

### Pattern Discovered: distingNT Dynamic Parameters

Successfully implemented specification-dependent parameters following the `advanced-plugin.cpp` template pattern.

**Key Pattern**:
1. Compute `numParameters` in `calculateRequirements()` based on specification
2. Allocate extra SRAM for dynamic arrays after the algorithm struct
3. Build parameter/page arrays at runtime in `construct()`
4. Use `parameterUiPrefix` callback for channel numbering

**Memory Layout** (in SRAM after struct):
```
[_FuelInjectorAlgorithm struct]
[_NT_parameter[] array - dynamic size]
[_NT_parameterPage[] array - 2 pages]
[uint8_t[] control page indices - 11 bytes]
[uint8_t[] routing page indices - 3 + 3N bytes]
```

### Successful Approaches

1. **Struct Organization**: Separated ARM-only fields with `#ifdef __arm__` to allow test compilation
2. **Parameter Enums**: Split into shared params (absolute indices) and per-channel offsets (relative)
3. **Formula**: `totalParams = kNumSharedParams + numChannels * kParamsPerChannel` (14 + 3N)
4. **Default Bus Assignment**: Input = 3+channel, Output = 15+channel

### Technical Gotchas

1. **LSP Errors Expected**: distingNT API types only available in ARM builds - LSP errors in non-ARM context are normal
2. **Compiler Warnings**: Must use `(void)var` to suppress unused variable warnings in placeholder functions
3. **Sign Comparison**: Cast `ticksPerBar` to `uint32_t` when comparing with `clock_tick_counter`
4. **Page Structure**: Unlike template (one page per channel), Fuel Injector uses single combined Routing page

### Verification Results

- ✅ All 529 test assertions pass
- ✅ Hardware build: 6.3KB (well under 64KB limit)
- ✅ No compiler warnings after cleanup
- ✅ Binary size stable (no significant increase)
- ✅ Parameter indices preserved for shared params

### Code Quality

- Removed unnecessary static arrays (s_page_control, s_page_routing, s_pages, parameterPages)
- Added code example in step() showing dynamic parameter access pattern
- Updated README with parameter count formula
- All commits atomic and well-documented
