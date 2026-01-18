#include "catch.hpp"
#include "../fuel_injector.h"

TEST_CASE("Microtiming shift - range calculation", "[injection_microtiming]") {
    SECTION("Shift range at 48 PPQN = +/- 12 ticks") {
        int shift_range = calculateMicrotimingRange(48);
        REQUIRE(shift_range == 12);
    }
    
    SECTION("Shift range at 24 PPQN = +/- 6 ticks") {
        int shift_range = calculateMicrotimingRange(24);
        REQUIRE(shift_range == 6);
    }
}

TEST_CASE("Microtiming shift - collision prevention", "[injection_microtiming]") {
    SECTION("Shifted hits maintain minimum 1 tick separation") {
        int pos1 = 10;
        int pos2 = 11;
        int shift1 = 2;
        
        int new_pos1 = applyMicrotimingShift(pos1, shift1, pos2);
        int separation = (new_pos1 > pos2) ? (new_pos1 - pos2) : (pos2 - new_pos1);
        REQUIRE(separation >= 1);
    }
    
    SECTION("No collision when shifting away") {
        int pos1 = 10;
        int pos2 = 15;
        int shift1 = -3;
        
        int new_pos1 = applyMicrotimingShift(pos1, shift1, pos2);
        REQUIRE(new_pos1 == 7);
    }
}

TEST_CASE("Microtiming shift - probability and fuel", "[injection_microtiming]") {
    XorShift32 rng = {12345};
    
    SECTION("Probability 0% = no shift") {
        bool should_shift = shouldApplyInjection(0, 100, rng);
        REQUIRE(should_shift == false);
    }
    
    SECTION("Probability 100%, Fuel 100% = always shift") {
        bool should_shift = shouldApplyInjection(100, 100, rng);
        REQUIRE(should_shift == true);
    }
    
    SECTION("Fuel 0% = no shift regardless of probability") {
        bool should_shift = shouldApplyInjection(100, 0, rng);
        REQUIRE(should_shift == false);
    }
    
    SECTION("Fuel 50% scales probability") {
        int shift_count = 0;
        for (int i = 0; i < 100; i++) {
            XorShift32 test_rng = {static_cast<uint32_t>(12345 + i)};
            if (shouldApplyInjection(100, 50, test_rng)) {
                shift_count++;
            }
        }
        REQUIRE(shift_count > 30);
        REQUIRE(shift_count < 70);
    }
}
