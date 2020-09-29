targets = 1942.wasm.js elevator_action.wasm.js star_force.wasm.js time_pilot.wasm.js twinbee.wasm.js

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

dist/elevator_action.wasm: $(addprefix src/,elevator_action.cpp z80.cpp cpu.cpp elevator_action.h z80.h mc6805.h cpu.h ay-3-8910.h sound_effect.h)
	emcc -O3 --no-entry -Wno-shift-op-parentheses -o $@ $(filter %.cpp,$^)

dist/star_force.wasm: $(addprefix src/,star_force.cpp z80.cpp cpu.cpp star_force.h z80.h cpu.h sn76489.h senjyo_sound.h)
	emcc -O3 --no-entry -Wno-shift-op-parentheses -o $@ $(filter %.cpp,$^)

dist/time_pilot.wasm: $(addprefix src/,time_pilot.cpp z80.cpp cpu.cpp time_pilot.h z80.h cpu.h ay-3-8910.h)
	emcc -O3 --no-entry -Wno-shift-op-parentheses -o $@ $(filter %.cpp,$^)

dist/twinbee.wasm: $(addprefix src/,twinbee.cpp z80.cpp cpu.cpp vlm5030.cpp twinbee.h mc68000.h z80.h cpu.h ay-3-8910.h k005289.h vlm5030.h)
	emcc -O3 -s INITIAL_MEMORY=33554432 --no-entry -Wno-shift-op-parentheses -o $@ $(filter %.cpp,$^)

