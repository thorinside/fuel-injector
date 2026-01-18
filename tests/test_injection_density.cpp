#include "catch.hpp"
#include "../fuel_injector.h"

TEST_CASE("Density Burst Injection", "[injection][density]") {
    
    SECTION("selectBeatsForDensityBurst selects existing beats") {
        ChannelPattern pattern = {};
        pattern.hit_positions_bar1[0] = 1;
        pattern.hit_positions_bar1[48] = 1;
        pattern.hit_positions_bar1[96] = 1;
        pattern.hit_count_bar1 = 3;
        
        uint8_t burst_beat_indices[MAX_TICKS_PER_BAR];
        uint8_t burst_count = 0;
        
        XorShift32 rng = {12345};
        uint16_t pattern_length = 192;
        uint16_t ppqn = 48;
        
        selectBeatsForDensityBurst(&pattern, burst_beat_indices, &burst_count, 100, &rng, pattern_length, ppqn);
        
        REQUIRE(burst_count >= 0);
        REQUIRE(burst_count <= 3);
    }
    
    SECTION("applyDensityBurstInjection adds subdivision hits") {
        bool output_pattern[MAX_TICKS_PER_BAR] = {};
        output_pattern[0] = true;
        
        uint8_t burst_beat_indices[MAX_TICKS_PER_BAR] = {0};
        uint8_t burst_count = 1;
        uint16_t ppqn = 48;
        
        applyDensityBurstInjection(output_pattern, burst_beat_indices, burst_count, ppqn);
        
        REQUIRE(output_pattern[0] == true);
        REQUIRE(output_pattern[ppqn / 2] == true);
    }
    
    SECTION("density burst creates eighth note subdivisions") {
        bool output_pattern[MAX_TICKS_PER_BAR] = {};
        output_pattern[48] = true;
        
        uint8_t burst_beat_indices[MAX_TICKS_PER_BAR] = {1};
        uint8_t burst_count = 1;
        uint16_t ppqn = 48;
        
        applyDensityBurstInjection(output_pattern, burst_beat_indices, burst_count, ppqn);
        
        uint16_t beat_start = 48;
        uint16_t eighth_spacing = ppqn / 2;
        
        REQUIRE(output_pattern[beat_start] == true);
        REQUIRE(output_pattern[beat_start + eighth_spacing] == true);
    }
    
    SECTION("density burst only affects selected beats") {
        bool output_pattern[MAX_TICKS_PER_BAR] = {};
        output_pattern[0] = true;
        output_pattern[48] = true;
        output_pattern[96] = true;
        
        uint8_t burst_beat_indices[MAX_TICKS_PER_BAR] = {1};
        uint8_t burst_count = 1;
        uint16_t ppqn = 48;
        
        applyDensityBurstInjection(output_pattern, burst_beat_indices, burst_count, ppqn);
        
        REQUIRE(output_pattern[24] == false);
        REQUIRE(output_pattern[72] == true);
        REQUIRE(output_pattern[120] == false);
    }
    
    SECTION("density burst respects Fuel scaling") {
        ChannelPattern pattern = {};
        pattern.hit_positions_bar1[0] = 1;
        pattern.hit_positions_bar1[48] = 1;
        pattern.hit_count_bar1 = 2;
        
        uint8_t burst_beat_indices[MAX_TICKS_PER_BAR];
        uint8_t burst_count = 0;
        
        XorShift32 rng = {12345};
        uint16_t pattern_length = 96;
        uint16_t ppqn = 48;
        
        selectBeatsForDensityBurst(&pattern, burst_beat_indices, &burst_count, 0, &rng, pattern_length, ppqn);
        
        REQUIRE(burst_count == 0);
    }
    
    SECTION("empty pattern produces no density burst") {
        ChannelPattern pattern = {};
        pattern.hit_count_bar1 = 0;
        
        uint8_t burst_beat_indices[MAX_TICKS_PER_BAR];
        uint8_t burst_count = 0;
        
        XorShift32 rng = {12345};
        uint16_t pattern_length = 192;
        uint16_t ppqn = 48;
        
        selectBeatsForDensityBurst(&pattern, burst_beat_indices, &burst_count, 100, &rng, pattern_length, ppqn);
        
        REQUIRE(burst_count == 0);
    }
    
    SECTION("density burst stays within pattern bounds") {
        bool output_pattern[MAX_TICKS_PER_BAR] = {};
        output_pattern[144] = true;
        
        uint8_t burst_beat_indices[MAX_TICKS_PER_BAR] = {3};
        uint8_t burst_count = 1;
        uint16_t ppqn = 48;
        
        applyDensityBurstInjection(output_pattern, burst_beat_indices, burst_count, ppqn);
        
        uint16_t subdivision_pos = 144 + (ppqn / 2);
        REQUIRE(subdivision_pos < MAX_TICKS_PER_BAR);
        REQUIRE(output_pattern[subdivision_pos] == true);
    }
}
