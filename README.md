# Fuel Injector (distingNT plugin)

Fuel Injector is a multi-channel trigger processor for Expert Sleepers distingNT. It learns a stable incoming groove per channel and, at a configurable interval, outputs an injected one-bar variation while passing the original triggers through unchanged the rest of the time.

## How It Works

- **Clocked** by an external **CV clock** (rising edge ≥ 1.0V).
- **Reset** defines bar 1 and aligns the injection schedule to the last reset.
- **Learning**: records incoming trigger edges on the clock grid; once the pattern is stable for `Learn Bars`, it becomes the learned pattern.
- **Locked**: continues pass-through and monitors for pattern changes; if the incoming pattern changes, it automatically re-learns.
- **Injection bars**: every `Inj Interval` bars (counting from the most recent reset), Fuel Injector outputs a modified version of the learned pattern for exactly one bar.

## Output Behavior

- **Normal bars (non-injection)**: pass-through preserves the incoming gate length and voltage.
- **Injection bars**: output is generated as **5V trigger pulses** (≈10ms max, clamped to ≤ 1/2 clock period so pulses always return to 0V).

## Parameters

### Control Page

- **Fuel** (0–100%): master intensity; at 0% the plugin is effectively a pass-through.
- **PPQN**: 1, 2, 4, 8, 16, 24, 48 (clock ticks per quarter note).
- **Bar Length**: 1–8 quarter notes.
- **Inj Interval**: 1–16 bars (relative to the most recent reset).
- **Learn Bars**: 1–8 bars (how long the pattern must remain stable before locking).
- **P:Microtiming**: shifts a subset of hits earlier/later (subtle at low values, wider/more frequent at high).
- **P:Omission**: removes a small number of hits (prefers non-downbeats).
- **P:Roll**: adds ratchets (mostly 2x at low values; 3x/4x appear at higher values).
- **P:Density**: adds extra eighth-note hits on selected beats.
- **P:Permutation**: reorders eighth-note segments (kept subtle unless probability is high).
- **P:Polyrhythm**: overlays a small number of evenly-spaced hits (only applies at higher probability).

### Routing Page

- **Clock Source**: CV (MIDI reserved; not implemented in this version).
- **Clock Input**: CV input bus for the clock.
- **Reset Input**: CV input bus for reset.
- Per-channel:
  - **Trig In**: incoming trigger/gate.
  - **Trig Out**: output bus.
  - **Trig Out mode**: Replace/Add.

## Patching Notes

- Use a clean clock into **Clock Input** and send a reset pulse to **Reset Input** when you want the injection schedule to restart from bar 1.
- If you set **Trig Out mode = Add**, avoid mapping **Trig Out** to the same bus as **Trig In** (otherwise you can sum voltages during injection bars).

## Build & Install

Run tests:

```bash
make test && ./tests/test_runner
```

Build the plugin object for distingNT:

```bash
make hardware
```

Deploy:

1. Copy `plugins/fuel_injector.o` to the SD card at `/programs/plug-ins/fuel_injector.o`.
2. Insert SD card into distingNT and reboot.
3. Select **Fuel Injector** from the plugin list.

Tip: GitHub Releases include a zip that already contains `programs/plug-ins/fuel_injector.o` — unzip it to the SD card root.

## Troubleshooting

- **No injection happens**: check Fuel > 0, Clock Source = CV, a clock is connected, and reset isn’t being retriggered accidentally.
- **Injections are too strong/too subtle**: start by adjusting Fuel, then the individual injection probabilities.
- **Output looks “stuck high” on injection bars**: check your clock rate vs. the drum module’s input; generated triggers are short pulses and should return to 0V.

## License

Copyright 2026 Neal Sanche
