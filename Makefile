CXX ?= c++
PYTHON ?= python3
CXXFLAGS ?= -std=c++17 -O2 -Wall -Wextra -Werror
STAGE ?= 4
PORT ?= 8765
# Bench loop: DEVICE is a serial port (/dev/cu.usbmodem*) or `auto` (exactly one Teensy); empty = no device.
DEVICE ?=
SERIAL ?=
SEND ?=
HEADERS := $(wildcard firmware/VEX_Pocket_Rival/*.h)
ART := $(wildcard art/*.json art/*/*.json art/*/*.png)

.PHONY: test sim sim-build art art-check motion web web-check firmware firmware-all setup-firmware upload monitor ports bench bench-report measure-check cad cad-check render render-meshes hero pack-mentor pack-studio docs check clean
build:
	mkdir -p build
build/core-tests: tests/core_tests.cpp $(HEADERS) | build
	$(CXX) $(CXXFLAGS) $< -o $@
build/vex-sim: simulator/main.cpp $(wildcard simulator/*.h) $(HEADERS) | build
	$(CXX) $(CXXFLAGS) $< -o $@
test: build/core-tests build/vex-sim
	./build/core-tests
	$(PYTHON) tests/simulator_test.py
	$(PYTHON) tests/pack_test.py
	$(PYTHON) tests/art_test.py
	$(PYTHON) tests/link_test.py
	$(PYTHON) tests/bench_report_test.py
	$(PYTHON) tests/measure_check_test.py
	$(PYTHON) tools/check_project.py
	$(PYTHON) tools/web.py check
sim-build: build/vex-sim
# Import art/ (PNG sprite strips, theme, fonts) into the generated firmware headers; see docs/ART.md.
art:
	$(PYTHON) tools/make_sprites.py
art-check:
	$(PYTHON) tools/make_sprites.py --check
# Motion video through the shared renderer at DISPLAY_MS (needs ffmpeg); verifies each segment's animation.
motion: build/vex-sim
	$(PYTHON) tools/render_motion.py
# In-browser workbench: shared firmware headers compiled to WebAssembly (needs emcc; see docs/LOCAL_DEVELOPMENT.md).
web:
	$(PYTHON) tools/web.py build
web-check:
	$(PYTHON) tools/web.py check
sim:
	$(PYTHON) tools/web.py serve --port $(PORT) $(if $(DEVICE),--device $(DEVICE),)
setup-firmware:
	$(PYTHON) tools/firmware.py setup
firmware:
	$(PYTHON) tools/firmware.py build --stage $(STAGE)
firmware-all:
	$(PYTHON) tools/firmware.py build --all
# The only targets that touch a Teensy. upload needs a compiled stage and an explicit or unique port.
upload:
	$(PYTHON) tools/firmware.py upload --stage $(STAGE) $(if $(SERIAL),--port $(SERIAL),)
monitor:
	$(PYTHON) tools/firmware.py monitor $(if $(SERIAL),--port $(SERIAL),) $(if $(SEND),--send '$(SEND)',)
ports:
	$(PYTHON) tools/firmware.py ports
# One bench loop: compile the stage, program the board, open the workbench with the Device tab bridged.
bench:
	$(MAKE) firmware STAGE=$(STAGE)
	$(MAKE) upload STAGE=$(STAGE) SERIAL=$(SERIAL)
	$(MAKE) sim DEVICE=$(if $(SERIAL),$(SERIAL),auto)
# Summarise the latest bench log (build/bench/latest.jsonl) or LOG=path.
bench-report:
	$(PYTHON) tools/bench_report.py $(LOG)
# Measured parts against CAD assumptions, and which STL/DXF files may be fabricated now.
measure-check:
	$(PYTHON) tools/measure_check.py
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
pack-studio:
	$(PYTHON) tools/pack_mentor.py --profile studio
docs:
	$(PYTHON) tools/make_docs.py
check: test
clean:
	$(PYTHON) -c "import shutil; shutil.rmtree('build',ignore_errors=True)"
