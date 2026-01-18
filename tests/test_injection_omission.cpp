#include "catch.hpp"
#include "../fuel_injector.h"

TEST_CASE("Hit Omission Injection", "[injection][omission]") {
    
    SECTION("selectHitsForOmission respects 25% limit") {
        ChannelPattern pattern = {};
        pattern.hit_positions_bar1[0] = 1;
        pattern.hit_positions_bar1[4] = 1;
        pattern.hit_positions_bar1[8] = 1;
        pattern.hit_positions_bar1[12] = 1;
        pattern.hit_count_bar1 = 4;
        
        uint8_t omit_indices[MAX_TICKS_PER_BAR];
        uint8_t omit_count = 0;
        
        XorShift32 rng = {12345};
        uint16_t pattern_length = 16;
        selectHitsForOmission(&pattern, omit_indices, &omit_count, 100, &rng, pattern_length);
        
        REQUIRE(omit_count <= 1);
    }
    
    SECTION("selectHitsForOmission prefers non-downbeat hits") {
        ChannelPattern pattern = {};
        pattern.hit_positions_bar1[0] = 1;
        pattern.hit_positions_bar1[4] = 1;
        pattern.hit_positions_bar1[8] = 1;
        pattern.hit_count_bar1 = 3;
        
        uint8_t omit_indices[MAX_TICKS_PER_BAR];
        uint8_t omit_count = 0;
        
        XorShift32 rng = {12345};
        uint16_t pattern_length = 16;
        selectHitsForOmission(&pattern, omit_indices, &omit_count, 100, &rng, pattern_length);
        
        if (omit_count > 0) {
            REQUIRE(omit_indices[0] != 0);
        }
    }
    
    SECTION("selectHitsForOmission respects probability scaling") {
        ChannelPattern pattern = {};
        pattern.hit_positions_bar1[4] = 1;
        pattern.hit_positions_bar1[8] = 1;
        pattern.hit_positions_bar1[12] = 1;
        pattern.hit_count_bar1 = 3;
        
        uint8_t omit_indices[MAX_TICKS_PER_BAR];
        uint8_t omit_count = 0;
        
        XorShift32 rng = {12345};
        uint16_t pattern_length = 16;
        
        selectHitsForOmission(&pattern, omit_indices, &omit_count, 0, &rng, pattern_length);
        REQUIRE(omit_count == 0);
    }
    
    SECTION("applyOmissionInjection removes selected hits") {
        bool output_pattern[MAX_TICKS_PER_BAR] = {};
        output_pattern[4] = true;
        output_pattern[8] = true;
        output_pattern[12] = true;
        
        uint8_t omit_indices[MAX_TICKS_PER_BAR] = {8};
        uint8_t omit_count = 1;
        
        applyOmissionInjection(output_pattern, omit_indices, omit_count);
        
        REQUIRE(output_pattern[4] == true);
        REQUIRE(output_pattern[8] == false);
        REQUIRE(output_pattern[12] == true);
    }
    
    SECTION("applyOmissionInjection handles multiple omissions") {
        bool output_pattern[MAX_TICKS_PER_BAR] = {};
        output_pattern[4] = true;
        output_pattern[8] = true;
        output_pattern[12] = true;
        
        uint8_t omit_indices[MAX_TICKS_PER_BAR] = {4, 12};
        uint8_t omit_count = 2;
        
        applyOmissionInjection(output_pattern, omit_indices, omit_count);
        
        REQUIRE(output_pattern[4] == false);
        REQUIRE(output_pattern[8] == true);
        REQUIRE(output_pattern[12] == false);
    }
    
    SECTION("omission respects Fuel parameter scaling") {
        ChannelPattern pattern = {};
        pattern.hit_positions_bar1[4] = 1;
        pattern.hit_positions_bar1[8] = 1;
        pattern.hit_positions_bar1[12] = 1;
        pattern.hit_count_bar1 = 3;
        
        uint8_t omit_indices[MAX_TICKS_PER_BAR];
        uint8_t omit_count = 0;
        
        XorShift32 rng = {12345};
        uint16_t pattern_length = 16;
        
        selectHitsForOmission(&pattern, omit_indices, &omit_count, 50, &rng, pattern_length);
        
        REQUIRE(omit_count <= 1);
    }
    
    SECTION("empty pattern produces no omissions") {
        ChannelPattern pattern = {};
        pattern.hit_count_bar1 = 0;
        
        uint8_t omit_indices[MAX_TICKS_PER_BAR];
        uint8_t omit_count = 0;
        
        XorShift32 rng = {12345};
        uint16_t pattern_length = 16;
        selectHitsForOmission(&pattern, omit_indices, &omit_count, 100, &rng, pattern_length);
        
        REQUIRE(omit_count == 0);
    }
}
