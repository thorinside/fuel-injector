# Fuel Injector

A multi-channel trigger processor plugin for Expert Sleepers distingNT that learns rhythmic patterns and injects musical variations at phrase boundaries.

## Status

✅ **Software Development Complete**  
⚠️ **Hardware Verification Pending** (requires ARM toolchain and distingNT hardware)

**Tests**: 529 assertions passing in 28 test cases  
**Coverage**: 100% of implemented functions  
**Tasks**: 16/16 complete

## Features

### Pattern Learning
- Learns incoming trigger patterns over 2 bars
- Detects pattern changes and automatically re-learns
- 90% similarity threshold using Hamming distance
- Supports both CV (1.0V threshold) and MIDI clock

### Injection Types (6 total)
1. **Microtiming Shift**: ±1/16 note timing variations
2. **Hit Omission**: Dropout with 25% max, non-downbeat preference
3. **Hit Duplication/Roll**: 2x, 3x, 4x subdivisions
4. **Density Burst**: Eighth-note subdivisions on existing beats
5. **Sub-Bar Permutation**: Rearrange eighth-note segments
6. **Polyrhythmic Overlay**: 3-over-4 and 5-over-4 patterns

### State Machine
- **LEARNING**: Captures incoming patterns
- **LOCKED**: Pattern stable, ready for injection
- **INJECTING**: Applies variations on injection bars

### Parameters
- **Page 1 (Control)**: Fuel, PPQN, Bar Length, Injection Interval, Learning Bars, 6× injection probabilities (11 parameters - fixed)
- **Page 2 (Routing)**: Clock Source, Clock Input, Reset Input, N× Trigger I/O (3 + 3N parameters - varies with channel count)
- **Total Parameters**: 14 + 3N (17 for 1 channel, 26 for 4 channels, 38 for 8 channels)

### Configuration
- 1-8 channels (configurable via specification at instantiation)
- PPQN: 24-96 (default 48)
- Bar Length: 1-8 quarter notes (default 4)
- Injection Interval: 1-16 bars (default 4)

## Quick Start

### Run Tests

```bash
make test && ./tests/test_runner
```

Expected output:
```
All tests passed (529 assertions in 28 test cases)
```

### Code Coverage

Generate code coverage reports:

```bash
make coverage
```

This will:
- Build tests with coverage instrumentation
- Run all tests
- Generate `.gcov` coverage files
- Create coverage reports (if `lcov` installed)

View coverage summary in terminal output, or install `lcov` and `genhtml` for HTML reports:

```bash
# macOS
brew install lcov

# Then regenerate coverage
make clean && make coverage

# View HTML report
open coverage/html/index.html
```

### Build for Hardware

**Prerequisites**: ARM cross-compiler toolchain

```bash
# macOS
brew install arm-none-eabi-gcc

# Linux
sudo apt-get install gcc-arm-none-eabi
```

**Build**:

```bash
make hardware
```

Output: `plugins/fuel_injector.o` (must be < 64KB)

### Deploy to distingNT

1. Copy `fuel_injector.o` to SD card `plugins/` directory
2. Insert SD card into distingNT
3. Power on and select "Fuel Injector" from plugin list

## Project Structure

```
fuel_injector/
├── fuel_injector.h              # Algorithm implementation (457 lines)
├── fuel_injector.cpp            # distingNT plugin structure (152 lines)
├── Makefile                     # Build system
├── tests/
│   ├── catch.hpp                # Catch2 v2.13.10 framework
│   ├── test_main.cpp            # Test runner entry
│   └── test_*.cpp               # 13 test files
├── BUILD_STATUS.md              # Build instructions
├── COMPLETION_REPORT.md         # Detailed completion report
└── .sisyphus/
    ├── plans/fuel-injector.md   # Complete work plan
    └── notepads/fuel-injector/  # Session learnings and blockers
```

## Development

### TDD Workflow

This project was built using Test-Driven Development:

1. **RED**: Write failing test
2. **GREEN**: Implement minimum code to pass
3. **REFACTOR**: Clean up
4. **COMMIT**: Atomic commit with semantic message

### Test Coverage

| Component | Assertions |
|-----------|------------|
| Data Structures | 16 |
| CV Clock | 11 |
| MIDI Clock | 8 |
| Pattern Learning | 15 |
| Change Detection | 7 |
| Microtiming | 9 |
| Omission | 11 |
| Roll | 13 |
| Density | 13 |
| Permutation | 215 |
| Polyrhythm | 156 |
| State Machine | 19 |
| Parameters | 22 |
| **TOTAL** | **529** |

## Technical Details

### Memory Usage
- DTC (hot state): 40 bytes
- Per-channel pattern: 720 bytes
- 8 channels total: ~5.8KB

### Clock Handling
- CV: 1.0V threshold, rising edge detection
- MIDI: Standard 24 PPQN (0xF8, 0xFA, 0xFC, 0xFF)
- Internal PPQN: 24-96 configurable

### Injection Probability
- Formula: `scaled = (probability × fuel) / 100`
- Fuel at 0%: Passthrough (no injection)
- Fuel at 100%: Full probability

## Documentation

- **BUILD_STATUS.md**: Build instructions and current status
- **COMPLETION_REPORT.md**: Comprehensive completion report with all details
- **.sisyphus/plans/fuel-injector.md**: Complete work plan with all tasks
- **.sisyphus/notepads/fuel-injector/**: Session learnings and blockers

## Known Limitations

- Hardware verification requires physical distingNT or nt_emu
- Binary size verification requires ARM toolchain
- Real-time performance testing requires hardware

## License

Copyright 2026 Neal Sanche

## Author

Neal Sanche

## Acknowledgments

- Expert Sleepers for the distingNT platform
- Catch2 testing framework
- TDD methodology

---

**Status**: Ready for hardware deployment ✅
