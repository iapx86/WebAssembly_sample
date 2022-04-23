targets = \
	1942.wasm.js baraduke.wasm.js chackn_pop.wasm.js crush_roller.wasm.js digdug.wasm.js digdug_ii.wasm.js \
	dragon_buster.wasm.js elevator_action.wasm.js frogger.wasm.js gradius.wasm.js grobda.wasm.js libble_rabble.wasm.js \
	mappy.wasm.js metro-cross.wasm.js motos.wasm.js pac-land.wasm.js pac-man.wasm.js pac_and_pal.wasm.js pengo.wasm.js \
	phozon.wasm.js sea_fighter_poseidon.wasm.js sky_kid.wasm.js star_force.wasm.js strategy_x.wasm.js \
	super_pac-man.wasm.js the_tower_of_druaga.wasm.js time_pilot.wasm.js time_tunnel.wasm.js toypop.wasm.js \
	twinbee.wasm.js vulgus.wasm.js zigzag.wasm.js
targets += \
	1942_rom.js baraduke_rom.js chackn_pop_rom.js crush_roller_rom.js digdug_rom.js digdug_ii_rom.js \
	dragon_buster_rom.js elevator_action_rom.js frogger_rom.js gradius_rom.js grobda_rom.js libble_rabble_rom.js \
	mappy_rom.js metro-cross_rom.js motos_rom.js pac-land_rom.js pac-man_rom.js pac_and_pal_rom.js pengo_rom.js \
	phozon_rom.js sea_fighter_poseidon_rom.js sky_kid_rom.js star_force_rom.js strategy_x_rom.js super_pac-man_rom.js \
	the_tower_of_druaga_rom.js time_pilot_rom.js time_tunnel_rom.js toypop_rom.js twinbee_rom.js vulgus_rom.js \
	zigzag_rom.js

.PHONY: all
all: dist $(addprefix dist/,$(targets))

.PHONY: clean
clean:
	del dist\*.wasm*
	del dist\*_rom.js

dist:
	mkdir dist

%.wasm.js: %.wasm
	powershell -ExecutionPolicy RemoteSigned -File convert.ps1 $< $@

dist/1942.wasm: $(addprefix src/,1942.cpp z80.cpp cpu.cpp 1942.h z80.h cpu.h ay-3-8910.h utils.h)
	emcc --std=c++20 -O3 --no-entry -Wno-shift-op-parentheses -o $@ $(filter %.cpp,$^)

dist/1942_rom.js: $(addprefix roms/,1942.zip)
	powershell -ExecutionPolicy RemoteSigned -File 1942.ps1 $^ $@

dist/baraduke.wasm: $(addprefix src/,baraduke.cpp mc6809.cpp mc6801.cpp cpu.cpp baraduke.h mc6809.h mc6801.h cpu.h c30.h utils.h)
	emcc --std=c++20 -O3 --no-entry -Wno-shift-op-parentheses -o $@ $(filter %.cpp,$^)

dist/baraduke_rom.js: $(addprefix roms/,aliensec.zip)
	powershell -ExecutionPolicy RemoteSigned -File baraduke.ps1 $^ $@

dist/chackn_pop.wasm: $(addprefix src/,chackn_pop.cpp z80.cpp mc6805.cpp cpu.cpp chackn_pop.h z80.h mc6805.h cpu.h ay-3-8910.h utils.h)
	emcc --std=c++20 -O3 --no-entry -Wno-shift-op-parentheses -o $@ $(filter %.cpp,$^)

dist/chackn_pop_rom.js: $(addprefix roms/,chaknpop.zip)
	powershell -ExecutionPolicy RemoteSigned -File chackn_pop.ps1 $^ $@

dist/crush_roller.wasm: $(addprefix src/,crush_roller.cpp z80.cpp cpu.cpp crush_roller.h z80.h cpu.h pac-man_sound.h utils.h)
	emcc --std=c++20 -O3 --no-entry -Wno-shift-op-parentheses -o $@ $(filter %.cpp,$^)

dist/crush_roller_rom.js: $(addprefix roms/,crush.zip)
	powershell -ExecutionPolicy RemoteSigned -File crush_roller.ps1 $^ $@

dist/digdug.wasm: $(addprefix src/,digdug.cpp z80.cpp mb8840.cpp cpu.cpp digdug.h z80.h mb8840.h cpu.h pac-man_sound.h utils.h)
	emcc --std=c++20 -O3 --no-entry -Wno-shift-op-parentheses -o $@ $(filter %.cpp,$^)

dist/digdug_rom.js: $(addprefix roms/,digdug.zip namco51.zip)
	powershell -ExecutionPolicy RemoteSigned -File digdug.ps1 $^ $@

dist/digdug_ii.wasm: $(addprefix src/,digdug_ii.cpp mc6809.cpp cpu.cpp digdug_ii.h mc6809.h cpu.h mappy_sound.h utils.h)
	emcc --std=c++20 -O3 --no-entry -Wno-shift-op-parentheses -o $@ $(filter %.cpp,$^)

dist/digdug_ii_rom.js: $(addprefix roms/,digdug2.zip)
	powershell -ExecutionPolicy RemoteSigned -File digdug_ii.ps1 $^ $@

dist/dragon_buster.wasm: $(addprefix src/,dragon_buster.cpp mc6809.cpp mc6801.cpp cpu.cpp dragon_buster.h mc6809.h mc6801.h cpu.h c30.h utils.h)
	emcc --std=c++20 -O3 --no-entry -Wno-shift-op-parentheses -o $@ $(filter %.cpp,$^)

dist/dragon_buster_rom.js: $(addprefix roms/,drgnbstr.zip)
	powershell -ExecutionPolicy RemoteSigned -File dragon_buster.ps1 $^ $@

dist/elevator_action.wasm: $(addprefix src/,elevator_action.cpp z80.cpp mc6805.cpp cpu.cpp elevator_action.h z80.h mc6805.h cpu.h ay-3-8910.h dac.h utils.h)
	emcc --std=c++20 -O3 --no-entry -Wno-shift-op-parentheses -o $@ $(filter %.cpp,$^)

dist/elevator_action_rom.js: $(addprefix roms/,elevator.zip)
	powershell -ExecutionPolicy RemoteSigned -File elevator_action.ps1 $^ $@

dist/frogger.wasm: $(addprefix src/,frogger.cpp z80.cpp cpu.cpp frogger.h z80.h cpu.h ay-3-8910.h utils.h)
	emcc --std=c++20 -O3 --no-entry -Wno-shift-op-parentheses -o $@ $(filter %.cpp,$^)

dist/frogger_rom.js: $(addprefix roms/,frogger.zip)
	powershell -ExecutionPolicy RemoteSigned -File frogger.ps1 $^ $@

dist/gradius.wasm: $(addprefix src/,gradius.cpp mc68000.cpp z80.cpp cpu.cpp vlm5030.cpp gradius.h mc68000.h z80.h cpu.h ay-3-8910.h k005289.h vlm5030.h utils.h)
	emcc --std=c++20 -O3 -s INITIAL_MEMORY=33554432 --no-entry -Wno-shift-op-parentheses -o $@ $(filter %.cpp,$^)

dist/gradius_rom.js: $(addprefix roms/,nemesis.zip)
	powershell -ExecutionPolicy RemoteSigned -File gradius.ps1 $^ $@

dist/grobda.wasm: $(addprefix src/,grobda.cpp mc6809.cpp cpu.cpp grobda.h mc6809.h cpu.h mappy_sound.h utils.h)
	emcc --std=c++20 -O3 --no-entry -Wno-shift-op-parentheses -o $@ $(filter %.cpp,$^)

dist/grobda_rom.js: $(addprefix roms/,grobda.zip)
	powershell -ExecutionPolicy RemoteSigned -File grobda.ps1 $^ $@

dist/libble_rabble.wasm: $(addprefix src/,libble_rabble.cpp mc6809.cpp mc68000.cpp cpu.cpp libble_rabble.h mc6809.h mc68000.h cpu.h mappy_sound.h utils.h)
	emcc --std=c++20 -O3 -s INITIAL_MEMORY=33554432 --no-entry -Wno-shift-op-parentheses -o $@ $(filter %.cpp,$^)

dist/libble_rabble_rom.js: $(addprefix roms/,liblrabl.zip)
	powershell -ExecutionPolicy RemoteSigned -File libble_rabble.ps1 $^ $@

dist/mappy.wasm: $(addprefix src/,mappy.cpp mc6809.cpp cpu.cpp mappy.h mc6809.h cpu.h mappy_sound.h utils.h)
	emcc --std=c++20 -O3 --no-entry -Wno-shift-op-parentheses -o $@ $(filter %.cpp,$^)

dist/mappy_rom.js: $(addprefix roms/,mappy.zip)
	powershell -ExecutionPolicy RemoteSigned -File mappy.ps1 $^ $@

dist/metro-cross.wasm: $(addprefix src/,metro-cross.cpp mc6809.cpp mc6801.cpp cpu.cpp metro-cross.h mc6809.h mc6801.h cpu.h c30.h utils.h utils.h)
	emcc --std=c++20 -O3 --no-entry -Wno-shift-op-parentheses -o $@ $(filter %.cpp,$^)

dist/metro-cross_rom.js: $(addprefix roms/,metrocrs.zip)
	powershell -ExecutionPolicy RemoteSigned -File metro-cross.ps1 $^ $@

dist/motos.wasm: $(addprefix src/,motos.cpp mc6809.cpp cpu.cpp motos.h mc6809.h cpu.h mappy_sound.h utils.h)
	emcc --std=c++20 -O3 --no-entry -Wno-shift-op-parentheses -o $@ $(filter %.cpp,$^)

dist/motos_rom.js: $(addprefix roms/,motos.zip)
	powershell -ExecutionPolicy RemoteSigned -File motos.ps1 $^ $@

dist/pac-land.wasm: $(addprefix src/,pac-land.cpp mc6809.cpp mc6801.cpp cpu.cpp pac-land.h mc6809.h mc6801.h cpu.h c30.h utils.h)
	emcc --std=c++20 -O3 --no-entry -Wno-shift-op-parentheses -o $@ $(filter %.cpp,$^)

dist/pac-land_rom.js: $(addprefix roms/,pacland.zip)
	powershell -ExecutionPolicy RemoteSigned -File pac-land.ps1 $^ $@

dist/pac-man.wasm: $(addprefix src/,pac-man.cpp z80.cpp cpu.cpp pac-man.h z80.h cpu.h pac-man_sound.h utils.h)
	emcc --std=c++20 -O3 --no-entry -Wno-shift-op-parentheses -o $@ $(filter %.cpp,$^)

dist/pac-man_rom.js: $(addprefix roms/,puckman.zip)
	powershell -ExecutionPolicy RemoteSigned -File pac-man.ps1 $^ $@

dist/pac_and_pal.wasm: $(addprefix src/,pac_and_pal.cpp mc6809.cpp cpu.cpp pac_and_pal.h mc6809.h cpu.h mappy_sound.h utils.h)
	emcc --std=c++20 -O3 --no-entry -Wno-shift-op-parentheses -o $@ $(filter %.cpp,$^)

dist/pac_and_pal_rom.js: $(addprefix roms/,pacnpal.zip)
	powershell -ExecutionPolicy RemoteSigned -File pac_and_pal.ps1 $^ $@

dist/pengo.wasm: $(addprefix src/,pengo.cpp sega_z80.cpp z80.cpp cpu.cpp pengo.h sega_z80.h z80.h cpu.h pac-man_sound.h utils.h)
	emcc --std=c++20 -O3 --no-entry -Wno-shift-op-parentheses -o $@ $(filter %.cpp,$^)

dist/pengo_rom.js: $(addprefix roms/,pengo.zip)
	powershell -ExecutionPolicy RemoteSigned -File pengo.ps1 $^ $@

dist/phozon.wasm: $(addprefix src/,phozon.cpp mc6809.cpp cpu.cpp phozon.h mc6809.h cpu.h mappy_sound.h utils.h)
	emcc --std=c++20 -O3 --no-entry -Wno-shift-op-parentheses -o $@ $(filter %.cpp,$^)

dist/phozon_rom.js: $(addprefix roms/,phozon.zip)
	powershell -ExecutionPolicy RemoteSigned -File phozon.ps1 $^ $@

dist/sea_fighter_poseidon.wasm: $(addprefix src/,sea_fighter_poseidon.cpp z80.cpp mc6805.cpp cpu.cpp sea_fighter_poseidon.h z80.h mc6805.h cpu.h ay-3-8910.h dac.h utils.h)
	emcc --std=c++20 -O3 --no-entry -Wno-shift-op-parentheses -o $@ $(filter %.cpp,$^)

dist/sea_fighter_poseidon_rom.js: $(addprefix roms/,sfposeid.zip)
	powershell -ExecutionPolicy RemoteSigned -File sea_fighter_poseidon.ps1 $^ $@

dist/sky_kid.wasm: $(addprefix src/,sky_kid.cpp mc6809.cpp mc6801.cpp cpu.cpp sky_kid.h mc6809.h mc6801.h cpu.h c30.h utils.h)
	emcc --std=c++20 -O3 --no-entry -Wno-shift-op-parentheses -o $@ $(filter %.cpp,$^)

dist/sky_kid_rom.js: $(addprefix roms/,skykid.zip)
	powershell -ExecutionPolicy RemoteSigned -File sky_kid.ps1 $^ $@

dist/star_force.wasm: $(addprefix src/,star_force.cpp z80.cpp cpu.cpp star_force.h z80.h cpu.h sn76489.h senjyo_sound.h utils.h)
	emcc --std=c++20 -O3 --no-entry -Wno-shift-op-parentheses -o $@ $(filter %.cpp,$^)

dist/star_force_rom.js: $(addprefix roms/,starforc.zip)
	powershell -ExecutionPolicy RemoteSigned -File star_force.ps1 $^ $@

dist/strategy_x.wasm: $(addprefix src/,strategy_x.cpp z80.cpp cpu.cpp strategy_x.h z80.h cpu.h ay-3-8910.h utils.h)
	emcc --std=c++20 -O3 --no-entry -Wno-shift-op-parentheses -o $@ $(filter %.cpp,$^)

dist/strategy_x_rom.js: $(addprefix roms/,stratgyx.zip)
	powershell -ExecutionPolicy RemoteSigned -File strategy_x.ps1 $^ $@

dist/super_pac-man.wasm: $(addprefix src/,super_pac-man.cpp mc6809.cpp cpu.cpp super_pac-man.h mc6809.h cpu.h mappy_sound.h utils.h)
	emcc --std=c++20 -O3 --no-entry -Wno-shift-op-parentheses -o $@ $(filter %.cpp,$^)

dist/super_pac-man_rom.js: $(addprefix roms/,superpac.zip)
	powershell -ExecutionPolicy RemoteSigned -File super_pac-man.ps1 $^ $@

dist/the_tower_of_druaga.wasm: $(addprefix src/,the_tower_of_druaga.cpp mc6809.cpp cpu.cpp the_tower_of_druaga.h mc6809.h cpu.h mappy_sound.h utils.h)
	emcc --std=c++20 -O3 --no-entry -Wno-shift-op-parentheses -o $@ $(filter %.cpp,$^)

dist/the_tower_of_druaga_rom.js: $(addprefix roms/,todruaga.zip)
	powershell -ExecutionPolicy RemoteSigned -File the_tower_of_druaga.ps1 $^ $@

dist/time_pilot.wasm: $(addprefix src/,time_pilot.cpp z80.cpp cpu.cpp time_pilot.h z80.h cpu.h ay-3-8910.h utils.h)
	emcc --std=c++20 -O3 --no-entry -Wno-shift-op-parentheses -o $@ $(filter %.cpp,$^)

dist/time_pilot_rom.js: $(addprefix roms/,timeplt.zip)
	powershell -ExecutionPolicy RemoteSigned -File time_pilot.ps1 $^ $@

dist/time_tunnel.wasm: $(addprefix src/,time_tunnel.cpp z80.cpp cpu.cpp time_tunnel.h z80.h cpu.h ay-3-8910.h dac.h utils.h)
	emcc --std=c++20 -O3 --no-entry -Wno-shift-op-parentheses -o $@ $(filter %.cpp,$^)

dist/time_tunnel_rom.js: $(addprefix roms/,timetunl.zip)
	powershell -ExecutionPolicy RemoteSigned -File time_tunnel.ps1 $^ $@

dist/toypop.wasm: $(addprefix src/,toypop.cpp mc6809.cpp mc68000.cpp cpu.cpp toypop.h mc6809.h mc68000.h cpu.h mappy_sound.h utils.h)
	emcc --std=c++20 -O3 -s INITIAL_MEMORY=33554432 --no-entry -Wno-shift-op-parentheses -o $@ $(filter %.cpp,$^)

dist/toypop_rom.js: $(addprefix roms/,toypop.zip)
	powershell -ExecutionPolicy RemoteSigned -File toypop.ps1 $^ $@

dist/twinbee.wasm: $(addprefix src/,twinbee.cpp mc68000.cpp z80.cpp cpu.cpp vlm5030.cpp twinbee.h mc68000.h z80.h cpu.h ay-3-8910.h k005289.h vlm5030.h utils.h)
	emcc --std=c++20 -O3 -s INITIAL_MEMORY=33554432 --no-entry -Wno-shift-op-parentheses -o $@ $(filter %.cpp,$^)

dist/twinbee_rom.js: $(addprefix roms/,twinbee.zip)
	powershell -ExecutionPolicy RemoteSigned -File twinbee.ps1 $^ $@

dist/vulgus.wasm: $(addprefix src/,vulgus.cpp z80.cpp cpu.cpp vulgus.h z80.h cpu.h ay-3-8910.h utils.h)
	emcc --std=c++20 -O3 --no-entry -Wno-shift-op-parentheses -o $@ $(filter %.cpp,$^)

dist/vulgus_rom.js: $(addprefix roms/,vulgus.zip)
	powershell -ExecutionPolicy RemoteSigned -File vulgus.ps1 $^ $@

dist/zigzag.wasm: $(addprefix src/,zigzag.cpp z80.cpp cpu.cpp zigzag.h z80.h cpu.h ay-3-8910.h utils.h)
	emcc --std=c++20 -O3 --no-entry -Wno-shift-op-parentheses -o $@ $(filter %.cpp,$^)

dist/zigzag_rom.js: $(addprefix roms/,zigzagb.zip)
	powershell -ExecutionPolicy RemoteSigned -File zigzag.ps1 $^ $@

