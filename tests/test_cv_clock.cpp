#include "catch.hpp"
#include "../fuel_injector.h"

TEST_CASE("CV Clock rising edge detection", "[cv_clock]") {
    SECTION("Rising edge at 1.0V triggers") {
        float prev = 0.5f;
        float current = 1.5f;
        REQUIRE(detectRisingEdge(current, prev, 1.0f) == true);
    }
    
    SECTION("No false trigger at 0.9V") {
        float prev = 0.5f;
        float current = 0.9f;
        REQUIRE(detectRisingEdge(current, prev, 1.0f) == false);
    }
    
    SECTION("No trigger on falling edge") {
        float prev = 1.5f;
        float current = 0.5f;
        REQUIRE(detectRisingEdge(current, prev, 1.0f) == false);
    }
    
    SECTION("No trigger when already high") {
        float prev = 1.5f;
        float current = 2.0f;
        REQUIRE(detectRisingEdge(current, prev, 1.0f) == false);
    }
}

TEST_CASE("CV Clock tick and bar calculations", "[cv_clock]") {
    SECTION("Tick counter increments") {
        int ticks = 0;
        ticks = incrementTick(ticks);
        REQUIRE(ticks == 1);
        ticks = incrementTick(ticks);
        REQUIRE(ticks == 2);
    }
    
    SECTION("Bar position calculation") {
        int ppqn = 48;
        int bar_length_qn = 4;
        
        REQUIRE(calculateBarPosition(0, ppqn, bar_length_qn) == 0);
        REQUIRE(calculateBarPosition(48, ppqn, bar_length_qn) == 48);
        REQUIRE(calculateBarPosition(192, ppqn, bar_length_qn) == 0);
        REQUIRE(calculateBarPosition(240, ppqn, bar_length_qn) == 48);
    }
    
    SECTION("Bar number calculation") {
        int ppqn = 48;
        int bar_length_qn = 4;
        
        REQUIRE(calculateBarNumber(0, ppqn, bar_length_qn) == 0);
        REQUIRE(calculateBarNumber(192, ppqn, bar_length_qn) == 1);
        REQUIRE(calculateBarNumber(384, ppqn, bar_length_qn) == 2);
    }
}

TEST_CASE("CV Clock timeout detection", "[cv_clock]") {
    const int TIMEOUT_SAMPLES = 96000;
    
    SECTION("No timeout before threshold") {
        REQUIRE(isClockTimeout(95999, TIMEOUT_SAMPLES) == false);
    }
    
    SECTION("Timeout at threshold") {
        REQUIRE(isClockTimeout(96000, TIMEOUT_SAMPLES) == true);
    }
    
    SECTION("Timeout after threshold") {
        REQUIRE(isClockTimeout(100000, TIMEOUT_SAMPLES) == true);
    }
}
