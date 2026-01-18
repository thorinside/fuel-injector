#include "catch.hpp"
#include "../fuel_injector.h"

TEST_CASE("MIDI Clock message handling", "[midi_clock]") {
    MidiClockState state = {0, false};
    
    SECTION("0xF8 increments tick counter") {
        handleMidiRealtime(0xF8, state);
        REQUIRE(state.midi_tick_count == 1);
        handleMidiRealtime(0xF8, state);
        REQUIRE(state.midi_tick_count == 2);
    }
    
    SECTION("0xFA Start resets and enables") {
        state.midi_tick_count = 100;
        handleMidiRealtime(0xFA, state);
        REQUIRE(state.midi_tick_count == 0);
        REQUIRE(state.midi_running == true);
    }
    
    SECTION("0xFC Stop disables clock") {
        state.midi_running = true;
        handleMidiRealtime(0xFC, state);
        REQUIRE(state.midi_running == false);
    }
    
    SECTION("0xFF Reset clears tick count") {
        state.midi_tick_count = 100;
        handleMidiRealtime(0xFF, state);
        REQUIRE(state.midi_tick_count == 0);
    }
}

TEST_CASE("MIDI PPQN conversion", "[midi_clock]") {
    SECTION("24 MIDI ticks = 1 quarter note") {
        REQUIRE(convertMidiTicksToInternal(24, 24) == 24);
    }
    
    SECTION("24 MIDI ticks = 48 internal ticks at 48 PPQN") {
        REQUIRE(convertMidiTicksToInternal(24, 48) == 48);
    }
    
    SECTION("48 MIDI ticks = 96 internal ticks at 48 PPQN") {
        REQUIRE(convertMidiTicksToInternal(48, 48) == 96);
    }
    
    SECTION("12 MIDI ticks = 24 internal ticks at 48 PPQN") {
        REQUIRE(convertMidiTicksToInternal(12, 48) == 24);
    }
}
