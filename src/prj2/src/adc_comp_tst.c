#include "compensator.h"
#include <stdio.h>


int main()
{
	comp_model cm;
	comp_init_model(COMP_PATH, &cm);

	printf("%d\n", cm.n);
	for(uint32_t i=0; i< cm.n; ++i){
		printf("p:%f q:%f ag:%f bg:%f rng_b:%d rng_e%d\n", cm.lut[i].p, cm.lut[i].q, 
			cm.lut[i].ag, cm.lut[i].bg, cm.lut[i].range_begin, cm.lut[i].range_end);			
	}

	for(uint32_t i=0; i<cm.n; ++i){
		float val_comp = comp_compensate(&cm, cm.lut[i].range_begin);
		printf("raw: %d comp: %f\n", cm.lut[i].range_begin, val_comp);
	}


	float val_comp = comp_compensate(&cm, cm.lut[(cm.n)-1].range_end-1);
	printf("raw: %d comp: %f\n", cm.lut[cm.n-1].range_end-1, val_comp);

	comp_destroy_model(&cm);

	return 0;
}
