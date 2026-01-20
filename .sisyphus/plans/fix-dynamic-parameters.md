# Fix Specification-Dependent Parameters

## Context

### Original Request
Fix the Fuel Injector distingNT plugin so that the number of trigger input/output parameters on the Routing page is determined by the Channels specification (1-8), rather than hardcoding all 8 channels.

### Interview Summary
**Key Discussions**:
- Current implementation has static parameter arrays with all 8 channels hardcoded
- The Channels specification (1-8) is defined but ignored for parameter count
- Need to follow the distingNT API pattern for dynamic parameters
- Control page (11 params) stays static; only Routing page becomes dynamic

**Research Findings**:
- The `advanced-plugin.cpp` template in the skill shows the exact pattern
- Parameters organized as: shared params + (numChannels × paramsPerChannel)
- Extra SRAM allocated for dynamic arrays
- `parameterUiPrefix` callback adds "1:", "2:", etc. prefixes

### Metis Review
**Identified Gaps** (addressed in plan):
- Missing `parameterUiPrefix` callback in factory definition
- Need to guard parameter access in `step()` with dynamic indices
- Edge case: null specifications pointer needs handling (default to 4)
- Edge case: spec value of 0 should clamp to 1
- Need acceptance criteria for 1, 4, and 8 channel configurations

**Guardrails Applied**:
- Follow distingNT pattern exactly - no invented patterns
- Keep static arrays for common parameters (Control page)
- Preserve existing parameter indices for shared params
- Don't change injection logic in fuel_injector.h
- Don't add per-channel injection settings
- Don't modify Control page structure

---

## Work Objectives

### Core Objective
Refactor the parameter system so that the Routing page shows only the trigger inputs/outputs for the number of channels specified, instead of always showing all 8 channels.

### Concrete Deliverables
- Updated `fuel_injector.cpp` with dynamic parameter construction
- Updated `_FuelInjectorAlgorithm` struct with dynamic array storage
- New `parameterUiPrefix` callback in factory

### Definition of Done
- [x] Plugin loads with 1 channel: Routing page shows 3 global + 3 channel params
- [x] Plugin loads with 4 channels: Routing page shows 3 global + 12 channel params
- [x] Plugin loads with 8 channels: Routing page shows 3 global + 24 channel params
- [x] `parameterUiPrefix` shows "1:", "2:", etc. for channel params
- [x] All 529 existing test assertions still pass
- [x] `make hardware` produces valid .o file
- [x] Parameter indices for shared params unchanged

### Must Have
- Dynamic `numParameters` based on Channels specification
- Dynamic Routing page with only active channel parameters
- `parameterUiPrefix` callback for channel numbering
- Proper memory allocation in `calculateRequirements()`

### Must NOT Have (Guardrails)
- No changes to Control page parameters
- No changes to injection algorithms in fuel_injector.h
- No per-channel injection settings
- No changes to GUID, name, or description
- No new features beyond specification-dependent parameters

---

## Verification Strategy

### Test Decision
- **Infrastructure exists**: YES (529 assertions passing)
- **User wants tests**: Existing tests must continue passing
- **Framework**: Catch2

### Manual Verification
After implementation, verify in nt_emu:
1. Create instance with 1 channel - verify Routing page layout
2. Create instance with 4 channels - verify Routing page layout
3. Create instance with 8 channels - verify Routing page layout
4. Verify parameter prefixes show "1:", "2:", etc.

---

## Task Flow

```
Task 1 (Update Struct)
       │
       ▼
Task 2 (Update calculateRequirements)
       │
       ▼
Task 3 (Update construct)
       │
       ▼
Task 4 (Add parameterUiPrefix)
       │
       ▼
Task 5 (Update step() indices)
       │
       ▼
Task 6 (Verify & Test)
```

## Parallelization

| Task | Depends On | Reason |
|------|------------|--------|
| 1 | - | Foundation - struct changes |
| 2 | 1 | Needs new struct fields |
| 3 | 2 | Needs correct memory allocation |
| 4 | 3 | Needs dynamic parameter arrays |
| 5 | 3 | Needs to know dynamic indices |
| 6 | 4, 5 | Final verification |

---

## TODOs

- [x] 1. Update Algorithm Struct for Dynamic Arrays

  **What to do**:
  - Modify `_FuelInjectorAlgorithm` struct to hold dynamic parameter arrays
  - Add fields:
    - `_NT_parameter* params` - pointer to dynamic parameter array
    - `int numParams` - actual parameter count
    - `_NT_parameterPage* pages` - pointer to dynamic page array  
    - `int numPages` - actual page count (always 2)
    - `uint8_t* routingPageParams` - parameter indices for routing page
    - `_NT_parameterPages paramPages` - pages wrapper struct
    - `int numChannels` - number of active channels
  - Keep existing `dtc` pointer and other fields

  **Must NOT do**:
  - Don't change `_FuelInjector_DTC` struct
  - Don't change `ChannelPattern` or injection-related fields
  - Don't remove `MAX_CHANNELS` constant

  **Parallelizable**: NO (foundation task)

  **References**:
  - Pattern Reference: `/Users/nealsanche/.claude/skills/disting-nt-cpp-plugin-writer/templates/advanced-plugin.cpp:65-84` - Algorithm struct pattern with dynamic arrays
  - Current Implementation: `fuel_injector.cpp:162-182` - Current construct() function

  **Acceptance Criteria**:
  - [ ] `_FuelInjectorAlgorithm` compiles with new fields
  - [ ] `make test` still compiles (may fail until other tasks complete)
  - [ ] Struct can hold pointers to dynamic arrays

  **Commit**: NO (groups with Task 3)

---

- [x] 2. Update calculateRequirements() for Dynamic Parameter Count

  **What to do**:
  - Modify `fuel_injector_calculate_requirements()` to compute dynamic `numParameters`
  - Formula: `numParams = kNumSharedParams + numChannels * kParamsPerChannel`
    - `kNumSharedParams` = 14 (Fuel through Reset Input)
    - `kParamsPerChannel` = 3 (Trig In, Trig Out, Trig Out Mode)
  - Update `req.sram` to include space for dynamic arrays:
    - `sizeof(_FuelInjectorAlgorithm)`
    - `+ numParams * sizeof(_NT_parameter)` 
    - `+ 2 * sizeof(_NT_parameterPage)` (Control + Routing)
    - `+ 11 * sizeof(uint8_t)` (control page indices - static)
    - `+ (3 + numChannels * 3) * sizeof(uint8_t)` (routing page indices - dynamic)
  - Handle edge cases:
    - If `specifications` is NULL, default to 4 channels
    - If `specifications[kSpecChannels]` is 0 or negative, clamp to 1

  **Must NOT do**:
  - Don't change memory allocation for DTC, DRAM, ITC

  **Parallelizable**: NO (depends on 1)

  **References**:
  - Pattern Reference: `/Users/nealsanche/.claude/skills/disting-nt-cpp-plugin-writer/templates/advanced-plugin.cpp:94-104` - Dynamic calculateRequirements pattern
  - Current Implementation: `fuel_injector.cpp:151-159` - Current calculateRequirements()

  **Acceptance Criteria**:
  - [ ] With 1 channel: `numParameters` = 14 + 3 = 17
  - [ ] With 4 channels: `numParameters` = 14 + 12 = 26
  - [ ] With 8 channels: `numParameters` = 14 + 24 = 38
  - [ ] `req.sram` includes dynamic array storage
  - [ ] NULL specifications defaults to 4 channels

  **Commit**: NO (groups with Task 3)

---

- [x] 3. Update construct() to Build Dynamic Parameter Arrays

  **What to do**:
  - Rewrite `fuel_injector_construct()` to build parameters at runtime
  - Memory layout in SRAM after struct:
    1. `_NT_parameter[]` array (numParams entries)
    2. `_NT_parameterPage[]` array (2 entries - Control + Routing)
    3. `uint8_t[]` control page indices (11 entries - static)
    4. `uint8_t[]` routing page indices (3 + numChannels*3 entries - dynamic)
  - Build parameter array:
    1. Copy shared parameters (Fuel through Reset Input) from static array
    2. For each channel (0 to numChannels-1):
       - Copy channel parameter template (3 params: TrigIn, TrigOut, TrigOutMode)
       - Set default input bus = 3 + channel
       - Set default output bus = 15 + channel
  - Build page arrays:
    1. Control page: indices 0-10 (Fuel through P:Polyrhythm)
    2. Routing page: indices 11-13 (Clock Source, Clock Input, Reset Input) + channel params
       - Build dynamically: [11, 12, 13, 14, 15, 16, ...] for all active channels
  - Set `alg->parameters = alg->params`
  - Set `alg->parameterPages = &alg->paramPages`
  - Store `numChannels` in algorithm struct
  
  **NOTE**: Unlike the template which creates one page per channel, Fuel Injector uses
  a single combined Routing page containing all channel parameters.

  **Must NOT do**:
  - Don't change DTC initialization logic
  - Don't change how parameters pointer is assigned to base class

  **Parallelizable**: NO (depends on 2)

  **References**:
  - Pattern Reference: `/Users/nealsanche/.claude/skills/disting-nt-cpp-plugin-writer/templates/advanced-plugin.cpp:106-163` - Dynamic construct pattern
  - Current Implementation: `fuel_injector.cpp:162-182` - Current construct()

  **Acceptance Criteria**:
  - [ ] Parameters built correctly for 1, 4, 8 channels
  - [ ] Control page has 11 parameters (static)
  - [ ] Routing page has 3 + (numChannels * 3) parameters
  - [ ] Default bus assignments: Ch1 = 3/15, Ch2 = 4/16, etc.
  - [ ] `alg->parameters` and `alg->parameterPages` correctly set
  - [ ] `make test` passes

  **Commit**: YES
  - Message: `refactor: make channel parameters specification-dependent`
  - Files: `fuel_injector.cpp`
  - Pre-commit: `make test && make hardware`

---

- [x] 4. Add parameterUiPrefix Callback

  **What to do**:
  - Create `fuel_injector_parameter_ui_prefix()` function
  - For parameters >= `kNumSharedParams` (14):
    - Calculate channel index: `(p - kNumSharedParams) / kParamsPerChannel`
    - Format prefix as "N:" where N = channel + 1
    - Return length of prefix string
  - For parameters < `kNumSharedParams`: return 0 (no prefix)
  - Add to factory struct: `.parameterUiPrefix = fuel_injector_parameter_ui_prefix`

  **Must NOT do**:
  - Don't add prefix to shared parameters (Fuel, Clock Source, etc.)

  **Parallelizable**: NO (depends on 3)

  **References**:
  - Pattern Reference: `/Users/nealsanche/.claude/skills/disting-nt-cpp-plugin-writer/templates/advanced-plugin.cpp:165-174` - parameterUiPrefix implementation
  - API Reference: `/Users/nealsanche/.claude/skills/disting-nt-cpp-plugin-writer/reference.md:429-444` - parameterUiPrefix callback

  **Acceptance Criteria**:
  - [ ] Channel 1 params show "1:" prefix
  - [ ] Channel 2 params show "2:" prefix
  - [ ] Shared params (Fuel, Clock Input, etc.) have no prefix
  - [ ] Factory struct includes `parameterUiPrefix` callback
  - [ ] `make hardware` succeeds

  **Commit**: YES
  - Message: `feat: add parameterUiPrefix for channel numbering`
  - Files: `fuel_injector.cpp`
  - Pre-commit: `make test && make hardware`

---

- [x] 5. Update step() to Use Dynamic Parameter Indices

  **What to do**:
  - Remove hardcoded parameter enums for per-channel params (kParamCh1TrigIn, etc.)
  - Keep shared parameter enums (kParamFuel through kParamResetInput)
  - In `step()`, compute channel parameter indices dynamically:
    ```cpp
    int base = kNumSharedParams + channel * kParamsPerChannel;
    int trigIn = self->v[base + 0];  // Trig In
    int trigOut = self->v[base + 1]; // Trig Out
    bool trigOutMode = self->v[base + 2]; // Trig Out Mode
    ```
  - Guard loop with `numChannels` from algorithm struct
  - Update parameter_changed() if it references channel params

  **Must NOT do**:
  - Don't change clock/reset handling logic
  - Don't change injection algorithm calls

  **Parallelizable**: NO (depends on 3)

  **References**:
  - Pattern Reference: `/Users/nealsanche/.claude/skills/disting-nt-cpp-plugin-writer/templates/advanced-plugin.cpp:215-228` - Dynamic parameter access in step()
  - Current Implementation: `fuel_injector.cpp:201-248` - Current step()

  **Acceptance Criteria**:
  - [ ] No hardcoded kParamCh*TrigIn/Out enums used in step()
  - [ ] Channel parameter access uses computed base index
  - [ ] Loop bounds use `numChannels` from algorithm struct
  - [ ] `make test` passes
  - [ ] `make hardware` succeeds

  **Commit**: YES
  - Message: `refactor: use dynamic parameter indices in step()`
  - Files: `fuel_injector.cpp`
  - Pre-commit: `make test && make hardware`

---

- [x] 6. Final Verification and Cleanup

  **What to do**:
  - Run full test suite: `make test && ./tests/test_runner`
  - Build for hardware: `make hardware`
  - Verify binary size is reasonable (should be similar to before)
  - Clean up any unused code:
    - Remove static per-channel parameter enums if no longer needed
    - Remove static `s_page_routing` array
  - Update README.md with correct parameter count formula
  - Verify parameter structure in comments

  **Must NOT do**:
  - Don't add new tests (existing tests should cover this)
  - Don't change any test files

  **Parallelizable**: NO (final task)

  **References**:
  - Test runner: `make test && ./tests/test_runner`
  - Hardware build: `make hardware`

  **Acceptance Criteria**:
  - [ ] All 529 test assertions pass
  - [ ] `make hardware` produces valid .o file
  - [ ] Binary size < 64KB
  - [ ] No compiler warnings
  - [ ] Code compiles cleanly with no unused variable warnings

  **Commit**: YES
  - Message: `chore: cleanup and verify dynamic parameter implementation`
  - Files: `fuel_injector.cpp`, `README.md` (if updated)
  - Pre-commit: `make test && make hardware`

---

## Commit Strategy

| After Task | Message | Files | Verification |
|------------|---------|-------|--------------|
| 3 | `refactor: make channel parameters specification-dependent` | fuel_injector.cpp | make test && make hardware |
| 4 | `feat: add parameterUiPrefix for channel numbering` | fuel_injector.cpp | make test && make hardware |
| 5 | `refactor: use dynamic parameter indices in step()` | fuel_injector.cpp | make test && make hardware |
| 6 | `chore: cleanup and verify dynamic parameter implementation` | fuel_injector.cpp, README.md | make test && make hardware |

---

## Success Criteria

### Verification Commands
```bash
# All unit tests pass
make test && ./tests/test_runner
# Expected: All tests passed (529 assertions in 28 test cases)

# Hardware build succeeds
make hardware
# Expected: plugins/fuel_injector.o created

# Check binary size
ls -la plugins/fuel_injector.o
# Expected: < 65536 bytes
```

### Final Checklist
- [x] Dynamic parameter count works for 1, 4, 8 channels
- [x] Routing page shows only active channel parameters
- [x] parameterUiPrefix shows "1:", "2:", etc.
- [x] All 529 existing test assertions pass
- [x] Binary size < 64KB (6.3KB verified)
- [x] No changes to injection logic
- [x] No changes to Control page
