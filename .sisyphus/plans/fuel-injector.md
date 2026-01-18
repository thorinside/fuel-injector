# Fuel Injector - Work Plan

## Context

### Original Request
Build "Fuel Injector" - a multi-channel trigger processor plugin for Expert Sleepers distingNT that learns incoming rhythmic patterns and injects musically relevant variations at phrase boundaries. Operates like a drummer who listens first, plays straight most of the time, and adds fills at transitions.

### Interview Summary
**Key Discussions**:
- Channels: Configurable 1-8 via specification (user chooses at instantiation)
- Clock: Both CV (1.0V threshold) and MIDI clock supported, user selects source
- Testing: TDD with Catch2 framework in tests/ directory
- Injection types: All 6 from PRD (Microtiming, Omission, Roll, Density, Permutation, Polyrhythm)
- Pattern buffer: 2 bars history per channel for stability detection
- Custom UI: Pattern visualization with injection activity feedback
- Probabilities: Per-type (0-100%) scaled by Fuel macro (0 stays 0)
- Parameter pages: 2 pages (Control, Routing)
- Bypass: Fuel at 0% = transparent passthrough
- Reset: Forces relearn from scratch

**Research Findings**:
- nt_grids uses countdown counter pattern for trigger pulse width (5 steps)
- Rising edge detection at threshold for clock/triggers
- DTC memory for hot state (counters, phases) - fastest access
- Specifications pattern from advanced-plugin.cpp for configurable channels
- 1-based parameter indexing for bus access

### Metis Review
**Identified Gaps** (addressed in plan):
- Clock threshold: User confirmed 1.0V (Eurorack standard), documented explicitly
- State machine transitions: Defined explicitly with timeout behaviors
- Pattern change detection: Hamming distance algorithm specified (>10% deviation = change)
- Trigger pulse width: 5 steps (matching nt_grids pattern)
- Clock timeout: 2 seconds no clock = hold state
- Injection probability: Independent per-type, not weighted selection
- Reset during injection: Cancel injection immediately, enter learning

**Guardrails Applied**:
- Exactly 6 injection types - no additions
- 2 bars pattern history - not configurable
- 1-8 channels via specification only
- 2 parameter pages maximum
- No pattern persistence across power cycles
- No MIDI output of patterns
- No per-channel injection settings

---

## Work Objectives

### Core Objective
Create a distingNT C++ plugin that learns incoming trigger patterns and injects controlled rhythmic variations at phrase boundaries, preserving groove while adding musical life.

### Concrete Deliverables
- `fuel_injector.cpp` - Main plugin implementation
- `fuel_injector.h` - Header with data structures and constants
- `tests/` directory with Catch2 unit tests
- `Makefile` - Build system for hardware and test targets
- Working plugin loadable in nt_emu and distingNT hardware

### Plugin Identity
- **GUID**: `NT_MULTICHAR('N', 's', 'F', 'I')` (Nealsanche + Fuel Injector)
- **Name**: "Fuel Injector"
- **Tags**: `kNT_tagUtility`

### Definition of Done
- [ ] All unit tests pass: `make test && ./tests/test_runner`
- [ ] Plugin builds for hardware: `make hardware` produces `.o` file < 64KB
- [ ] Plugin loads in nt_emu without errors
- [ ] 8-channel configuration works with CV clock
- [ ] All 6 injection types function as specified
- [ ] Custom UI displays pattern and injection state
- [ ] Fuel at 0% passes triggers unchanged (verified with scope/measurement)

### Must Have
- State machine: Learning → Locked → Injecting → Locked
- Pattern learning at PPQN resolution (24 or 48)
- Pattern change detection with immediate relearn
- All 6 injection types with independent probabilities
- CV and MIDI clock support
- Custom UI with pattern visualization
- Configurable 1-8 channels via specification

### Must NOT Have (Guardrails)
- No ML/AI pattern detection
- No pattern persistence across power cycles
- No MIDI output of learned patterns
- No per-channel injection settings (global only)
- No auto tempo detection from triggers
- No additional injection types beyond the 6 specified
- No more than 2 parameter pages
- No "preview" or "undo" modes
- No swing quantization beyond learned timing

---

## Verification Strategy (MANDATORY)

### Test Decision
- **Infrastructure exists**: NO (new project)
- **User wants tests**: YES (TDD)
- **Framework**: Catch2 (header-only)

### TDD Workflow
Each implementation task follows RED-GREEN-REFACTOR:
1. **RED**: Write failing test first
2. **GREEN**: Implement minimum code to pass
3. **REFACTOR**: Clean up while keeping tests green

### Test Infrastructure Setup (Task 0)
- Install Catch2 header
- Create test runner
- Verify with example test

---

## State Machine Definition

```
                    ┌─────────────────┐
         Reset ────►│    LEARNING     │◄──── Pattern Change
                    │  (passthrough)  │
                    └────────┬────────┘
                             │ Pattern stable
                             │ (N bars identical)
                             ▼
                    ┌─────────────────┐
                    │     LOCKED      │◄──── Injection complete
                    │  (passthrough)  │
                    └────────┬────────┘
                             │ Injection bar reached
                             │ (bar % interval == 0)
                             ▼
                    ┌─────────────────┐
                    │   INJECTING     │
                    │ (apply effects) │
                    └─────────────────┘
```

**Transitions**:
- LEARNING → LOCKED: After `learning_bars` bars with >90% pattern similarity
- LOCKED → INJECTING: On injection bar (bar_count % injection_interval == 0)
- INJECTING → LOCKED: At end of injection bar
- ANY → LEARNING: On reset input OR pattern change detected (>10% deviation)

**Clock Timeout**: If no clock pulse for 2 seconds, hold current state (no transition)

---

## Task Flow

```
Task 0 (Test Infra)
       │
       ▼
Task 1 (Data Structures)
       │
       ▼
┌──────┴──────┐
│             │
▼             ▼
Task 2      Task 3
(CV Clock)  (MIDI Clock)
│             │
└──────┬──────┘
       │
       ▼
Task 4 (Pattern Learning)
       │
       ▼
Task 5 (Change Detection)
       │
       ▼
┌──────────────────────────────────────────────────┐
│                  Injection Types                  │
│  ┌─────┐ ┌─────┐ ┌─────┐ ┌─────┐ ┌─────┐ ┌─────┐ │
│  │ T6  │ │ T7  │ │ T8  │ │ T9  │ │ T10 │ │ T11 │ │
│  └─────┘ └─────┘ └─────┘ └─────┘ └─────┘ └─────┘ │
│  Micro   Omit    Roll   Density  Perm   Poly    │
└──────────────────────────────────────────────────┘
       │
       ▼
Task 12 (State Machine Integration)
       │
       ▼
Task 13 (Parameters & Pages)
       │
       ▼
Task 14 (Custom UI)
       │
       ▼
Task 15 (Hardware Build & Verification)
```

## Parallelization

| Group | Tasks | Reason |
|-------|-------|--------|
| A | 2, 3 | CV Clock and MIDI Clock are independent |
| B | 6, 7, 8, 9, 10, 11 | All injection types are independent |

| Task | Depends On | Reason |
|------|------------|--------|
| 1 | 0 | Data structures need test infra |
| 2, 3 | 1 | Clock handling needs data structures |
| 4 | 2 or 3 | Pattern learning needs clock |
| 5 | 4 | Change detection needs pattern learning |
| 6-11 | 4 | Injection types need pattern data |
| 12 | 5, 6-11 | Integration needs all components |
| 13 | 12 | Parameters need state machine |
| 14 | 13 | UI needs parameters |
| 15 | 14 | Build needs complete plugin |

---

## TODOs

- [x] 0. Setup Test Infrastructure with Catch2

  **What to do**:
  - Download Catch2 single-header to `tests/catch.hpp`
  - Create `tests/test_main.cpp` with `#define CATCH_CONFIG_MAIN`
  - Create `tests/test_example.cpp` with trivial passing test
  - Add `test` target to Makefile
  - Verify: `make test && ./tests/test_runner` passes

  **Must NOT do**:
  - No integration tests (unit tests only)
  - No hardware dependencies in test code
  - No mocking of distingNT API yet

  **Parallelizable**: NO (foundation task)

  **References**:
  - Catch2 docs: https://github.com/catchorg/Catch2/blob/v2.x/docs/tutorial.md
  - Template Makefile: `/Users/nealsanche/.claude/skills/disting-nt-cpp-plugin-writer/templates/Makefile`

  **Acceptance Criteria**:
  - [ ] `tests/catch.hpp` exists (Catch2 v2.x single header)
  - [ ] `make test` compiles test runner without errors
  - [ ] `./tests/test_runner` → 1 test passes, 0 failures
  - [ ] Test output shows "All tests passed"

  **Commit**: YES
  - Message: `chore: setup Catch2 test infrastructure`
  - Files: `tests/catch.hpp`, `tests/test_main.cpp`, `tests/test_example.cpp`, `Makefile`

---

- [x] 1. Define Core Data Structures

  **What to do**:
  - Create `fuel_injector.h` with:
    - `FuelInjectorState` enum: `LEARNING`, `LOCKED`, `INJECTING`
    - `ClockSource` enum: `CV`, `MIDI`
    - `InjectionType` enum: 6 types
    - `ChannelPattern` struct: hit positions, timing data for 2 bars
    - `InjectionConfig` struct: per-type probabilities
    - `_FuelInjector_DTC` struct: hot state (phase, counters, trigger states)
    - `_FuelInjectorAlgorithm` struct: inherits `_NT_algorithm`
    - Simple PRNG struct (xorshift32) for probability rolls
  - Define constants: `MAX_CHANNELS=8`, `MAX_PPQN=48`, `MAX_TICKS_PER_BAR=336`
  - Write unit tests for struct sizes and memory layout

  **Must NOT do**:
  - No implementation logic yet
  - No distingNT API dependencies in header (keep testable)

  **Parallelizable**: NO (depends on 0)

  **References**:
  - Pattern References: `/Users/nealsanche/nosuch/nt_grids/nt_grids.cc:30-80` - Algorithm struct pattern
  - Pattern References: `/Users/nealsanche/.claude/skills/disting-nt-cpp-plugin-writer/templates/advanced-plugin.cpp:65-95` - DTC struct pattern
  - API Reference: `/Users/nealsanche/.claude/skills/disting-nt-cpp-plugin-writer/reference.md:14-34` - _NT_algorithm structure

  **Acceptance Criteria**:
  - [ ] RED: `tests/test_data_structures.cpp` with tests for struct sizes
  - [ ] GREEN: `fuel_injector.h` compiles, tests pass
  - [ ] `sizeof(_FuelInjector_DTC)` fits in reasonable DTC allocation (~1KB)
  - [ ] `sizeof(ChannelPattern) * 8` fits in SRAM (~16KB)
  - [ ] `make test` → all structure tests pass

  **Commit**: YES
  - Message: `feat: define core data structures for Fuel Injector`
  - Files: `fuel_injector.h`, `tests/test_data_structures.cpp`

---

- [x] 2. Implement CV Clock Handling

  **What to do**:
  - Implement rising edge detection at 1.0V threshold
  - Track clock ticks per bar based on PPQN setting
  - Calculate bar position from tick count
  - Handle clock timeout (2 seconds → hold state)
  - Store previous clock value for edge detection
  - Unit test edge detection with sample sequences

  **Must NOT do**:
  - No MIDI clock handling (separate task)
  - No pattern learning yet (just clock tracking)

  **Parallelizable**: YES (with Task 3)

  **References**:
  - Pattern References: `/Users/nealsanche/nosuch/nt_grids/nt_grids.cc:283-296` - Rising edge detection pattern
  - Pattern References: `/Users/nealsanche/nosuch/nt_grids/nt_grids_pattern_generator.cc:45-60` - PPQN tick counting
  - API Reference: `/Users/nealsanche/.claude/skills/disting-nt-cpp-plugin-writer/reference.md:1515-1519` - Trigger detection pattern

  **Acceptance Criteria**:
  - [ ] RED: `tests/test_cv_clock.cpp` with edge detection tests
  - [ ] GREEN: CV clock module passes all tests
  - [ ] Test: Rising edge at 1.0V detected correctly
  - [ ] Test: No false triggers at 0.9V
  - [ ] Test: Bar position advances after (PPQN × bar_length) ticks
  - [ ] Test: Timeout after 2 seconds of no clock

  **Commit**: YES
  - Message: `feat: implement CV clock handling with edge detection`
  - Files: `fuel_injector.cpp` (clock section), `tests/test_cv_clock.cpp`

---

- [x] 3. Implement MIDI Clock Handling

  **What to do**:
  - Implement `midiRealtime()` callback for 0xF8 clock messages
  - Track ticks at 24 PPQN (MIDI standard)
  - Convert to internal PPQN if different (24 or 48)
  - Handle MIDI Start (0xFA) and Stop (0xFC) messages
  - Unit test with simulated MIDI byte sequences

  **Must NOT do**:
  - No MIDI note handling
  - No SysEx handling
  - No MIDI output

  **Parallelizable**: YES (with Task 2)

  **References**:
  - API Reference: `/Users/nealsanche/.claude/skills/disting-nt-cpp-plugin-writer/reference.md:316-327` - midiRealtime callback
  - API Reference: `/Users/nealsanche/.claude/skills/disting-nt-cpp-plugin-writer/examples.md:580-584` - MIDI realtime bytes

  **Acceptance Criteria**:
  - [ ] RED: `tests/test_midi_clock.cpp` with MIDI clock tests
  - [ ] GREEN: MIDI clock module passes all tests
  - [ ] Test: 0xF8 increments tick counter
  - [ ] Test: 24 ticks = 1 quarter note
  - [ ] Test: 0xFA resets tick counter
  - [ ] Test: 0xFC stops clock (no timeout reset)

  **Commit**: YES
  - Message: `feat: implement MIDI clock handling`
  - Files: `fuel_injector.cpp` (MIDI section), `tests/test_midi_clock.cpp`

---

- [x] 4. Implement Pattern Learning

  **What to do**:
  - Detect trigger inputs using 1.0V threshold (Schmitt trigger)
  - Record hit positions relative to clock ticks
  - Store 2 bars of pattern data per channel
  - Calculate pattern similarity between bars (Hamming distance)
  - Transition LEARNING → LOCKED when similarity > 90% for N bars
  - Unit test pattern recording and similarity calculation

  **Must NOT do**:
  - No modification of triggers during learning (passthrough only)
  - No quantization of timing (preserve raw positions)
  - No tempo inference from triggers

  **Parallelizable**: NO (depends on 2 or 3)

  **References**:
  - Pattern References: `/Users/nealsanche/nosuch/nt_grids/nt_grids.cc:283-310` - Trigger detection in step()
  - PRD Reference: Section 6 "Learning Phase" - Behavior specification
  - PRD Reference: Section 12 "Core Processing Flow" - State transitions

  **Acceptance Criteria**:
  - [ ] RED: `tests/test_pattern_learning.cpp`
  - [ ] GREEN: Pattern learning module passes all tests
  - [ ] Test: Hits recorded at correct tick positions
  - [ ] Test: 2 bars of history maintained (oldest discarded)
  - [ ] Test: Similarity = 100% for identical patterns
  - [ ] Test: Similarity = 50% for half-different patterns
  - [ ] Test: State transitions to LOCKED after configured learning_bars

  **Commit**: YES
  - Message: `feat: implement pattern learning with similarity detection`
  - Files: `fuel_injector.cpp` (learning section), `tests/test_pattern_learning.cpp`

---

- [x] 5. Implement Pattern Change Detection

  **What to do**:
  - Compare incoming triggers against learned pattern
  - Detect change when similarity drops below 90%
  - Change indicators: missing hits, new hits, density shift
  - Immediate transition to LEARNING on change
  - Passthrough triggers during transition
  - Unit test various change scenarios

  **Must NOT do**:
  - No gradual adaptation (immediate relearn only)
  - No "grace period" for temporary deviations

  **Parallelizable**: NO (depends on 4)

  **References**:
  - PRD Reference: Section 7 "Pattern Change Detection" - Change indicators
  - PRD Quote: "Pattern change detection must be conservative. False positives are better than groove corruption."

  **Acceptance Criteria**:
  - [ ] RED: `tests/test_change_detection.cpp`
  - [ ] GREEN: Change detection module passes all tests
  - [ ] Test: Missing expected hit triggers relearn
  - [ ] Test: New unexpected hit triggers relearn
  - [ ] Test: >10% density change triggers relearn
  - [ ] Test: State immediately transitions to LEARNING
  - [ ] Test: Triggers pass through unchanged during transition

  **Commit**: YES
  - Message: `feat: implement pattern change detection`
  - Files: `fuel_injector.cpp` (detection section), `tests/test_change_detection.cpp`

---

- [x] 6. Implement Injection: Microtiming Shift

  **What to do**:
  - Shift existing hits by +/- 1/16 note equivalent
  - Calculate shift in ticks: (PPQN / 4) ticks = 1/16 note
  - Ensure shifted hits don't collide (minimum 1 tick apart)
  - Preserve relative spacing of nearby hits
  - Apply probability and Fuel scaling
  - Unit test shift boundaries and collision prevention

  **Must NOT do**:
  - No shifts that collapse two hits to same tick
  - No shifts on unlearned hit positions

  **Parallelizable**: YES (with Tasks 7-11)

  **References**:
  - PRD Reference: Section 9.1 "Microtiming Shift" - Rules and musical effect
  - PRD Quote: "Microtiming must never collapse two hits into the same tick"

  **Acceptance Criteria**:
  - [ ] RED: `tests/test_injection_microtiming.cpp`
  - [ ] GREEN: Microtiming injection passes all tests
  - [ ] Test: Shift range = +/- (PPQN/4) ticks
  - [ ] Test: Adjacent hits maintain minimum 1 tick separation
  - [ ] Test: Probability 0% = no shift
  - [ ] Test: Fuel 50% scales probability by 0.5
  - [ ] Test: Only learned hit positions are shifted

  **Commit**: YES
  - Message: `feat: implement microtiming shift injection`
  - Files: `fuel_injector.cpp` (microtiming section), `tests/test_injection_microtiming.cpp`

---

- [x] 7. Implement Injection: Hit Omission

  **What to do**:
  - Remove selected hits during injection bar
  - Prefer non-downbeat hits for omission
  - Limit: maximum 25% of hits per bar can be omitted
  - Apply probability and Fuel scaling
  - Unit test omission selection and limits

  **Must NOT do**:
  - No omission of downbeat (beat 1) hits
  - No omission beyond 25% limit

  **Parallelizable**: YES (with Tasks 6, 8-11)

  **References**:
  - PRD Reference: Section 9.2 "Hit Omission (Dropout)" - Rules

  **Acceptance Criteria**:
  - [ ] RED: `tests/test_injection_omission.cpp`
  - [ ] GREEN: Hit omission injection passes all tests
  - [ ] Test: Downbeat hits never omitted
  - [ ] Test: Maximum 25% omission enforced
  - [ ] Test: Probability 0% = no omission
  - [ ] Test: Fuel scaling works correctly

  **Commit**: YES
  - Message: `feat: implement hit omission injection`
  - Files: `fuel_injector.cpp` (omission section), `tests/test_injection_omission.cpp`

---

- [x] 8. Implement Injection: Hit Duplication / Roll

  **What to do**:
  - Convert single hit to double, triplet, or ratchet
  - Clock-aligned subdivisions only
  - Double: 2 hits at half timing
  - Triplet: 3 hits at third timing
  - Ratchet: 4+ hits rapid-fire
  - Apply probability and Fuel scaling
  - Unit test subdivision timing accuracy

  **Must NOT do**:
  - No non-clock-aligned subdivisions
  - No rolls extending beyond original hit's beat

  **Parallelizable**: YES (with Tasks 6-7, 9-11)

  **References**:
  - PRD Reference: Section 9.3 "Hit Duplication / Roll" - Rules
  - Pattern Reference: `/Users/nealsanche/nosuch/nt_grids/nt_grids.cc:344-354` - Trigger pulse width pattern

  **Acceptance Criteria**:
  - [ ] RED: `tests/test_injection_roll.cpp`
  - [ ] GREEN: Roll injection passes all tests
  - [ ] Test: Double creates 2 evenly-spaced hits
  - [ ] Test: Triplet creates 3 evenly-spaced hits
  - [ ] Test: Subdivisions align to clock ticks
  - [ ] Test: Roll stays within beat boundary

  **Commit**: YES
  - Message: `feat: implement hit duplication/roll injection`
  - Files: `fuel_injector.cpp` (roll section), `tests/test_injection_roll.cpp`

---

- [x] 9. Implement Injection: Density Burst

  **What to do**:
  - Temporarily increase trigger density during injection bar
  - Add hits at subdivisions of existing beats
  - Revert to normal immediately after injection bar
  - Apply probability and Fuel scaling
  - Unit test density calculation and revert

  **Must NOT do**:
  - No density increase persisting beyond injection bar
  - No hits on completely silent beats (only subdivide existing)

  **Parallelizable**: YES (with Tasks 6-8, 10-11)

  **References**:
  - PRD Reference: Section 9.4 "Density Burst" - Rules

  **Acceptance Criteria**:
  - [ ] RED: `tests/test_injection_density.cpp`
  - [ ] GREEN: Density burst injection passes all tests
  - [ ] Test: Additional hits created at subdivisions
  - [ ] Test: Next bar returns to normal density
  - [ ] Test: Only beats with existing hits get subdivided

  **Commit**: YES
  - Message: `feat: implement density burst injection`
  - Files: `fuel_injector.cpp` (density section), `tests/test_injection_density.cpp`

---

- [x] 10. Implement Injection: Sub-Bar Permutation

  **What to do**:
  - Reorder time segments within bar at eighth-note granularity
  - All channels apply same permutation (synchronized)
  - Example: [1,2,3,4] → [1,3,2,4] at quarter-note level
  - Return to normal order after injection bar
  - Apply probability and Fuel scaling
  - Unit test permutation generation and application

  **Must NOT do**:
  - No reordering of triggers between channels
  - No permutations persisting beyond injection bar
  - No quarter-note or sixteenth granularity (eighth only)

  **Parallelizable**: YES (with Tasks 6-9, 11)

  **References**:
  - PRD Reference: Section 9.5 "Synchronous Sub-Bar Permutation" - Concept and example
  - PRD Quote: "Permutations reorder time segments, not triggers between channels"

  **Acceptance Criteria**:
  - [ ] RED: `tests/test_injection_permutation.cpp`
  - [ ] GREEN: Permutation injection passes all tests
  - [ ] Test: Eighth-note segments reordered correctly
  - [ ] Test: All channels receive same permutation
  - [ ] Test: Channel identity preserved (no cross-channel)
  - [ ] Test: Next bar returns to original order

  **Commit**: YES
  - Message: `feat: implement sub-bar permutation injection`
  - Files: `fuel_injector.cpp` (permutation section), `tests/test_injection_permutation.cpp`

---

- [x] 11. Implement Injection: Polyrhythmic Overlay

  **What to do**:
  - Overlay secondary subdivision during injection bar
  - Support: 3-over-4 and 5-over-4
  - Calculate overlay positions based on bar length
  - Remove overlay immediately after injection bar
  - Apply probability and Fuel scaling
  - Unit test polyrhythm position calculation

  **Must NOT do**:
  - No polyrhythms persisting beyond single bar
  - No complex ratios beyond 3:4 and 5:4

  **Parallelizable**: YES (with Tasks 6-10)

  **References**:
  - PRD Reference: Section 9.6 "Polyrhythmic Overlay" - Examples
  - PRD Quote: "Polyrhythms must not persist beyond a single bar"

  **Acceptance Criteria**:
  - [ ] RED: `tests/test_injection_polyrhythm.cpp`
  - [ ] GREEN: Polyrhythm injection passes all tests
  - [ ] Test: 3-over-4 creates 3 evenly-spaced hits over 4-beat bar
  - [ ] Test: 5-over-4 creates 5 evenly-spaced hits over 4-beat bar
  - [ ] Test: Overlay removed after injection bar
  - [ ] Test: Works correctly with different bar lengths

  **Commit**: YES
  - Message: `feat: implement polyrhythmic overlay injection`
  - Files: `fuel_injector.cpp` (polyrhythm section), `tests/test_injection_polyrhythm.cpp`

---

- [x] 12. Implement State Machine Integration

  **What to do**:
  - Integrate all components into unified state machine
  - Implement state transitions per diagram above
  - Handle reset input (edge-triggered, forces LEARNING)
  - Handle injection bar detection (bar % interval == 0)
  - Implement injection type selection per bar
  - Enforce complexity budget (not all types at once unless probability allows)
  - Unit test complete state machine transitions

  **Must NOT do**:
  - No adding new states
  - No deferred transitions (immediate only)

  **Parallelizable**: NO (depends on 5, 6-11)

  **References**:
  - PRD Reference: Section 10 "Stochastic Selection Model" - Selection logic
  - PRD Reference: Section 12 "Core Processing Flow" - State machine pseudocode
  - State Machine Diagram: See "State Machine Definition" section above

  **Acceptance Criteria**:
  - [ ] RED: `tests/test_state_machine.cpp`
  - [ ] GREEN: State machine passes all tests
  - [ ] Test: LEARNING → LOCKED after stable pattern
  - [ ] Test: LOCKED → INJECTING on injection bar
  - [ ] Test: INJECTING → LOCKED after bar complete
  - [ ] Test: ANY → LEARNING on reset
  - [ ] Test: ANY → LEARNING on pattern change
  - [ ] Test: Injection bar = bar_count % injection_interval == 0

  **Commit**: YES
  - Message: `feat: integrate state machine with all components`
  - Files: `fuel_injector.cpp` (integration), `tests/test_state_machine.cpp`

---

- [ ] 13. Implement Parameter System and Pages

  **What to do**:
  - Define parameters array with all parameters
  - Page 1 (Control): Fuel, PPQN, Bar Length, Injection Interval, Learning Bars, 6× injection probabilities
  - Page 2 (Routing): Clock Source, Clock Input, Reset Input, 8× Trigger In, 8× Trigger Out
  - Implement `parameterChanged()` callback
  - Implement specification for channel count (1-8)
  - Use `parameterUiPrefix` for channel numbering
  - Implement `calculateRequirements()` with proper memory sizing

  **Must NOT do**:
  - No more than 2 parameter pages
  - No per-channel injection settings
  - No runtime channel count changes (specification only)

  **Parallelizable**: NO (depends on 12)

  **References**:
  - Pattern Reference: `/Users/nealsanche/.claude/skills/disting-nt-cpp-plugin-writer/templates/advanced-plugin.cpp:19-55` - Specifications and parameters
  - API Reference: `/Users/nealsanche/.claude/skills/disting-nt-cpp-plugin-writer/reference.md:76-119` - Parameter units and types
  - API Reference: `/Users/nealsanche/.claude/skills/disting-nt-cpp-plugin-writer/reference.md:384-459` - parameterUiPrefix

  **Acceptance Criteria**:
  - [ ] Parameters compile without errors
  - [ ] Page 1 contains all control parameters
  - [ ] Page 2 contains all routing parameters
  - [ ] Specification allows 1-8 channel selection
  - [ ] `parameterUiPrefix` shows "1:", "2:", etc. for per-channel params
  - [ ] `calculateRequirements()` returns correct memory sizes
  - [ ] `parameterChanged()` updates internal state correctly

  **Commit**: YES
  - Message: `feat: implement parameter system with 2 pages`
  - Files: `fuel_injector.cpp` (parameters), `fuel_injector.h` (param enums)

---

- [ ] 14. Implement Custom UI

  **What to do**:
  - Implement `hasCustomUi()` returning true
  - Implement `draw()` for pattern visualization:
    - Show current state (LEARNING/LOCKED/INJECTING)
    - Display learned pattern as dots/lines
    - Show injection activity during injection bars
    - Display bar counter and injection interval
  - Implement `customUi()` for pot/button handling
  - Implement `setupUi()` for pot initialization

  **Must NOT do**:
  - No waveform display (pattern dots only)
  - No complex animations
  - No third parameter page via UI

  **Parallelizable**: NO (depends on 13)

  **References**:
  - API Reference: `/Users/nealsanche/.claude/skills/disting-nt-cpp-plugin-writer/reference.md:696-749` - Display and drawing
  - API Reference: `/Users/nealsanche/.claude/skills/disting-nt-cpp-plugin-writer/reference.md:750-794` - Custom UI functions
  - Pattern Reference: `/Users/nealsanche/.claude/skills/disting-nt-cpp-plugin-writer/examples.md:315-380` - Drawing and UI examples

  **Acceptance Criteria**:
  - [ ] `hasCustomUi()` returns true
  - [ ] `draw()` displays current state text
  - [ ] Pattern shown as dots at correct positions (256x64 display)
  - [ ] Injection bar shows visual activity indicator
  - [ ] Bar counter displayed
  - [ ] `customUi()` handles pot changes for Fuel parameter
  - [ ] UI renders without crashes in nt_emu

  **Commit**: YES
  - Message: `feat: implement custom UI with pattern visualization`
  - Files: `fuel_injector.cpp` (UI section)

---

- [ ] 15. Hardware Build and Final Verification

  **What to do**:
  - Verify all unit tests pass: `make test`
  - Build for hardware: `make hardware`
  - Verify .o file is under 64KB
  - Load in nt_emu and verify:
    - Plugin loads without errors
    - Parameters accessible
    - Clock input works (CV and MIDI)
    - Triggers pass through in LEARNING mode
    - Pattern locks after stable input
    - Injection occurs on injection bars
    - All 6 injection types function
    - Custom UI displays correctly
  - Test 8-channel configuration
  - Measure CPU usage

  **Must NOT do**:
  - No optimization unless .text exceeds 20KB
  - No feature additions during verification

  **Parallelizable**: NO (final task)

  **References**:
  - Build Reference: `/Users/nealsanche/.claude/skills/disting-nt-cpp-plugin-writer/templates/Makefile` - Build targets
  - Size Reference: `/Users/nealsanche/.claude/skills/disting-nt-cpp-plugin-writer/reference.md:1360-1388` - Size optimization
  - Testing Reference: `/Users/nealsanche/.claude/skills/disting-nt-cpp-plugin-writer/testing.md` - nt_emu workflow

  **Acceptance Criteria**:
  - [ ] `make test` → All tests pass (0 failures)
  - [ ] `make hardware` → Produces `plugins/fuel_injector.o`
  - [ ] `.o` file size < 64KB (check with `ls -la plugins/`)
  - [ ] nt_emu: Plugin loads in slot
  - [ ] nt_emu: CV clock advances bar counter
  - [ ] nt_emu: Triggers visible on output busses
  - [ ] nt_emu: State shows LEARNING → LOCKED transition
  - [ ] nt_emu: Injection visible on injection bars
  - [ ] nt_emu: Custom UI renders correctly
  - [ ] nt_emu: 8 channels functional simultaneously

  **Commit**: YES
  - Message: `chore: verify hardware build and nt_emu testing`
  - Files: (verification only, may include minor fixes)

---

## Commit Strategy

| After Task | Message | Files | Verification |
|------------|---------|-------|--------------|
| 0 | `chore: setup Catch2 test infrastructure` | tests/*, Makefile | make test |
| 1 | `feat: define core data structures` | fuel_injector.h, tests/* | make test |
| 2 | `feat: implement CV clock handling` | fuel_injector.cpp, tests/* | make test |
| 3 | `feat: implement MIDI clock handling` | fuel_injector.cpp, tests/* | make test |
| 4 | `feat: implement pattern learning` | fuel_injector.cpp, tests/* | make test |
| 5 | `feat: implement pattern change detection` | fuel_injector.cpp, tests/* | make test |
| 6 | `feat: implement microtiming shift injection` | fuel_injector.cpp, tests/* | make test |
| 7 | `feat: implement hit omission injection` | fuel_injector.cpp, tests/* | make test |
| 8 | `feat: implement hit duplication/roll injection` | fuel_injector.cpp, tests/* | make test |
| 9 | `feat: implement density burst injection` | fuel_injector.cpp, tests/* | make test |
| 10 | `feat: implement sub-bar permutation injection` | fuel_injector.cpp, tests/* | make test |
| 11 | `feat: implement polyrhythmic overlay injection` | fuel_injector.cpp, tests/* | make test |
| 12 | `feat: integrate state machine` | fuel_injector.cpp, tests/* | make test |
| 13 | `feat: implement parameter system` | fuel_injector.cpp, fuel_injector.h | make test |
| 14 | `feat: implement custom UI` | fuel_injector.cpp | make test |
| 15 | `chore: verify hardware build` | (verification) | make hardware |

---

## Success Criteria

### Verification Commands
```bash
# All unit tests pass
make test && ./tests/test_runner
# Expected: All tests passed (X assertions in Y test cases)

# Hardware build succeeds
make hardware
# Expected: plugins/fuel_injector.o created

# Check binary size
ls -la plugins/fuel_injector.o
# Expected: < 65536 bytes

# Check symbols (no undefined references except NT_*)
arm-none-eabi-nm -u plugins/fuel_injector.o
# Expected: Only NT_* and __* symbols
```

### Final Checklist
- [ ] All "Must Have" features present
- [ ] All "Must NOT Have" guardrails respected
- [ ] All 16 tasks completed with passing tests
- [ ] Plugin loads in nt_emu
- [ ] All 6 injection types functional
- [ ] Custom UI displays pattern and state
- [ ] 8-channel configuration works
- [ ] Binary size < 64KB
