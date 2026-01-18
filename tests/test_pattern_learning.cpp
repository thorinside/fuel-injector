#include "catch.hpp"
#include "../fuel_injector.h"

TEST_CASE("Pattern learning - hit detection", "[pattern_learning]") {
    ChannelPattern pattern = {};
    
    SECTION("Record hit at tick position") {
        recordHit(pattern, 0, 10);
        REQUIRE(pattern.hit_positions_bar1[10] == 1);
    }
    
    SECTION("Multiple hits in same bar") {
        recordHit(pattern, 0, 10);
        recordHit(pattern, 0, 20);
        recordHit(pattern, 0, 30);
        REQUIRE(pattern.hit_count_bar1 == 3);
    }
    
    SECTION("Hits in second bar") {
        recordHit(pattern, 1, 10);
        recordHit(pattern, 1, 20);
        REQUIRE(pattern.hit_count_bar2 == 2);
    }
}

TEST_CASE("Pattern learning - similarity calculation", "[pattern_learning]") {
    ChannelPattern pattern = {};
    
    SECTION("Identical patterns = 100% similarity") {
        recordHit(pattern, 0, 10);
        recordHit(pattern, 0, 20);
        recordHit(pattern, 1, 10);
        recordHit(pattern, 1, 20);
        
        float similarity = calculatePatternSimilarity(pattern);
        REQUIRE(similarity == 100.0f);
    }
    
    SECTION("Completely different patterns = 0% similarity") {
        recordHit(pattern, 0, 10);
        recordHit(pattern, 0, 20);
        recordHit(pattern, 1, 30);
        recordHit(pattern, 1, 40);
        
        float similarity = calculatePatternSimilarity(pattern);
        REQUIRE(similarity == 0.0f);
    }
    
    SECTION("Partial matching patterns") {
        recordHit(pattern, 0, 10);
        recordHit(pattern, 0, 20);
        recordHit(pattern, 1, 10);
        recordHit(pattern, 1, 30);
        
        float similarity = calculatePatternSimilarity(pattern);
        REQUIRE(similarity > 30.0f);
        REQUIRE(similarity < 40.0f);
    }
}

TEST_CASE("Pattern learning - state transitions", "[pattern_learning]") {
    PatternLearner learner = {LEARNING, 0, 2};
    
    SECTION("Remains in LEARNING until stable") {
        learner.stable_bars_count = 0;
        updateLearningState(learner, 85.0f);
        REQUIRE(learner.state == LEARNING);
    }
    
    SECTION("Transitions to LOCKED after stable bars") {
        learner.stable_bars_count = 1;
        updateLearningState(learner, 95.0f);
        REQUIRE(learner.state == LOCKED);
    }
    
    SECTION("Requires >90% similarity for stability") {
        learner.stable_bars_count = 1;
        updateLearningState(learner, 89.0f);
        REQUIRE(learner.state == LEARNING);
        REQUIRE(learner.stable_bars_count == 0);
    }
}

TEST_CASE("Pattern learning - 2 bar history", "[pattern_learning]") {
    ChannelPattern pattern = {};
    
    SECTION("Maintains 2 bars of history") {
        recordHit(pattern, 0, 10);
        recordHit(pattern, 1, 20);
        
        REQUIRE(pattern.hit_count_bar1 > 0);
        REQUIRE(pattern.hit_count_bar2 > 0);
    }
    
    SECTION("Oldest bar discarded on new bar") {
        recordHit(pattern, 0, 10);
        recordHit(pattern, 1, 20);
        
        int old_bar2_count = pattern.hit_count_bar2;
        shiftBarsForNewBar(pattern);
        
        REQUIRE(pattern.hit_count_bar1 == old_bar2_count);
        REQUIRE(pattern.hit_count_bar2 == 0);
    }
}
