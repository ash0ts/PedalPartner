CXX ?= c++
PYTHON ?= python3
CXXFLAGS ?= -std=c++17 -O2 -Wall -Wextra -Werror
STAGE ?= 4
PORT ?= 8765
HEADERS := $(wildcard firmware/VEX_Pocket_Rival/*.h)

.PHONY: test sim sim-build firmware firmware-all setup-firmware cad cad-check render render-meshes hero pack-mentor docs check clean
build:
	mkdir -p build
build/core-tests: tests/core_tests.cpp $(HEADERS) | build
	$(CXX) $(CXXFLAGS) $< -o $@
build/vex-sim: simulator/main.cpp simulator/canvas.h $(HEADERS) | build
	$(CXX) $(CXXFLAGS) $< -o $@
test: build/core-tests build/vex-sim
	./build/core-tests
	$(PYTHON) tests/simulator_test.py
	$(PYTHON) tests/pack_test.py
	$(PYTHON) tools/check_project.py
sim-build: build/vex-sim
sim: build/vex-sim
	$(PYTHON) simulator/server.py --port $(PORT)
setup-firmware:
	$(PYTHON) tools/firmware.py setup
firmware:
	$(PYTHON) tools/firmware.py build --stage $(STAGE)
firmware-all:
	$(PYTHON) tools/firmware.py build --all
cad:
	$(PYTHON) hardware/cad/build_cad.py
	$(PYTHON) hardware/cad/make_cut_profiles.py
cad-check:
	$(PYTHON) hardware/cad/validate_delivery.py
render:
	blender -b --python hardware/cad/render_cad.py
render-meshes:
	$(PYTHON) hardware/cad/export_render_meshes.py
hero: render-meshes
	blender -b --python-exit-code 1 --python hardware/cad/render_hero.py
	$(PYTHON) tools/compose_hero.py
pack-mentor:
	$(PYTHON) tools/pack_mentor.py
docs:
	$(PYTHON) tools/make_docs.py
check: test
clean:
	$(PYTHON) -c "import shutil; shutil.rmtree('build',ignore_errors=True)"
