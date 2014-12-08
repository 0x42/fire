
#include <stdio.h>
#include "ocfg.h"


int main(int argc, char *argv[])
{
	TOHT *life;
	TOHT *wdlifes;
	FILE *out;
	char slife[17] = {0};
	int master_en;
	int sch_life_master, sch_life_mslave;
	int master_life, mslave_life;
	char *wdfile = "/mnt/ramdisk/wd.lifes";
	char *masterfile = "/mnt/ramdisk/master.life";
	char *slavefile = "/mnt/ramdisk/mslave.life";
	int res = 0;

	wdlifes = cfg_load(wdfile);
	master_en = cfg_getint(wdlifes, "WD:master", -1);
	if (master_en) {
		master_life = cfg_getint(wdlifes, "WD:master_life", -1);
		life = cfg_load(masterfile);
		sch_life_master = cfg_getint(life, "WD:life", -1);
		if (sch_life_master != master_life) {
			sprintf(slife, "%016d", sch_life_master);
			if ((out = fopen(wdfile, "w")) == NULL) {
				fprintf(stderr, "get_cron_life(): cannot open %s\n", wdfile);
				res = 1;
			} else {
				cfg_put(wdlifes, "WD:master_life", slife);
				cfg_save(wdlifes, out);
				fclose(out);
			}
		} else
			res = 1;
		
		cfg_free(life);
	}
	
	mslave_life = cfg_getint(wdlifes, "WD:mslave_life", -1);
	life = cfg_load(slavefile);
	sch_life_mslave = cfg_getint(life, "WD:life", -1);
	if (sch_life_mslave != mslave_life) {
		sprintf(slife, "%016d", sch_life_mslave);
	
		if ((out = fopen(wdfile, "w")) == NULL) {
			fprintf(stderr, "get_cron_life(): cannot open %s\n", wdfile);
			res = 1;
		} else {
			cfg_put(wdlifes, "WD:mslave_life", slife);
			cfg_save(wdlifes, out);
			fclose(out);
		}
	} else
		res = 1;
	
	cfg_free(life);
	cfg_free(wdlifes);

	if (res)
		system("reboot");

	return res;
}

