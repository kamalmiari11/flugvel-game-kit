# Desktop build of the game kit. Needs a C++17 compiler and nothing else -
# no SDL, no Python, no package installs. `make run` and look at a browser.

CXX      ?= g++
CXXFLAGS ?= -std=c++17 -O2 -g -Wall -Wextra -Wshadow
INCLUDES  = -Iinclude -Isim -Igames
BUILD     = build
OBJ       = $(BUILD)/obj
BIN       = $(BUILD)/sim
TESTBIN   = $(BUILD)/tests

GAME ?= welcome      # first in the registry: the start screen
PORT ?= 8080

KIT_SRC  = $(wildcard src/gamekit/*.cpp)
GAME_SRC = $(wildcard games/*.cpp) $(wildcard games/*/*.cpp)
SIM_SRC  = $(wildcard sim/*.cpp)
TEST_SRC = $(wildcard tests/*.cpp)

SIM_OBJ  = $(patsubst %.cpp,$(OBJ)/%.o,$(KIT_SRC) $(GAME_SRC) $(SIM_SRC))
# The tests bring their own main(), so the simulator's is left out of them.
TEST_OBJ = $(patsubst %.cpp,$(OBJ)/%.o,$(KIT_SRC) $(GAME_SRC) $(filter-out sim/main.cpp,$(SIM_SRC)) $(TEST_SRC))

.PHONY: all run sim test check lint replay clean help

help:
	@echo "make run            build and open the simulator on http://127.0.0.1:$(PORT)/"
	@echo "make sim            build $(BIN) only"
	@echo "make test           build and run the test bench"
	@echo "make lint           check the game sources against the house rules"
	@echo "make replay         replay scripts/demo.input and write a filmstrip"
	@echo "                    (try GAME=gaterun - the start screen barely moves)"
	@echo "make check          lint + test + a strict replay: what CI runs"
	@echo ""
	@echo "GAME=$(GAME)   PORT=$(PORT)   (override on the command line)"

all: sim

sim: $(BIN)

$(BIN): $(SIM_OBJ)
	@mkdir -p $(dir $@)
	$(CXX) $(CXXFLAGS) $^ -o $@

$(TESTBIN): $(TEST_OBJ)
	@mkdir -p $(dir $@)
	$(CXX) $(CXXFLAGS) $^ -o $@

# The simulator's browser page is embedded in the binary. Regenerate the
# header when the HTML changes - and carry on without it if Python is not
# around, since the generated header is committed.
sim/page_html.h: sim/page.html
	@python3 tools/embed_page.py 2>/dev/null \
	  || echo "note: sim/page.html changed but python3 is missing - sim/page_html.h left as it was"

$(OBJ)/sim/Server.o: sim/page_html.h

$(OBJ)/%.o: %.cpp
	@mkdir -p $(dir $@)
	$(CXX) $(CXXFLAGS) $(INCLUDES) -MMD -MP -c $< -o $@

run: $(BIN)
	$(BIN) --game $(GAME) --port $(PORT)

test: $(TESTBIN)
	$(TESTBIN)

lint:
	@sh tests/lint.sh

replay: $(BIN)
	$(BIN) --game $(GAME) --script scripts/demo.input --duration 20000 \
	       --record $(BUILD)/run.html --shot $(BUILD)/final.bmp
	@echo "open $(BUILD)/run.html in a browser"

check: lint test $(BIN)
	@for g in $$($(BIN) --list | awk '{print $$1}'); do \
	  echo "--- strict replay: $$g"; \
	  $(BIN) --game $$g --script scripts/demo.input --duration 20000 --strict || exit 1; \
	done
	@echo "all checks passed"

clean:
	rm -rf $(BUILD)

-include $(shell find $(OBJ) -name '*.d' 2>/dev/null)
