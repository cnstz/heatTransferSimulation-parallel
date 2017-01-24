#ifndef MPI_HEAT_FUNCTIONS_H
#define MPI_HEAT_FUNCTIONS_H

#define UP          0
#define DOWN        1
#define LEFT        2
#define RIGHT       3
#define NONE        -1

void decompose1d(int n, int m, int i, int* s, int* e);	// returns start/end coordinates in s/e
char *itoa(int num, char *str);	// int to string
void inidat0(int nx, int ny, int task_X, int task_Y, int task_X_start, int task_X_end, int task_Y_start, int task_Y_end, float *u, int neighbors[4]);
void inidat1(int task_X, int task_Y, float *u);	// initializes *u[1] to 0.0
void prtdat(int nx, int ny, float *u1, char *fnam);
void update(int x_start, int x_end, int y_start, int y_end, int ny, float *u1, float *u2);
int check_convergence(int x_start, int x_end, int y_start, int y_end, int ny, float *u1, float *u2);	// checks for convergence between *u[0] and *u[1]

#endif /* "MPI_HEAT_FUNCTIONS_H" */