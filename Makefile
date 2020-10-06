targets = \
	1942.wasm.js baraduke.wasm.js chackn_pop.wasm.js crush_roller.wasm.js dragon_buster.wasm.js elevator_action.wasm.js \
	frogger.wasm.js gradius.wasm.js libble_rabble.wasm.js star_force.wasm.js time_pilot.wasm.js twinbee.wasm.js

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

dist/baraduke.wasm: $(addprefix src/,baraduke.cpp cpu.cpp baraduke.h mc6809.h mc6801.h cpu.h c30.h)
	emcc -O3 --no-entry -Wno-shift-op-parentheses -o $@ $(filter %.cpp,$^)

dist/chackn_pop.wasm: $(addprefix src/,chackn_pop.cpp z80.cpp cpu.cpp chackn_pop.h z80.h mc6805.h cpu.h ay-3-8910.h)
	emcc -O3 --no-entry -Wno-shift-op-parentheses -o $@ $(filter %.cpp,$^)

dist/crush_roller.wasm: $(addprefix src/,crush_roller.cpp z80.cpp cpu.cpp crush_roller.h z80.h cpu.h pac-man_sound.h)
	emcc -O3 --no-entry -Wno-shift-op-parentheses -o $@ $(filter %.cpp,$^)

dist/dragon_buster.wasm: $(addprefix src/,dragon_buster.cpp cpu.cpp dragon_buster.h mc6809.h mc6801.h cpu.h c30.h)
	emcc -O3 --no-entry -Wno-shift-op-parentheses -o $@ $(filter %.cpp,$^)

dist/elevator_action.wasm: $(addprefix src/,elevator_action.cpp z80.cpp cpu.cpp elevator_action.h z80.h mc6805.h cpu.h ay-3-8910.h sound_effect.h)
	emcc -O3 --no-entry -Wno-shift-op-parentheses -o $@ $(filter %.cpp,$^)

dist/frogger.wasm: $(addprefix src/,frogger.cpp z80.cpp cpu.cpp frogger.h z80.h cpu.h ay-3-8910.h)
	emcc -O3 --no-entry -Wno-shift-op-parentheses -o $@ $(filter %.cpp,$^)

dist/gradius.wasm: $(addprefix src/,gradius.cpp z80.cpp cpu.cpp vlm5030.cpp gradius.h mc68000.h z80.h cpu.h ay-3-8910.h k005289.h vlm5030.h)
	emcc -O3 -s INITIAL_MEMORY=33554432 --no-entry -Wno-shift-op-parentheses -o $@ $(filter %.cpp,$^)

dist/libble_rabble.wasm: $(addprefix src/,libble_rabble.cpp cpu.cpp libble_rabble.h mc6809.h mc68000.h cpu.h mappy_sound.h)
	emcc -O3 -s INITIAL_MEMORY=33554432 --no-entry -Wno-shift-op-parentheses -o $@ $(filter %.cpp,$^)

dist/star_force.wasm: $(addprefix src/,star_force.cpp z80.cpp cpu.cpp star_force.h z80.h cpu.h sn76489.h senjyo_sound.h)
	emcc -O3 --no-entry -Wno-shift-op-parentheses -o $@ $(filter %.cpp,$^)

dist/time_pilot.wasm: $(addprefix src/,time_pilot.cpp z80.cpp cpu.cpp time_pilot.h z80.h cpu.h ay-3-8910.h)
	emcc -O3 --no-entry -Wno-shift-op-parentheses -o $@ $(filter %.cpp,$^)

dist/twinbee.wasm: $(addprefix src/,twinbee.cpp z80.cpp cpu.cpp vlm5030.cpp twinbee.h mc68000.h z80.h cpu.h ay-3-8910.h k005289.h vlm5030.h)
	emcc -O3 -s INITIAL_MEMORY=33554432 --no-entry -Wno-shift-op-parentheses -o $@ $(filter %.cpp,$^)

