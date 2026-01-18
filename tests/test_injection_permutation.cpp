#include "catch.hpp"
#include "../fuel_injector.h"

TEST_CASE("Sub-Bar Permutation Injection", "[injection][permutation]") {
    
    SECTION("generatePermutation creates valid permutation") {
        uint8_t permutation[8];
        XorShift32 rng = {12345};
        uint8_t segment_count = 8;
        
        generatePermutation(permutation, segment_count, &rng);
        
        bool found[8] = {false};
        for (uint8_t i = 0; i < segment_count; i++) {
            REQUIRE(permutation[i] < segment_count);
            found[permutation[i]] = true;
        }
        
        for (uint8_t i = 0; i < segment_count; i++) {
            REQUIRE(found[i] == true);
        }
    }
    
    SECTION("applyPermutationInjection reorders eighth-note segments") {
        bool input_pattern[MAX_TICKS_PER_BAR] = {};
        input_pattern[0] = true;
        input_pattern[24] = true;
        input_pattern[48] = true;
        
        bool output_pattern[MAX_TICKS_PER_BAR] = {};
        
        uint8_t permutation[8] = {1, 0, 2, 3, 4, 5, 6, 7};
        uint16_t ppqn = 48;
        uint16_t pattern_length = 192;
        
        applyPermutationInjection(input_pattern, output_pattern, permutation, ppqn, pattern_length);
        
        REQUIRE(output_pattern[24] == true);
        REQUIRE(output_pattern[0] == true);
        REQUIRE(output_pattern[48] == true);
    }
    
    SECTION("permutation preserves all hits") {
        bool input_pattern[MAX_TICKS_PER_BAR] = {};
        input_pattern[0] = true;
        input_pattern[12] = true;
        input_pattern[24] = true;
        input_pattern[48] = true;
        
        bool output_pattern[MAX_TICKS_PER_BAR] = {};
        
        uint8_t permutation[8] = {2, 1, 0, 3, 4, 5, 6, 7};
        uint16_t ppqn = 48;
        uint16_t pattern_length = 192;
        
        applyPermutationInjection(input_pattern, output_pattern, permutation, ppqn, pattern_length);
        
        int input_count = 0;
        int output_count = 0;
        for (int i = 0; i < pattern_length; i++) {
            if (input_pattern[i]) input_count++;
            if (output_pattern[i]) output_count++;
        }
        
        REQUIRE(input_count == output_count);
    }
    
    SECTION("permutation respects eighth-note granularity") {
        bool input_pattern[MAX_TICKS_PER_BAR] = {};
        input_pattern[0] = true;
        
        bool output_pattern[MAX_TICKS_PER_BAR] = {};
        
        uint8_t permutation[8] = {1, 0, 2, 3, 4, 5, 6, 7};
        uint16_t ppqn = 48;
        uint16_t pattern_length = 192;
        
        applyPermutationInjection(input_pattern, output_pattern, permutation, ppqn, pattern_length);
        
        uint16_t eighth_note = ppqn / 2;
        REQUIRE(output_pattern[eighth_note] == true);
    }
    
    SECTION("permutation respects Fuel scaling") {
        uint8_t permutation[8];
        XorShift32 rng = {12345};
        uint8_t segment_count = 8;
        uint8_t fuel = 0;
        
        bool should_apply = shouldApplyInjection(100, fuel, rng);
        
        REQUIRE(should_apply == false);
    }
    
    SECTION("identity permutation produces same pattern") {
        bool input_pattern[MAX_TICKS_PER_BAR] = {};
        input_pattern[0] = true;
        input_pattern[24] = true;
        input_pattern[48] = true;
        
        bool output_pattern[MAX_TICKS_PER_BAR] = {};
        
        uint8_t permutation[8] = {0, 1, 2, 3, 4, 5, 6, 7};
        uint16_t ppqn = 48;
        uint16_t pattern_length = 192;
        
        applyPermutationInjection(input_pattern, output_pattern, permutation, ppqn, pattern_length);
        
        for (int i = 0; i < pattern_length; i++) {
            REQUIRE(output_pattern[i] == input_pattern[i]);
        }
    }
    
    SECTION("permutation handles full bar") {
        bool input_pattern[MAX_TICKS_PER_BAR] = {};
        for (int i = 0; i < 192; i += 24) {
            input_pattern[i] = true;
        }
        
        bool output_pattern[MAX_TICKS_PER_BAR] = {};
        
        uint8_t permutation[8] = {7, 6, 5, 4, 3, 2, 1, 0};
        uint16_t ppqn = 48;
        uint16_t pattern_length = 192;
        
        applyPermutationInjection(input_pattern, output_pattern, permutation, ppqn, pattern_length);
        
        int output_count = 0;
        for (int i = 0; i < pattern_length; i++) {
            if (output_pattern[i]) output_count++;
        }
        
        REQUIRE(output_count == 8);
    }
}
