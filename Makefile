targets = 1942.wasm.js star_force.wasm.js time_pilot.wasm.js

.PHONY: all
all: dist $(addprefix dist/,$(targets))

.PHONY: clean
clean:
	del dist\*.wasm*

dist:
	mkdir dist

%.wasm.js: %.wasm
	powershell -ExecutionPolicy RemoteSigned -File convert.ps1 $< $@

dist/1942.wasm: $(addprefix src/,1942.cpp z80.cpp cpu.cpp 1942.h z80.h cpu.h ay-3-8910.h)
	emcc -O3 --no-entry -Wno-shift-op-parentheses -o $@ $(filter %.cpp,$^)

dist/star_force.wasm: $(addprefix src/,star_force.cpp z80.cpp cpu.cpp star_force.h z80.h cpu.h sn76489.h senjyo_sound.h)
	emcc -O3 --no-entry -Wno-shift-op-parentheses -o $@ $(filter %.cpp,$^)

dist/time_pilot.wasm: $(addprefix src/,time_pilot.cpp z80.cpp cpu.cpp time_pilot.h z80.h cpu.h ay-3-8910.h)
	emcc -O3 --no-entry -Wno-shift-op-parentheses -o $@ $(filter %.cpp,$^)

