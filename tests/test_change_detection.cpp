#include "catch.hpp"
#include "../fuel_injector.h"

TEST_CASE("Pattern change detection - missing hits", "[change_detection]") {
    ChannelPattern learned = {};
    ChannelPattern incoming = {};
    
    SECTION("Missing expected hit triggers relearn") {
        recordHit(learned, 0, 10);
        recordHit(learned, 0, 20);
        recordHit(incoming, 0, 10);
        
        bool changed = detectPatternChange(learned, incoming);
        REQUIRE(changed == true);
    }
    
    SECTION("All hits present = no change") {
        recordHit(learned, 0, 10);
        recordHit(learned, 0, 20);
        recordHit(incoming, 0, 10);
        recordHit(incoming, 0, 20);
        
        bool changed = detectPatternChange(learned, incoming);
        REQUIRE(changed == false);
    }
}

TEST_CASE("Pattern change detection - new hits", "[change_detection]") {
    ChannelPattern learned = {};
    ChannelPattern incoming = {};
    
    SECTION("New unexpected hit triggers relearn") {
        recordHit(learned, 0, 10);
        recordHit(incoming, 0, 10);
        recordHit(incoming, 0, 30);
        
        bool changed = detectPatternChange(learned, incoming);
        REQUIRE(changed == true);
    }
}

TEST_CASE("Pattern change detection - density change", "[change_detection]") {
    ChannelPattern learned = {};
    ChannelPattern incoming = {};
    
    SECTION(">10% density change triggers relearn") {
        for (int i = 0; i < 10; i++) {
            recordHit(learned, 0, i * 10);
        }
        for (int i = 0; i < 8; i++) {
            recordHit(incoming, 0, i * 10);
        }
        
        bool changed = detectPatternChange(learned, incoming);
        REQUIRE(changed == true);
    }
    
    SECTION("<=10% density change = no change") {
        for (int i = 0; i < 10; i++) {
            recordHit(learned, 0, i * 10);
        }
        for (int i = 0; i < 9; i++) {
            recordHit(incoming, 0, i * 10);
        }
        
        bool changed = detectPatternChange(learned, incoming);
        REQUIRE(changed == false);
    }
}

TEST_CASE("Pattern change detection - state transition", "[change_detection]") {
    PatternLearner learner = {LOCKED, 2, 2};
    
    SECTION("Change detected transitions to LEARNING") {
        handlePatternChange(learner);
        REQUIRE(learner.state == LEARNING);
        REQUIRE(learner.stable_bars_count == 0);
    }
}
