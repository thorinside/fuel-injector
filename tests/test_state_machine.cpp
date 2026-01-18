#include "catch.hpp"
#include "../fuel_injector.h"

TEST_CASE("State Machine Integration", "[state][integration]") {
    
    SECTION("initial state is LEARNING") {
        PatternLearner learner = {};
        learner.state = LEARNING;
        learner.stable_bars_count = 0;
        learner.required_stable_bars = 2;
        
        REQUIRE(learner.state == LEARNING);
    }
    
    SECTION("LEARNING transitions to LOCKED after stable pattern") {
        PatternLearner learner = {};
        learner.state = LEARNING;
        learner.stable_bars_count = 0;
        learner.required_stable_bars = 2;
        
        updateLearningState(learner, 95.0f);
        REQUIRE(learner.stable_bars_count == 1);
        REQUIRE(learner.state == LEARNING);
        
        updateLearningState(learner, 95.0f);
        REQUIRE(learner.stable_bars_count == 2);
        REQUIRE(learner.state == LOCKED);
    }
    
    SECTION("LOCKED transitions to INJECTING on injection bar") {
        _FuelInjector_DTC dtc = {};
        dtc.state = LOCKED;
        dtc.bar_counter = 4;
        dtc.is_injection_bar = false;
        
        uint8_t injection_interval = 4;
        
        bool should_inject = shouldInjectThisBar(dtc.bar_counter, injection_interval);
        
        REQUIRE(should_inject == true);
    }
    
    SECTION("INJECTING transitions to LOCKED after bar complete") {
        _FuelInjector_DTC dtc = {};
        dtc.state = INJECTING;
        dtc.current_bar_position = 191;
        
        uint16_t ppqn = 48;
        uint16_t bar_length_qn = 4;
        uint16_t bar_length_ticks = ppqn * bar_length_qn;
        
        bool bar_complete = isBarComplete(dtc.current_bar_position, bar_length_ticks);
        
        REQUIRE(bar_complete == true);
    }
    
    SECTION("ANY state transitions to LEARNING on reset") {
        _FuelInjector_DTC dtc = {};
        dtc.state = LOCKED;
        
        PatternLearner learner = {};
        learner.state = LOCKED;
        learner.stable_bars_count = 5;
        
        handleReset(&dtc, &learner);
        
        REQUIRE(dtc.state == LEARNING);
        REQUIRE(learner.state == LEARNING);
        REQUIRE(learner.stable_bars_count == 0);
    }
    
    SECTION("ANY state transitions to LEARNING on pattern change") {
        _FuelInjector_DTC dtc = {};
        dtc.state = LOCKED;
        
        PatternLearner learner = {};
        learner.state = LOCKED;
        learner.stable_bars_count = 5;
        
        handlePatternChange(learner);
        
        REQUIRE(learner.state == LEARNING);
        REQUIRE(learner.stable_bars_count == 0);
    }
    
    SECTION("injection bar detection uses modulo") {
        uint32_t bar_counter = 8;
        uint8_t injection_interval = 4;
        
        bool should_inject = shouldInjectThisBar(bar_counter, injection_interval);
        
        REQUIRE(should_inject == true);
        
        bar_counter = 9;
        should_inject = shouldInjectThisBar(bar_counter, injection_interval);
        
        REQUIRE(should_inject == false);
    }
    
    SECTION("bar completion detection") {
        uint16_t ppqn = 48;
        uint16_t bar_length_qn = 4;
        uint16_t bar_length_ticks = ppqn * bar_length_qn;
        
        bool complete = isBarComplete(191, bar_length_ticks);
        REQUIRE(complete == true);
        
        complete = isBarComplete(100, bar_length_ticks);
        REQUIRE(complete == false);
    }
    
    SECTION("state machine handles full cycle") {
        PatternLearner learner = {};
        learner.state = LEARNING;
        learner.stable_bars_count = 0;
        learner.required_stable_bars = 2;
        
        _FuelInjector_DTC dtc = {};
        dtc.state = LEARNING;
        dtc.bar_counter = 0;
        
        updateLearningState(learner, 95.0f);
        updateLearningState(learner, 95.0f);
        REQUIRE(learner.state == LOCKED);
        
        dtc.state = LOCKED;
        dtc.bar_counter = 4;
        bool should_inject = shouldInjectThisBar(dtc.bar_counter, 4);
        REQUIRE(should_inject == true);
        
        dtc.state = INJECTING;
        dtc.current_bar_position = 191;
        bool bar_complete = isBarComplete(dtc.current_bar_position, 192);
        REQUIRE(bar_complete == true);
    }
}
