CXX ?= g++
CXXFLAGS ?= -std=c++17 -O2 -g -Wall -Wextra -Wpedantic
CPPFLAGS += -Isrc -Ithird_party/tinyxml2
LDLIBS += -lglut -lGL

CORE = src/config.cpp src/math.cpp src/game.cpp src/controller.cpp third_party/tinyxml2/tinyxml2.cpp
SOURCES = $(CORE) src/renderer.cpp src/main.cpp
OBJECTS = $(patsubst %.cpp,build/%.o,$(SOURCES))
CORE_OBJECTS = $(patsubst %.cpp,build/%.o,$(CORE))
TEST_OBJECTS = build/tests/game_tests.o build/tests/graphics_tests.o

.PHONY: all run clean test test-graphics test-ui test-api dist
all: trabalhocg

run: all
	./trabalhocg

trabalhocg: $(OBJECTS)
	$(CXX) $(CXXFLAGS) $(LDFLAGS) $^ $(LDLIBS) -o $@

build/%.o: %.cpp
	@mkdir -p $(dir $@)
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) -MMD -MP -c $< -o $@

build/testes: $(CORE_OBJECTS) build/tests/game_tests.o
	$(CXX) $(CXXFLAGS) $(LDFLAGS) $^ -o $@

build/testes_graficos: $(CORE_OBJECTS) build/src/renderer.o build/tests/graphics_tests.o
	$(CXX) $(CXXFLAGS) $(LDFLAGS) $^ $(LDLIBS) -o $@

test: build/testes
	./build/testes

test-graphics: build/testes_graficos
	./build/testes_graficos

test-ui: all
	python3 tests/ui_tests.py

test-api:
	python3 tests/check_gl_api.py

dist:
	python3 tests/package.py "$(AUTOR)"

clean:
	rm -rf build trabalhocg

-include $(OBJECTS:.o=.d) $(TEST_OBJECTS:.o=.d)
