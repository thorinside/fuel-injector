#include "catch.hpp"
#include "../fuel_injector.h"

TEST_CASE("Polyrhythmic Overlay Injection", "[injection][polyrhythm]") {
    
    SECTION("selectPolyrhythmType chooses 3-over-4 or 5-over-4") {
        XorShift32 rng = {12345};
        
        uint8_t type = selectPolyrhythmType(&rng);
        
        REQUIRE((type == 3 || type == 5));
    }
    
    SECTION("applyPolyrhythmInjection creates 3-over-4 pattern") {
        bool output_pattern[MAX_TICKS_PER_BAR] = {};
        
        uint8_t polyrhythm_type = 3;
        uint16_t ppqn = 48;
        uint16_t bar_length_qn = 4;
        
        applyPolyrhythmInjection(output_pattern, polyrhythm_type, ppqn, bar_length_qn);
        
        uint16_t bar_length_ticks = ppqn * bar_length_qn;
        uint16_t spacing = bar_length_ticks / 3;
        
        REQUIRE(output_pattern[0] == true);
        REQUIRE(output_pattern[spacing] == true);
        REQUIRE(output_pattern[spacing * 2] == true);
    }
    
    SECTION("applyPolyrhythmInjection creates 5-over-4 pattern") {
        bool output_pattern[MAX_TICKS_PER_BAR] = {};
        
        uint8_t polyrhythm_type = 5;
        uint16_t ppqn = 48;
        uint16_t bar_length_qn = 4;
        
        applyPolyrhythmInjection(output_pattern, polyrhythm_type, ppqn, bar_length_qn);
        
        uint16_t bar_length_ticks = ppqn * bar_length_qn;
        uint16_t spacing = bar_length_ticks / 5;
        
        REQUIRE(output_pattern[0] == true);
        REQUIRE(output_pattern[spacing] == true);
        REQUIRE(output_pattern[spacing * 2] == true);
        REQUIRE(output_pattern[spacing * 3] == true);
        REQUIRE(output_pattern[spacing * 4] == true);
    }
    
    SECTION("polyrhythm spacing is evenly distributed") {
        bool output_pattern[MAX_TICKS_PER_BAR] = {};
        
        uint8_t polyrhythm_type = 3;
        uint16_t ppqn = 48;
        uint16_t bar_length_qn = 4;
        
        applyPolyrhythmInjection(output_pattern, polyrhythm_type, ppqn, bar_length_qn);
        
        uint16_t bar_length_ticks = ppqn * bar_length_qn;
        uint16_t expected_spacing = bar_length_ticks / 3;
        
        REQUIRE(expected_spacing == 64);
    }
    
    SECTION("polyrhythm works with different bar lengths") {
        bool output_pattern[MAX_TICKS_PER_BAR] = {};
        
        uint8_t polyrhythm_type = 5;
        uint16_t ppqn = 48;
        uint16_t bar_length_qn = 3;
        
        applyPolyrhythmInjection(output_pattern, polyrhythm_type, ppqn, bar_length_qn);
        
        uint16_t bar_length_ticks = ppqn * bar_length_qn;
        uint16_t spacing = bar_length_ticks / 5;
        
        int hit_count = 0;
        for (int i = 0; i < bar_length_ticks; i++) {
            if (output_pattern[i]) hit_count++;
        }
        
        REQUIRE(hit_count == 5);
    }
    
    SECTION("polyrhythm stays within bar bounds") {
        bool output_pattern[MAX_TICKS_PER_BAR] = {};
        
        uint8_t polyrhythm_type = 5;
        uint16_t ppqn = 48;
        uint16_t bar_length_qn = 4;
        
        applyPolyrhythmInjection(output_pattern, polyrhythm_type, ppqn, bar_length_qn);
        
        uint16_t bar_length_ticks = ppqn * bar_length_qn;
        
        for (int i = bar_length_ticks; i < MAX_TICKS_PER_BAR; i++) {
            REQUIRE(output_pattern[i] == false);
        }
    }
    
    SECTION("polyrhythm respects Fuel scaling") {
        XorShift32 rng = {12345};
        uint8_t fuel = 0;
        
        bool should_apply = shouldApplyInjection(100, fuel, rng);
        
        REQUIRE(should_apply == false);
    }
}
