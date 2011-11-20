cd ..
mkdir generated
cd scripts
perl cave_sprite_func.pl ../generated/cave_sprite_func.h
perl cave_tile_func.pl ../generated/cave_tile_func.h
perl neo_sprite_func.pl ../generated/neo_sprite_func.h
perl psikyo_tile_func.pl ../generated/psikyo_tile_func.h
perl toa_gp9001_func.pl ../generated/toa_gp9001_func.h
perl gamelist.pl -o ../generated/driverlist.h -l ../../gamelist.txt ../../burn/drivers/capcom ../../burn/drivers/cave ../../burn/drivers/cps3 ../../burn/drivers/galaxian ../../burn/drivers/irem ../../burn/drivers/konami ../../burn/drivers/megadrive ../../burn/drivers/misc_post90s ../../burn/drivers/misc_pre90s ../../burn/drivers/neogeo ../../burn/drivers/pgm ../../burn/drivers/psikyo ../../burn/drivers/sega ../../burn/drivers/taito ../../burn/drivers/toaplan 
 