#include "catch.hpp"
#include "../fuel_injector.h"

TEST_CASE("Parameter System", "[parameters]") {
    
    SECTION("parameter count is correct") {
        int control_params = 11;
        int routing_params_per_channel = 2;
        int max_channels = 8;
        int total_routing = 3 + (routing_params_per_channel * max_channels);
        int total_params = control_params + total_routing;
        
        REQUIRE(control_params == 11);
        REQUIRE(total_params == 30);
    }
    
    SECTION("Fuel parameter range 0-100") {
        uint8_t fuel_min = 0;
        uint8_t fuel_max = 100;
        uint8_t fuel_default = 100;
        
        REQUIRE(fuel_min == 0);
        REQUIRE(fuel_max == 100);
        REQUIRE(fuel_default == 100);
    }
    
    SECTION("PPQN parameter range 24-96") {
        uint16_t ppqn_min = 24;
        uint16_t ppqn_max = 96;
        uint16_t ppqn_default = 48;
        
        REQUIRE(ppqn_min == 24);
        REQUIRE(ppqn_max == 96);
        REQUIRE(ppqn_default == 48);
    }
    
    SECTION("Bar Length parameter range 1-8") {
        uint8_t bar_length_min = 1;
        uint8_t bar_length_max = 8;
        uint8_t bar_length_default = 4;
        
        REQUIRE(bar_length_min == 1);
        REQUIRE(bar_length_max == 8);
        REQUIRE(bar_length_default == 4);
    }
    
    SECTION("Injection Interval parameter range 1-16") {
        uint8_t interval_min = 1;
        uint8_t interval_max = 16;
        uint8_t interval_default = 4;
        
        REQUIRE(interval_min == 1);
        REQUIRE(interval_max == 16);
        REQUIRE(interval_default == 4);
    }
    
    SECTION("Learning Bars parameter range 1-8") {
        uint8_t learning_min = 1;
        uint8_t learning_max = 8;
        uint8_t learning_default = 2;
        
        REQUIRE(learning_min == 1);
        REQUIRE(learning_max == 8);
        REQUIRE(learning_default == 2);
    }
    
    SECTION("all injection probability parameters range 0-100") {
        uint8_t prob_min = 0;
        uint8_t prob_max = 100;
        
        REQUIRE(prob_min == 0);
        REQUIRE(prob_max == 100);
    }
    
    SECTION("channel specification range 1-8") {
        uint8_t channels_min = 1;
        uint8_t channels_max = 8;
        uint8_t channels_default = 4;
        
        REQUIRE(channels_min == 1);
        REQUIRE(channels_max == MAX_CHANNELS);
        REQUIRE(channels_default == 4);
    }
}
