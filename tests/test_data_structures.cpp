#include "catch.hpp"
#include "../fuel_injector.h"

TEST_CASE("FuelInjector data structure sizes", "[structures]") {
    SECTION("DTC struct fits in 1KB") {
        REQUIRE(sizeof(_FuelInjector_DTC) <= 1024);
    }
    
    SECTION("ChannelPattern fits in 2KB") {
        REQUIRE(sizeof(ChannelPattern) <= 2048);
    }
    
    SECTION("8 channels fit in 16KB SRAM") {
        REQUIRE(sizeof(ChannelPattern) * 8 <= 16384);
    }
}

TEST_CASE("FuelInjector enums", "[enums]") {
    SECTION("FuelInjectorState enum values") {
        FuelInjectorState state = LEARNING;
        REQUIRE((state == LEARNING || state == LOCKED || state == INJECTING));
    }
    
    SECTION("ClockSource enum values") {
        ClockSource source = CV;
        REQUIRE((source == CV || source == MIDI));
    }
    
    SECTION("InjectionType enum has 6 types") {
        REQUIRE(static_cast<int>(INJECTION_TYPE_COUNT) == 6);
    }
}

TEST_CASE("FuelInjector constants", "[constants]") {
    REQUIRE(MAX_CHANNELS == 8);
    REQUIRE(MAX_PPQN == 48);
    REQUIRE(MAX_TICKS_PER_BAR == 336);  // 48 PPQN × 7 beats max
}

TEST_CASE("XorShift32 PRNG", "[prng]") {
    XorShift32 rng = {12345};
    uint32_t val1 = rng.next();
    uint32_t val2 = rng.next();
    uint32_t val3 = rng.next();
    
    SECTION("Produces different values") {
        REQUIRE(val1 != val2);
        REQUIRE(val2 != val3);
        REQUIRE(val1 != val3);
    }
    
    SECTION("Produces non-zero values") {
        REQUIRE(val1 != 0);
        REQUIRE(val2 != 0);
        REQUIRE(val3 != 0);
    }
}
