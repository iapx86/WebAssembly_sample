targets = \
	1942.wasm.js baraduke.wasm.js chackn_pop.wasm.js crush_roller.wasm.js digdug.wasm.js digdug_ii.wasm.js \
	dragon_buster.wasm.js elevator_action.wasm.js frogger.wasm.js gradius.wasm.js grobda.wasm.js libble_rabble.wasm.js \
	mappy.wasm.js metro-cross.wasm.js motos.wasm.js pac-land.wasm.js pac-man.wasm.js pac_and_pal.wasm.js pengo.wasm.js \
	phozon.wasm.js sea_fighter_poseidon.wasm.js sky_kid.wasm.js star_force.wasm.js strategy_x.wasm.js \
	super_pac-man.wasm.js the_tower_of_druaga.wasm.js time_pilot.wasm.js time_tunnel.wasm.js toypop.wasm.js \
	twinbee.wasm.js vulgus.wasm.js zigzag.wasm.js
targets += \
	1942.png.js baraduke.png.js chackn_pop.png.js crush_roller.png.js digdug.png.js digdug_ii.png.js \
	dragon_buster.png.js elevator_action.png.js frogger.png.js gradius.png.js grobda.png.js libble_rabble.png.js \
	mappy.png.js metro-cross.png.js motos.png.js pac-land.png.js pac-man.png.js pac_and_pal.png.js pengo.png.js \
	phozon.png.js sea_fighter_poseidon.png.js sky_kid.png.js star_force.png.js strategy_x.png.js super_pac-man.png.js \
	the_tower_of_druaga.png.js time_pilot.png.js time_tunnel.png.js toypop.png.js twinbee.png.js vulgus.png.js \
	zigzag.png.js

.PHONY: all
all: dist $(addprefix dist/,$(targets))

.PHONY: clean
clean:
	del dist\*.wasm*
	del dist\*.png.js

dist:
	mkdir dist

%.wasm.js: convert.js %.wasm
	node $^ $@

dist/1942.wasm: $(addprefix src/,1942.cpp z80.cpp cpu.cpp 1942.h z80.h cpu.h ay-3-8910.h utils.h)
	emcc --std=c++20 -O3 --no-entry -Wno-shift-op-parentheses -o $@ $(filter %.cpp,$^)

dist/1942.png.js: $(addprefix roms/,1942.js 1942.zip)
	node $^ $@

dist/baraduke.wasm: $(addprefix src/,baraduke.cpp mc6809.cpp mc6801.cpp cpu.cpp baraduke.h mc6809.h mc6801.h cpu.h c30.h utils.h)
	emcc --std=c++20 -O3 --no-entry -Wno-shift-op-parentheses -o $@ $(filter %.cpp,$^)

dist/baraduke.png.js: $(addprefix roms/,baraduke.js aliensec.zip)
	node $^ $@

dist/chackn_pop.wasm: $(addprefix src/,chackn_pop.cpp z80.cpp mc6805.cpp cpu.cpp chackn_pop.h z80.h mc6805.h cpu.h ay-3-8910.h utils.h)
	emcc --std=c++20 -O3 --no-entry -Wno-shift-op-parentheses -o $@ $(filter %.cpp,$^)

dist/chackn_pop.png.js: $(addprefix roms/,chackn_pop.js chaknpop.zip)
	node $^ $@

dist/crush_roller.wasm: $(addprefix src/,crush_roller.cpp z80.cpp cpu.cpp crush_roller.h z80.h cpu.h pac-man_sound.h utils.h)
	emcc --std=c++20 -O3 --no-entry -Wno-shift-op-parentheses -o $@ $(filter %.cpp,$^)

dist/crush_roller.png.js: $(addprefix roms/,crush_roller.js crush.zip)
	node $^ $@

dist/digdug.wasm: $(addprefix src/,digdug.cpp z80.cpp mb8840.cpp cpu.cpp digdug.h z80.h mb8840.h cpu.h pac-man_sound.h utils.h)
	emcc --std=c++20 -O3 --no-entry -Wno-shift-op-parentheses -o $@ $(filter %.cpp,$^)

dist/digdug.png.js: $(addprefix roms/,digdug.js digdug.zip namco51.zip)
	node $^ $@

dist/digdug_ii.wasm: $(addprefix src/,digdug_ii.cpp mc6809.cpp cpu.cpp digdug_ii.h mc6809.h cpu.h mappy_sound.h utils.h)
	emcc --std=c++20 -O3 --no-entry -Wno-shift-op-parentheses -o $@ $(filter %.cpp,$^)

dist/digdug_ii.png.js: $(addprefix roms/,digdug_ii.js digdug2.zip)
	node $^ $@

dist/dragon_buster.wasm: $(addprefix src/,dragon_buster.cpp mc6809.cpp mc6801.cpp cpu.cpp dragon_buster.h mc6809.h mc6801.h cpu.h c30.h utils.h)
	emcc --std=c++20 -O3 --no-entry -Wno-shift-op-parentheses -o $@ $(filter %.cpp,$^)

dist/dragon_buster.png.js: $(addprefix roms/,dragon_buster.js drgnbstr.zip)
	node $^ $@

dist/elevator_action.wasm: $(addprefix src/,elevator_action.cpp z80.cpp mc6805.cpp cpu.cpp elevator_action.h z80.h mc6805.h cpu.h ay-3-8910.h dac.h utils.h)
	emcc --std=c++20 -O3 --no-entry -Wno-shift-op-parentheses -o $@ $(filter %.cpp,$^)

dist/elevator_action.png.js: $(addprefix roms/,elevator_action.js elevator.zip)
	node $^ $@

dist/frogger.wasm: $(addprefix src/,frogger.cpp z80.cpp cpu.cpp frogger.h z80.h cpu.h ay-3-8910.h utils.h)
	emcc --std=c++20 -O3 --no-entry -Wno-shift-op-parentheses -o $@ $(filter %.cpp,$^)

dist/frogger.png.js: $(addprefix roms/,frogger.js frogger.zip)
	node $^ $@

dist/gradius.wasm: $(addprefix src/,gradius.cpp mc68000.cpp z80.cpp cpu.cpp vlm5030.cpp gradius.h mc68000.h z80.h cpu.h ay-3-8910.h k005289.h vlm5030.h utils.h)
	emcc --std=c++20 -O3 -s INITIAL_MEMORY=33554432 --no-entry -Wno-shift-op-parentheses -o $@ $(filter %.cpp,$^)

dist/gradius.png.js: $(addprefix roms/,gradius.js nemesis.zip)
	node $^ $@

dist/grobda.wasm: $(addprefix src/,grobda.cpp mc6809.cpp cpu.cpp grobda.h mc6809.h cpu.h mappy_sound.h utils.h)
	emcc --std=c++20 -O3 --no-entry -Wno-shift-op-parentheses -o $@ $(filter %.cpp,$^)

dist/grobda.png.js: $(addprefix roms/,grobda.js grobda.zip)
	node $^ $@

dist/libble_rabble.wasm: $(addprefix src/,libble_rabble.cpp mc6809.cpp mc68000.cpp cpu.cpp libble_rabble.h mc6809.h mc68000.h cpu.h mappy_sound.h utils.h)
	emcc --std=c++20 -O3 -s INITIAL_MEMORY=33554432 --no-entry -Wno-shift-op-parentheses -o $@ $(filter %.cpp,$^)

dist/libble_rabble.png.js: $(addprefix roms/,libble_rabble.js liblrabl.zip)
	node $^ $@

dist/mappy.wasm: $(addprefix src/,mappy.cpp mc6809.cpp cpu.cpp mappy.h mc6809.h cpu.h mappy_sound.h utils.h)
	emcc --std=c++20 -O3 --no-entry -Wno-shift-op-parentheses -o $@ $(filter %.cpp,$^)

dist/mappy.png.js: $(addprefix roms/,mappy.js mappy.zip)
	node $^ $@

dist/metro-cross.wasm: $(addprefix src/,metro-cross.cpp mc6809.cpp mc6801.cpp cpu.cpp metro-cross.h mc6809.h mc6801.h cpu.h c30.h utils.h utils.h)
	emcc --std=c++20 -O3 --no-entry -Wno-shift-op-parentheses -o $@ $(filter %.cpp,$^)

dist/metro-cross.png.js: $(addprefix roms/,metro-cross.js metrocrs.zip)
	node $^ $@

dist/motos.wasm: $(addprefix src/,motos.cpp mc6809.cpp cpu.cpp motos.h mc6809.h cpu.h mappy_sound.h utils.h)
	emcc --std=c++20 -O3 --no-entry -Wno-shift-op-parentheses -o $@ $(filter %.cpp,$^)

dist/motos.png.js: $(addprefix roms/,motos.js motos.zip)
	node $^ $@

dist/pac-land.wasm: $(addprefix src/,pac-land.cpp mc6809.cpp mc6801.cpp cpu.cpp pac-land.h mc6809.h mc6801.h cpu.h c30.h utils.h)
	emcc --std=c++20 -O3 --no-entry -Wno-shift-op-parentheses -o $@ $(filter %.cpp,$^)

dist/pac-land.png.js: $(addprefix roms/,pac-land.js pacland.zip)
	node $^ $@

dist/pac-man.wasm: $(addprefix src/,pac-man.cpp z80.cpp cpu.cpp pac-man.h z80.h cpu.h pac-man_sound.h utils.h)
	emcc --std=c++20 -O3 --no-entry -Wno-shift-op-parentheses -o $@ $(filter %.cpp,$^)

dist/pac-man.png.js: $(addprefix roms/,pac-man.js puckman.zip)
	node $^ $@

dist/pac_and_pal.wasm: $(addprefix src/,pac_and_pal.cpp mc6809.cpp cpu.cpp pac_and_pal.h mc6809.h cpu.h mappy_sound.h utils.h)
	emcc --std=c++20 -O3 --no-entry -Wno-shift-op-parentheses -o $@ $(filter %.cpp,$^)

dist/pac_and_pal.png.js: $(addprefix roms/,pac_and_pal.js pacnpal.zip)
	node $^ $@

dist/pengo.wasm: $(addprefix src/,pengo.cpp sega_z80.cpp z80.cpp cpu.cpp pengo.h sega_z80.h z80.h cpu.h pac-man_sound.h utils.h)
	emcc --std=c++20 -O3 --no-entry -Wno-shift-op-parentheses -o $@ $(filter %.cpp,$^)

dist/pengo.png.js: $(addprefix roms/,pengo.js pengo.zip)
	node $^ $@

dist/phozon.wasm: $(addprefix src/,phozon.cpp mc6809.cpp cpu.cpp phozon.h mc6809.h cpu.h mappy_sound.h utils.h)
	emcc --std=c++20 -O3 --no-entry -Wno-shift-op-parentheses -o $@ $(filter %.cpp,$^)

dist/phozon.png.js: $(addprefix roms/,phozon.js phozon.zip)
	node $^ $@

dist/sea_fighter_poseidon.wasm: $(addprefix src/,sea_fighter_poseidon.cpp z80.cpp mc6805.cpp cpu.cpp sea_fighter_poseidon.h z80.h mc6805.h cpu.h ay-3-8910.h dac.h utils.h)
	emcc --std=c++20 -O3 --no-entry -Wno-shift-op-parentheses -o $@ $(filter %.cpp,$^)

dist/sea_fighter_poseidon.png.js: $(addprefix roms/,sea_fighter_poseidon.js sfposeid.zip)
	node $^ $@

dist/sky_kid.wasm: $(addprefix src/,sky_kid.cpp mc6809.cpp mc6801.cpp cpu.cpp sky_kid.h mc6809.h mc6801.h cpu.h c30.h utils.h)
	emcc --std=c++20 -O3 --no-entry -Wno-shift-op-parentheses -o $@ $(filter %.cpp,$^)

dist/sky_kid.png.js: $(addprefix roms/,sky_kid.js skykid.zip)
	node $^ $@

dist/star_force.wasm: $(addprefix src/,star_force.cpp z80.cpp cpu.cpp star_force.h z80.h cpu.h sn76489.h senjyo_sound.h utils.h)
	emcc --std=c++20 -O3 --no-entry -Wno-shift-op-parentheses -o $@ $(filter %.cpp,$^)

dist/star_force.png.js: $(addprefix roms/,star_force.js starforc.zip)
	node $^ $@

dist/strategy_x.wasm: $(addprefix src/,strategy_x.cpp z80.cpp cpu.cpp strategy_x.h z80.h cpu.h ay-3-8910.h utils.h)
	emcc --std=c++20 -O3 --no-entry -Wno-shift-op-parentheses -o $@ $(filter %.cpp,$^)

dist/strategy_x.png.js: $(addprefix roms/,strategy_x.js stratgyx.zip)
	node $^ $@

dist/super_pac-man.wasm: $(addprefix src/,super_pac-man.cpp mc6809.cpp cpu.cpp super_pac-man.h mc6809.h cpu.h mappy_sound.h utils.h)
	emcc --std=c++20 -O3 --no-entry -Wno-shift-op-parentheses -o $@ $(filter %.cpp,$^)

dist/super_pac-man.png.js: $(addprefix roms/,super_pac-man.js superpac.zip)
	node $^ $@

dist/the_tower_of_druaga.wasm: $(addprefix src/,the_tower_of_druaga.cpp mc6809.cpp cpu.cpp the_tower_of_druaga.h mc6809.h cpu.h mappy_sound.h utils.h)
	emcc --std=c++20 -O3 --no-entry -Wno-shift-op-parentheses -o $@ $(filter %.cpp,$^)

dist/the_tower_of_druaga.png.js: $(addprefix roms/,the_tower_of_druaga.js todruaga.zip)
	node $^ $@

dist/time_pilot.wasm: $(addprefix src/,time_pilot.cpp z80.cpp cpu.cpp time_pilot.h z80.h cpu.h ay-3-8910.h utils.h)
	emcc --std=c++20 -O3 --no-entry -Wno-shift-op-parentheses -o $@ $(filter %.cpp,$^)

dist/time_pilot.png.js: $(addprefix roms/,time_pilot.js timeplt.zip)
	node $^ $@

dist/time_tunnel.wasm: $(addprefix src/,time_tunnel.cpp z80.cpp cpu.cpp time_tunnel.h z80.h cpu.h ay-3-8910.h dac.h utils.h)
	emcc --std=c++20 -O3 --no-entry -Wno-shift-op-parentheses -o $@ $(filter %.cpp,$^)

dist/time_tunnel.png.js: $(addprefix roms/,time_tunnel.js timetunl.zip)
	node $^ $@

dist/toypop.wasm: $(addprefix src/,toypop.cpp mc6809.cpp mc68000.cpp cpu.cpp toypop.h mc6809.h mc68000.h cpu.h mappy_sound.h utils.h)
	emcc --std=c++20 -O3 -s INITIAL_MEMORY=33554432 --no-entry -Wno-shift-op-parentheses -o $@ $(filter %.cpp,$^)

dist/toypop.png.js: $(addprefix roms/,toypop.js toypop.zip)
	node $^ $@

dist/twinbee.wasm: $(addprefix src/,twinbee.cpp mc68000.cpp z80.cpp cpu.cpp vlm5030.cpp twinbee.h mc68000.h z80.h cpu.h ay-3-8910.h k005289.h vlm5030.h utils.h)
	emcc --std=c++20 -O3 -s INITIAL_MEMORY=33554432 --no-entry -Wno-shift-op-parentheses -o $@ $(filter %.cpp,$^)

dist/twinbee.png.js: $(addprefix roms/,twinbee.js twinbee.zip)
	node $^ $@

dist/vulgus.wasm: $(addprefix src/,vulgus.cpp z80.cpp cpu.cpp vulgus.h z80.h cpu.h ay-3-8910.h utils.h)
	emcc --std=c++20 -O3 --no-entry -Wno-shift-op-parentheses -o $@ $(filter %.cpp,$^)

dist/vulgus.png.js: $(addprefix roms/,vulgus.js vulgus.zip)
	node $^ $@

dist/zigzag.wasm: $(addprefix src/,zigzag.cpp z80.cpp cpu.cpp zigzag.h z80.h cpu.h ay-3-8910.h utils.h)
	emcc --std=c++20 -O3 --no-entry -Wno-shift-op-parentheses -o $@ $(filter %.cpp,$^)

dist/zigzag.png.js: $(addprefix roms/,zigzag.js zigzagb.zip)
	node $^ $@

