CXX ?= g++
CXXFLAGS ?= -std=c++17 -O2 -g -Wall -Wextra -Wpedantic
CPPFLAGS += -Isrc -Ithird_party/tinyxml2
LDLIBS += -lglut -lGL

FONTES = src/configuracao.cpp src/geometria.cpp src/jogo.cpp src/controlador.cpp src/renderizador.cpp src/principal.cpp third_party/tinyxml2/tinyxml2.cpp
OBJETOS = $(patsubst %.cpp,build/%.o,$(FONTES))

.PHONY: all run web clean
all: trabalhocg

run: all
	./trabalhocg

web:
	./web/build.sh

trabalhocg: $(OBJETOS)
	$(CXX) $(CXXFLAGS) $(LDFLAGS) $^ $(LDLIBS) -o $@

build/%.o: %.cpp
	@mkdir -p $(dir $@)
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) -MMD -MP -c $< -o $@

clean:
	rm -rf build web/dist trabalhocg

-include $(OBJETOS:.o=.d)
