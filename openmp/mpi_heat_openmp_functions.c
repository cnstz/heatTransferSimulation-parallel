#include "mpi_heat_openmp_functions.h"
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <omp.h>

struct Parms {
  float cx;
  float cy;
} parms = {0.1, 0.1};

void decompose1d(int n, int m, int i, int* s, int* e) {
	const int length  = n / m;
	const int deficit = n % m;
	*s =  i * length + ( i < deficit ? i : deficit );
	*e = *s + length - ( i < deficit ? 0 : 1 );
	if ( ( *e >= n ) || ( i == m - 1 ) )
		*e = n - 1;
}


char *itoa(int num, char *str) {
	if (str == NULL)
		return NULL;
	sprintf(str, "%d", num);
	return str;
}

void inidat0(int nx, int ny, int task_X, int task_Y, int task_X_start, int task_X_end, int task_Y_start, int task_Y_end, float *u, int neighbors[4]) {
	int ix, iy, x_pos, y_pos;
	int x_start, x_end, y_start, y_end;
	x_start = task_X_start;
	x_end = task_X_end;
	y_start = task_Y_start;
	y_end = task_Y_end;

	if (neighbors[LEFT] != NONE) {
		y_start++;
		y_end++;
	}
	if (neighbors[UP] != NONE) {
		x_start++;
		x_end++;
	}

	for (ix = x_start; ix <= x_end; ix++) {
		x_pos = ix - task_X_start;
		for (iy = y_start; iy <= y_end; iy++) {
			y_pos = iy - task_Y_start;
			if (neighbors[LEFT] != NONE && neighbors[UP] != NONE)
				*(u + x_pos * task_Y + y_pos) = (float)((ix - 1) * (nx - ix) * (iy - 1) * (ny - iy));
			else if (neighbors[LEFT] != NONE)
				*(u + x_pos * task_Y + y_pos) = (float)((ix) * (nx - ix - 1) * (iy - 1) * (ny - iy));
			else if (neighbors[UP] != NONE)
				*(u + x_pos * task_Y + y_pos) = (float)((ix - 1) * (nx - ix) * (iy) * (ny - iy - 1));
			else
				*(u + x_pos * task_Y + y_pos) = (float)((ix) * (nx - ix - 1) * (iy) * (ny - iy - 1));
		}
	}
}

void inidat1(int task_X, int task_Y, float *u) {
	int ix, iy;

	for (ix = 0; ix < task_X; ix++)
		for (iy = 0; iy < task_Y; iy++)
			*(u + ix * task_Y + iy) = 0;

}

void prtdat(int nx, int ny, float * u1, char *fnam) {
	int ix, iy;
	FILE *fp;

	fp = fopen(fnam, "w");
	for (ix = 0; ix < nx; ix++) {
		for (iy = 0; iy < ny; iy++)
			fprintf(fp, "%6.1f\t", *(u1 + ix * ny + iy));
		fprintf(fp, "\n");
	}

	fclose(fp);
}

void update(int x_start, int x_end, int y_start, int y_end, int ny, float *u1, float *u2) {
	int ix, iy;
	#pragma omp parallel for collapse(2) private(ix, iy)
	for (ix = x_start; ix <= x_end; ix++)
		for (iy = y_start; iy <= y_end; iy++)
			*(u2 + ix * ny + iy) = *(u1 + ix * ny + iy)  +
			                       parms.cx * (*(u1 + (ix + 1) * ny + iy) +
			                                   * (u1 + (ix - 1) * ny + iy) -
			                                   2.0 * *(u1 + ix * ny + iy)) +
			                       parms.cy * (*(u1 + ix * ny + iy + 1) +
			                                   * (u1 + ix * ny + iy - 1) -
			                                   2.0 * *(u1 + ix * ny + iy));
}

float round_float(float f) {
	return roundf(f * 100) / 100;
}

int check_convergence(int x_start, int x_end, int y_start, int y_end, int ny, float *u1, float *u2) {
	int ix, iy;
	#pragma omp parallel for collapse(2) private(ix, iy)
	for (ix = x_start; ix <= x_end - 1; ix++)
		for (iy = y_start; iy <= y_end - 1; iy++)
			if (round_float(*(u1 + ix * ny + iy)) != round_float(*(u2 + ix * ny + iy)))
				return 0; // not converged

	return 1;	// converged
}
