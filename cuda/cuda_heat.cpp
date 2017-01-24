#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#define STEPS 100

extern "C" void update(float*, float*, int , int);

void inidat0(int nx, int ny, float *u) {
int ix, iy;
for (ix = 0; ix <= nx-1; ix++)
  for (iy = 0; iy <= ny-1; iy++)
	 *(u+ix*ny+iy) = (float)(ix * (nx - ix - 1) * iy * (ny - iy - 1));
}

void inidat1(int nx, int ny, float *u) {
	int ix, iy;

	for (ix = 0; ix < nx; ix++)
		for (iy = 0; iy < ny; iy++)
			*(u + ix * ny + iy) = 0;
}

int main(int argc, char* argv[]) {
	int iz, i;
	clock_t s, e;
	float *u0, *u1;
	int NXPROB, NYPROB;

	NXPROB = atoi(argv[1]);
	NYPROB = atoi(argv[2]);

	u0 = (float*) malloc(NXPROB * NYPROB * sizeof(float));
	u1 = (float*) malloc(NXPROB * NYPROB * sizeof(float));

	inidat0(NXPROB,NYPROB,u0);
	inidat1(NXPROB,NYPROB,u1);
	


	iz = 0;
	s = clock();
	for(i = 0; i < STEPS; i++){
		if(iz == 0)
			update(u0, u1, NXPROB, NYPROB);
		else
			update(u1, u0, NXPROB, NYPROB);
		iz = 1 - iz;
	}
	e = clock();
	
	printf("[%d][%d]: Time elapsed: %f seconds\n", NXPROB, NYPROB, (float)(e-s)/CLOCKS_PER_SEC);
	
	free(u0);
	free(u1);
	
	return 0;
}

