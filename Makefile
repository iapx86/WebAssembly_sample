targets = \
	1942.wasm.js baraduke.wasm.js chackn_pop.wasm.js crush_roller.wasm.js digdug.wasm.js digdug_ii.wasm.js \
	dragon_buster.wasm.js elevator_action.wasm.js frogger.wasm.js gradius.wasm.js grobda.wasm.js libble_rabble.wasm.js \
	mappy.wasm.js metro-cross.wasm.js motos.wasm.js pac-land.wasm.js pac-man.wasm.js pac_and_pal.wasm.js pengo.wasm.js \
	phozon.wasm.js sea_fighter_poseidon.wasm.js sky_kid.wasm.js star_force.wasm.js strategy_x.wasm.js \
	super_pac-man.wasm.js the_tower_of_druaga.wasm.js time_pilot.wasm.js time_tunnel.wasm.js toypop.wasm.js \
	twinbee.wasm.js vulgus.wasm.js zigzag.wasm.js

.PHONY: all
all: dist $(addprefix dist/,$(targets))

.PHONY: clean
clean:
	del dist\*.wasm*

dist:
	mkdir dist

%.wasm.js: %.wasm
	pwsh -ExecutionPolicy RemoteSigned -File convert.ps1 $< $@

dist/1942.wasm: $(addprefix src/,1942.cpp z80.cpp cpu.cpp 1942.h z80.h cpu.h ay-3-8910.h)
	emcc -O3 --no-entry -Wno-shift-op-parentheses -o $@ $(filter %.cpp,$^)

dist/baraduke.wasm: $(addprefix src/,baraduke.cpp cpu.cpp baraduke.h mc6809.h mc6801.h cpu.h c30.h)
	emcc -O3 --no-entry -Wno-shift-op-parentheses -o $@ $(filter %.cpp,$^)

dist/chackn_pop.wasm: $(addprefix src/,chackn_pop.cpp z80.cpp cpu.cpp chackn_pop.h z80.h mc6805.h cpu.h ay-3-8910.h)
	emcc -O3 --no-entry -Wno-shift-op-parentheses -o $@ $(filter %.cpp,$^)

dist/crush_roller.wasm: $(addprefix src/,crush_roller.cpp z80.cpp cpu.cpp crush_roller.h z80.h cpu.h pac-man_sound.h)
	emcc -O3 --no-entry -Wno-shift-op-parentheses -o $@ $(filter %.cpp,$^)

dist/digdug.wasm: $(addprefix src/,digdug.cpp z80.cpp cpu.cpp digdug.h z80.h mb8840.h cpu.h pac-man_sound.h)
	emcc -O3 --no-entry -Wno-shift-op-parentheses -o $@ $(filter %.cpp,$^)

dist/digdug_ii.wasm: $(addprefix src/,digdug_ii.cpp cpu.cpp digdug_ii.h mc6809.h cpu.h mappy_sound.h)
	emcc -O3 --no-entry -Wno-shift-op-parentheses -o $@ $(filter %.cpp,$^)

dist/dragon_buster.wasm: $(addprefix src/,dragon_buster.cpp cpu.cpp dragon_buster.h mc6809.h mc6801.h cpu.h c30.h)
	emcc -O3 --no-entry -Wno-shift-op-parentheses -o $@ $(filter %.cpp,$^)

dist/elevator_action.wasm: $(addprefix src/,elevator_action.cpp z80.cpp cpu.cpp elevator_action.h z80.h mc6805.h cpu.h ay-3-8910.h sound_effect.h)
	emcc -O3 --no-entry -Wno-shift-op-parentheses -o $@ $(filter %.cpp,$^)

dist/frogger.wasm: $(addprefix src/,frogger.cpp z80.cpp cpu.cpp frogger.h z80.h cpu.h ay-3-8910.h)
	emcc -O3 --no-entry -Wno-shift-op-parentheses -o $@ $(filter %.cpp,$^)

dist/gradius.wasm: $(addprefix src/,gradius.cpp z80.cpp cpu.cpp vlm5030.cpp gradius.h mc68000.h z80.h cpu.h ay-3-8910.h k005289.h vlm5030.h)
	emcc -O3 -s INITIAL_MEMORY=33554432 --no-entry -Wno-shift-op-parentheses -o $@ $(filter %.cpp,$^)

dist/grobda.wasm: $(addprefix src/,grobda.cpp cpu.cpp grobda.h mc6809.h cpu.h mappy_sound.h sound_effect.h)
	emcc -O3 --no-entry -Wno-shift-op-parentheses -o $@ $(filter %.cpp,$^)

dist/libble_rabble.wasm: $(addprefix src/,libble_rabble.cpp cpu.cpp libble_rabble.h mc6809.h mc68000.h cpu.h mappy_sound.h)
	emcc -O3 -s INITIAL_MEMORY=33554432 --no-entry -Wno-shift-op-parentheses -o $@ $(filter %.cpp,$^)

dist/mappy.wasm: $(addprefix src/,mappy.cpp cpu.cpp mappy.h mc6809.h cpu.h mappy_sound.h)
	emcc -O3 --no-entry -Wno-shift-op-parentheses -o $@ $(filter %.cpp,$^)

dist/metro-cross.wasm: $(addprefix src/,metro-cross.cpp cpu.cpp metro-cross.h mc6809.h mc6801.h cpu.h c30.h)
	emcc -O3 --no-entry -Wno-shift-op-parentheses -o $@ $(filter %.cpp,$^)

dist/motos.wasm: $(addprefix src/,motos.cpp cpu.cpp motos.h mc6809.h cpu.h mappy_sound.h)
	emcc -O3 --no-entry -Wno-shift-op-parentheses -o $@ $(filter %.cpp,$^)

dist/pac-land.wasm: $(addprefix src/,pac-land.cpp cpu.cpp pac-land.h mc6809.h mc6801.h cpu.h c30.h)
	emcc -O3 --no-entry -Wno-shift-op-parentheses -o $@ $(filter %.cpp,$^)

dist/pac-man.wasm: $(addprefix src/,pac-man.cpp z80.cpp cpu.cpp pac-man.h z80.h cpu.h pac-man_sound.h)
	emcc -O3 --no-entry -Wno-shift-op-parentheses -o $@ $(filter %.cpp,$^)

dist/pac_and_pal.wasm: $(addprefix src/,pac_and_pal.cpp cpu.cpp pac_and_pal.h mc6809.h cpu.h mappy_sound.h)
	emcc -O3 --no-entry -Wno-shift-op-parentheses -o $@ $(filter %.cpp,$^)

dist/pengo.wasm: $(addprefix src/,pengo.cpp z80.cpp cpu.cpp pengo.h z80.h cpu.h pac-man_sound.h)
	emcc -O3 --no-entry -Wno-shift-op-parentheses -o $@ $(filter %.cpp,$^)

dist/phozon.wasm: $(addprefix src/,phozon.cpp cpu.cpp phozon.h mc6809.h cpu.h mappy_sound.h)
	emcc -O3 --no-entry -Wno-shift-op-parentheses -o $@ $(filter %.cpp,$^)

dist/sea_fighter_poseidon.wasm: $(addprefix src/,sea_fighter_poseidon.cpp z80.cpp cpu.cpp sea_fighter_poseidon.h z80.h mc6805.h cpu.h ay-3-8910.h sound_effect.h)
	emcc -O3 --no-entry -Wno-shift-op-parentheses -o $@ $(filter %.cpp,$^)

dist/sky_kid.wasm: $(addprefix src/,sky_kid.cpp cpu.cpp sky_kid.h mc6809.h mc6801.h cpu.h c30.h)
	emcc -O3 --no-entry -Wno-shift-op-parentheses -o $@ $(filter %.cpp,$^)

dist/star_force.wasm: $(addprefix src/,star_force.cpp z80.cpp cpu.cpp star_force.h z80.h cpu.h sn76489.h senjyo_sound.h)
	emcc -O3 --no-entry -Wno-shift-op-parentheses -o $@ $(filter %.cpp,$^)

dist/strategy_x.wasm: $(addprefix src/,strategy_x.cpp z80.cpp cpu.cpp strategy_x.h z80.h cpu.h ay-3-8910.h)
	emcc -O3 --no-entry -Wno-shift-op-parentheses -o $@ $(filter %.cpp,$^)

dist/super_pac-man.wasm: $(addprefix src/,super_pac-man.cpp cpu.cpp super_pac-man.h mc6809.h cpu.h mappy_sound.h)
	emcc -O3 --no-entry -Wno-shift-op-parentheses -o $@ $(filter %.cpp,$^)

dist/the_tower_of_druaga.wasm: $(addprefix src/,the_tower_of_druaga.cpp cpu.cpp the_tower_of_druaga.h mc6809.h cpu.h mappy_sound.h)
	emcc -O3 --no-entry -Wno-shift-op-parentheses -o $@ $(filter %.cpp,$^)

dist/time_pilot.wasm: $(addprefix src/,time_pilot.cpp z80.cpp cpu.cpp time_pilot.h z80.h cpu.h ay-3-8910.h)
	emcc -O3 --no-entry -Wno-shift-op-parentheses -o $@ $(filter %.cpp,$^)

dist/time_tunnel.wasm: $(addprefix src/,time_tunnel.cpp z80.cpp cpu.cpp time_tunnel.h z80.h cpu.h ay-3-8910.h sound_effect.h)
	emcc -O3 -s INITIAL_MEMORY=268435456 --no-entry -Wno-shift-op-parentheses -o $@ $(filter %.cpp,$^)

dist/toypop.wasm: $(addprefix src/,toypop.cpp cpu.cpp toypop.h mc6809.h mc68000.h cpu.h mappy_sound.h)
	emcc -O3 -s INITIAL_MEMORY=33554432 --no-entry -Wno-shift-op-parentheses -o $@ $(filter %.cpp,$^)

dist/twinbee.wasm: $(addprefix src/,twinbee.cpp z80.cpp cpu.cpp vlm5030.cpp twinbee.h mc68000.h z80.h cpu.h ay-3-8910.h k005289.h vlm5030.h)
	emcc -O3 -s INITIAL_MEMORY=33554432 --no-entry -Wno-shift-op-parentheses -o $@ $(filter %.cpp,$^)

dist/vulgus.wasm: $(addprefix src/,vulgus.cpp z80.cpp cpu.cpp vulgus.h z80.h cpu.h ay-3-8910.h)
	emcc -O3 --no-entry -Wno-shift-op-parentheses -o $@ $(filter %.cpp,$^)

dist/zigzag.wasm: $(addprefix src/,zigzag.cpp z80.cpp cpu.cpp zigzag.h z80.h cpu.h ay-3-8910.h)
	emcc -O3 --no-entry -Wno-shift-op-parentheses -o $@ $(filter %.cpp,$^)

