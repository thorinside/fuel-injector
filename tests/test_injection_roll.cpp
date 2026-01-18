#include "catch.hpp"
#include "../fuel_injector.h"

TEST_CASE("Hit Duplication / Roll Injection", "[injection][roll]") {
    
    SECTION("selectHitsForRoll selects hits for duplication") {
        ChannelPattern pattern = {};
        pattern.hit_positions_bar1[4] = 1;
        pattern.hit_positions_bar1[8] = 1;
        pattern.hit_positions_bar1[12] = 1;
        pattern.hit_count_bar1 = 3;
        
        uint8_t roll_indices[MAX_TICKS_PER_BAR];
        uint8_t roll_count = 0;
        uint8_t roll_subdivisions[MAX_TICKS_PER_BAR];
        
        XorShift32 rng = {12345};
        uint16_t pattern_length = 16;
        
        selectHitsForRoll(&pattern, roll_indices, &roll_count, roll_subdivisions, 100, &rng, pattern_length);
        
        REQUIRE(roll_count >= 0);
        REQUIRE(roll_count <= 3);
    }
    
    SECTION("applyRollInjection creates double hits") {
        bool output_pattern[MAX_TICKS_PER_BAR] = {};
        output_pattern[8] = true;
        
        uint8_t roll_indices[MAX_TICKS_PER_BAR] = {8};
        uint8_t roll_count = 1;
        uint8_t roll_subdivisions[MAX_TICKS_PER_BAR] = {2};
        uint16_t ppqn = 48;
        
        applyRollInjection(output_pattern, roll_indices, roll_count, roll_subdivisions, ppqn);
        
        uint16_t spacing = ppqn / 2;
        uint16_t second_hit = 8 + spacing;
        
        REQUIRE(output_pattern[8] == true);
        REQUIRE(output_pattern[second_hit] == true);
    }
    
    SECTION("applyRollInjection creates triplet hits") {
        bool output_pattern[MAX_TICKS_PER_BAR] = {};
        output_pattern[12] = true;
        
        uint8_t roll_indices[MAX_TICKS_PER_BAR] = {12};
        uint8_t roll_count = 1;
        uint8_t roll_subdivisions[MAX_TICKS_PER_BAR] = {3};
        uint16_t ppqn = 48;
        
        applyRollInjection(output_pattern, roll_indices, roll_count, roll_subdivisions, ppqn);
        
        uint16_t spacing = ppqn / 3;
        uint16_t second_hit = 12 + spacing;
        uint16_t third_hit = 12 + (spacing * 2);
        
        REQUIRE(output_pattern[12] == true);
        REQUIRE(output_pattern[second_hit] == true);
        REQUIRE(output_pattern[third_hit] == true);
    }
    
    SECTION("applyRollInjection creates ratchet (4 hits)") {
        bool output_pattern[MAX_TICKS_PER_BAR] = {};
        output_pattern[0] = true;
        
        uint8_t roll_indices[MAX_TICKS_PER_BAR] = {0};
        uint8_t roll_count = 1;
        uint8_t roll_subdivisions[MAX_TICKS_PER_BAR] = {4};
        uint16_t ppqn = 48;
        
        applyRollInjection(output_pattern, roll_indices, roll_count, roll_subdivisions, ppqn);
        
        uint16_t spacing = ppqn / 4;
        
        REQUIRE(output_pattern[0] == true);
        REQUIRE(output_pattern[spacing] == true);
        REQUIRE(output_pattern[spacing * 2] == true);
        REQUIRE(output_pattern[spacing * 3] == true);
    }
    
    SECTION("roll subdivisions align to clock ticks") {
        bool output_pattern[MAX_TICKS_PER_BAR] = {};
        output_pattern[0] = true;
        
        uint8_t roll_indices[MAX_TICKS_PER_BAR] = {0};
        uint8_t roll_count = 1;
        uint8_t roll_subdivisions[MAX_TICKS_PER_BAR] = {2};
        uint16_t ppqn = 48;
        
        applyRollInjection(output_pattern, roll_indices, roll_count, roll_subdivisions, ppqn);
        
        uint16_t spacing = ppqn / 2;
        REQUIRE(spacing == 24);
        REQUIRE(output_pattern[24] == true);
    }
    
    SECTION("roll stays within beat boundary") {
        bool output_pattern[MAX_TICKS_PER_BAR] = {};
        output_pattern[44] = true;
        
        uint8_t roll_indices[MAX_TICKS_PER_BAR] = {44};
        uint8_t roll_count = 1;
        uint8_t roll_subdivisions[MAX_TICKS_PER_BAR] = {4};
        uint16_t ppqn = 48;
        
        applyRollInjection(output_pattern, roll_indices, roll_count, roll_subdivisions, ppqn);
        
        uint16_t spacing = ppqn / 4;
        
        REQUIRE(output_pattern[44] == true);
        REQUIRE(output_pattern[44 + spacing] == false);
        REQUIRE(output_pattern[44 + (spacing * 2)] == false);
    }
    
    SECTION("roll respects Fuel scaling") {
        ChannelPattern pattern = {};
        pattern.hit_positions_bar1[4] = 1;
        pattern.hit_positions_bar1[8] = 1;
        pattern.hit_count_bar1 = 2;
        
        uint8_t roll_indices[MAX_TICKS_PER_BAR];
        uint8_t roll_count = 0;
        uint8_t roll_subdivisions[MAX_TICKS_PER_BAR];
        
        XorShift32 rng = {12345};
        uint16_t pattern_length = 16;
        
        selectHitsForRoll(&pattern, roll_indices, &roll_count, roll_subdivisions, 0, &rng, pattern_length);
        
        REQUIRE(roll_count == 0);
    }
    
    SECTION("multiple rolls can be applied") {
        bool output_pattern[MAX_TICKS_PER_BAR] = {};
        output_pattern[0] = true;
        output_pattern[48] = true;
        
        uint8_t roll_indices[MAX_TICKS_PER_BAR] = {0, 48};
        uint8_t roll_count = 2;
        uint8_t roll_subdivisions[MAX_TICKS_PER_BAR] = {2, 3};
        uint16_t ppqn = 48;
        
        applyRollInjection(output_pattern, roll_indices, roll_count, roll_subdivisions, ppqn);
        
        uint16_t spacing_double = ppqn / 2;
        uint16_t spacing_triple = ppqn / 3;
        
        REQUIRE(output_pattern[0 + spacing_double] == true);
        REQUIRE(output_pattern[48 + spacing_triple] == true);
        REQUIRE(output_pattern[48 + (spacing_triple * 2)] == true);
    }
}
