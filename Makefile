PLUGIN_NAME = fuel_injector
SOURCES = fuel_injector.cpp
TEST_SOURCES = tests/test_main.cpp tests/test_example.cpp tests/test_data_structures.cpp tests/test_cv_clock.cpp tests/test_midi_clock.cpp tests/test_pattern_learning.cpp tests/test_change_detection.cpp tests/test_injection_microtiming.cpp tests/test_injection_omission.cpp tests/test_injection_roll.cpp tests/test_injection_density.cpp tests/test_injection_permutation.cpp tests/test_injection_polyrhythm.cpp tests/test_state_machine.cpp tests/test_parameters.cpp
TEST_RUNNER = tests/test_runner

UNAME_S := $(shell uname -s)
TARGET ?= hardware

ifeq ($(TARGET),hardware)
    CXX = arm-none-eabi-g++
    CFLAGS = -std=c++11 \
             -mcpu=cortex-m7 \
             -mfpu=fpv5-d16 \
             -mfloat-abi=hard \
             -mthumb \
             -Os \
             -ffunction-sections \
             -fdata-sections \
             -fno-rtti \
             -fno-exceptions \
             -fno-unwind-tables \
             -fno-asynchronous-unwind-tables \
             -Wall
    INCLUDES = -I. -I./distingNT_API/include
    LDFLAGS = -Wl,--relocatable -nostdlib
    OUTPUT_DIR = plugins
    BUILD_DIR = build
    OUTPUT = $(OUTPUT_DIR)/$(PLUGIN_NAME).o
    OBJECTS = $(patsubst %.cpp, $(BUILD_DIR)/%.o, $(SOURCES))
    CHECK_CMD = arm-none-eabi-nm $(OUTPUT) | grep ' U '
    SIZE_CMD = arm-none-eabi-size -A $(OUTPUT)

else ifeq ($(TARGET),test)
    ifeq ($(UNAME_S),Darwin)
        CXX = clang++
        CFLAGS = -std=c++11 -fPIC -Os -Wall -fno-rtti -fno-exceptions
        LDFLAGS = -dynamiclib -undefined dynamic_lookup
        EXT = dylib
    endif

    ifeq ($(UNAME_S),Linux)
        CXX = g++
        CFLAGS = -std=c++11 -fPIC -Os -Wall -fno-rtti -fno-exceptions
        LDFLAGS = -shared
        EXT = so
    endif

    ifeq ($(OS),Windows_NT)
        CXX = g++
        CFLAGS = -std=c++11 -fPIC -Os -Wall -fno-rtti -fno-exceptions
        LDFLAGS = -shared
        EXT = dll
    endif

    INCLUDES = -I. -I./distingNT_API/include
    OUTPUT_DIR = plugins
    OUTPUT = $(OUTPUT_DIR)/$(PLUGIN_NAME).$(EXT)
    CHECK_CMD = nm $(OUTPUT) | grep ' U ' || true
    SIZE_CMD = ls -lh $(OUTPUT)
endif

all: $(OUTPUT)

ifeq ($(TARGET),hardware)
$(OUTPUT): $(OBJECTS)
	@mkdir -p $(OUTPUT_DIR)
	$(CXX) $(CFLAGS) $(LDFLAGS) -o $@ $^
	@echo "Built: $@"
	@$(SIZE_CMD)

$(BUILD_DIR)/%.o: %.cpp | $(BUILD_DIR)
	$(CXX) $(CFLAGS) $(INCLUDES) -c -o $@ $<

$(BUILD_DIR):
	@mkdir -p $(BUILD_DIR)

else ifeq ($(TARGET),test)
$(OUTPUT): $(SOURCES)
	@mkdir -p $(OUTPUT_DIR)
	$(CXX) $(CFLAGS) $(INCLUDES) $(LDFLAGS) -o $@ $(SOURCES)
	@echo "Built: $@"
endif

hardware:
	@$(MAKE) TARGET=hardware

test: $(TEST_RUNNER)
	@echo "Test runner built successfully"

$(TEST_RUNNER): $(TEST_SOURCES)
	@mkdir -p tests
	g++ -std=c++11 -Wall -I. -o $(TEST_RUNNER) $(TEST_SOURCES)

coverage: $(TEST_SOURCES)
	@echo "Building with coverage..."
	@mkdir -p tests coverage
	g++ -std=c++11 -Wall -I. --coverage -fprofile-arcs -ftest-coverage -o $(TEST_RUNNER) $(TEST_SOURCES)
	@echo "Running tests..."
	./$(TEST_RUNNER)
	@echo "Generating coverage report..."
	@gcov $(TEST_SOURCES) -o . 2>/dev/null | grep -A 3 "fuel_injector.h" || echo "Coverage data generated"
	@lcov --capture --directory . --output-file coverage/coverage.info --no-external 2>/dev/null || echo "lcov not installed (optional)"
	@genhtml coverage/coverage.info --output-directory coverage/html 2>/dev/null || echo "genhtml not installed (optional)"
	@echo ""
	@echo "Coverage summary:"
	@gcov fuel_injector.h -o . 2>/dev/null | grep -E "Lines executed|fuel_injector.h" || echo "Run 'gcov *.gcno' to see detailed coverage"
	@echo ""
	@echo "Coverage files generated in coverage/"
	@echo "View detailed HTML report: open coverage/html/index.html"

both: hardware test

check: $(OUTPUT)
	@$(CHECK_CMD)

size: $(OUTPUT)
	@$(SIZE_CMD)

clean:
	rm -rf $(BUILD_DIR) $(OUTPUT_DIR) $(TEST_RUNNER) coverage *.gcov *.gcda *.gcno

.PHONY: all hardware test both check size clean coverage
