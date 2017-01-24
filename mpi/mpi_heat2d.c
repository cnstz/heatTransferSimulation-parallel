#include "mpi.h"
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include "mpi_heat_functions.h"

#define NXPROB      20                 /* x dimension of problem grid */
#define NYPROB      20                 /* y dimension of problem grid */
#define STEPS       100                /* number of time steps */
#define UP          0
#define DOWN        1
#define LEFT        2
#define RIGHT       3
#define NONE        -1

#define CONVERGENCE 0
#define CONVERGENCE_n 10

int main (int argc, char *argv[]) {
	/* Variables declaration */
	int number_of_tasks,
	    task_X, task_Y, // dimensions for tasks array
	    task_X_start, task_X_end, task_Y_start, task_Y_end, // coordinates for tasks array
	    neighbors[4],  // UP, DOWN, LEFT, RIGHT
	    rank, // rank of task
	    ndims, dims[2], periods[2], reorder,  // used to create cartesian communicator
	    coords[2],  // coords of task in virtual topology
	    rc, iz, step, //
	    task_convergence, reduced_convergence;  // used for convergence
	double start_time, end_time, task_time, reduced_time; // used for timing
	float ***u;
	/*************************/

	/* MPI variables declaration */
	MPI_Datatype MPI_row, // contiguous datatype for arrays rows
	             MPI_column;  // vector datatype for arrays columns
	MPI_Comm MPI_COMM_CARTESIAN;  // cartesian grid
	MPI_Request request_send[4];  // requests for sending
	MPI_Request request_receive[4]; // requests for receiving
	/*****************************/

	/* MPI initialization */
	MPI_Init(&argc, &argv);
	MPI_Comm_size(MPI_COMM_WORLD, &number_of_tasks);
	/**********************/

	int number_of_tasks_sqrt = sqrt(number_of_tasks);

	/* Check number of tasks square root */
	if (number_of_tasks_sqrt * number_of_tasks_sqrt != number_of_tasks) {
		printf("ERROR: the square root of number of tasks must be an integer\n");
		printf("Quitting...\n");
		MPI_Abort(MPI_COMM_WORLD, rc);
		exit(1);
	}
	/************************/

	/* MPI cartesian grid */
	ndims = 2;
	dims[0] = dims[1] = number_of_tasks_sqrt;
	periods[0] = periods[1] = 0;
	reorder = 0;
	MPI_Cart_create(MPI_COMM_WORLD, ndims, dims, periods, reorder, &MPI_COMM_CARTESIAN);
	MPI_Comm_rank(MPI_COMM_CARTESIAN, &rank);
	MPI_Cart_coords(MPI_COMM_CARTESIAN, rank, 2, coords);
	/************************/

	/* Dimensions and coordinates for tasks sub-arrays */
	decompose1d(NXPROB, dims[0], coords[0], &task_X_start, &task_X_end);
	decompose1d(NYPROB, dims[1], coords[1], &task_Y_start, &task_Y_end);
	task_X = task_X_end - task_X_start + 1;
	task_Y = task_Y_end - task_Y_start + 1;
	/***************************************************/

	/* Check for neighbors */
	MPI_Cart_shift(MPI_COMM_CARTESIAN, 1, 1, &neighbors[LEFT], &neighbors[RIGHT]);
	MPI_Cart_shift(MPI_COMM_CARTESIAN, 0, 1, &neighbors[UP] , &neighbors[DOWN]);
	/********************************/

	/* Change array dimensions according to neighbors */
	if (neighbors[LEFT] != NONE)
		task_Y++;
	if (neighbors[RIGHT] != NONE)
		task_Y++;
	if (neighbors[UP] != NONE)
		task_X++;
	if (neighbors[DOWN] != NONE)
		task_X++;
	/****************************************************/

	/* Custom MPI types for array rows and columns */
	MPI_Type_contiguous(task_Y, MPI_FLOAT, &MPI_row);
	MPI_Type_commit(&MPI_row);
	MPI_Type_vector(task_X, 1, task_Y, MPI_FLOAT, &MPI_column);
	MPI_Type_commit(&MPI_column);
	/*************************************************/

	/* Initialize u array (2nd/3rd dimension is contiguous)*/
	u = malloc(2 * sizeof(float**));
	u[0] = malloc(task_X * sizeof(float*));
	u[1] = malloc(task_X * sizeof(float*));
	float* u0Data = malloc(task_X * task_Y * sizeof(float));
	float* u1Data = malloc(task_X * task_Y * sizeof(float));
	int i = 0;
	for (i = 0; i < task_X; i++) {
		u[0][i] = u0Data + i * task_Y;
		u[1][i] = u1Data + i * task_Y;
	}
	inidat0(NXPROB, NYPROB, task_X, task_Y, task_X_start, task_X_end, task_Y_start, task_Y_end, *u[0], neighbors);
	inidat1(task_X, task_Y, *u[1]);
	/**********************/

	MPI_Barrier(MPI_COMM_CARTESIAN);

	/* Start timer */
	start_time = MPI_Wtime();
	/***************/

	iz = 0;
#if (CONVERGENCE == 1)
	task_convergence = reduced_convergence = 0;
#endif

	for (step = 0; step <= STEPS; step++) {
		/* Send rows/columns to neighbors */
		if (neighbors[LEFT] != NONE)
			MPI_Isend(&u[iz][0][1], 1, MPI_column, neighbors[LEFT], RIGHT, MPI_COMM_CARTESIAN, &request_send[LEFT]);
		if (neighbors[RIGHT] != NONE)
			MPI_Isend(&u[iz][0][task_Y - 2], 1, MPI_column, neighbors[RIGHT], LEFT, MPI_COMM_CARTESIAN, &request_send[RIGHT]);
		if (neighbors[UP] != NONE)
			MPI_Isend(&u[iz][1][0], 1, MPI_row, neighbors[UP], DOWN, MPI_COMM_CARTESIAN, &request_send[UP]);
		if (neighbors[DOWN] != NONE)
			MPI_Isend(&u[iz][task_X - 2][0], 1, MPI_row, neighbors[DOWN], UP, MPI_COMM_CARTESIAN, &request_send[DOWN]);
		/*********************************/

		/*Receive rows/columns from neighbors */
		if (neighbors[LEFT] != NONE)
			MPI_Irecv(&u[iz][0][0], 1, MPI_column, neighbors[LEFT], LEFT, MPI_COMM_CARTESIAN, &request_receive[LEFT]);
		if (neighbors[RIGHT] != NONE)
			MPI_Irecv(&u[iz][0][task_Y - 1], 1, MPI_column, neighbors[RIGHT], RIGHT, MPI_COMM_CARTESIAN, &request_receive[RIGHT]);
		if (neighbors[UP] != NONE)
			MPI_Irecv(&u[iz][0][0], 1, MPI_row, neighbors[UP], UP, MPI_COMM_CARTESIAN, &request_receive[UP]);
		if (neighbors[DOWN] != NONE)
			MPI_Irecv(&u[iz][task_X - 1][0], 1, MPI_row, neighbors[DOWN], DOWN, MPI_COMM_CARTESIAN, &request_receive[DOWN]);
		/*************************************/

		/* Wait to receive any rows/columns*/
		if (neighbors[LEFT] != NONE)
			MPI_Wait(&request_receive[LEFT], MPI_STATUS_IGNORE);
		if (neighbors[RIGHT] != NONE)
			MPI_Wait(&request_receive[RIGHT], MPI_STATUS_IGNORE);
		if (neighbors[UP] != NONE)
			MPI_Wait(&request_receive[UP], MPI_STATUS_IGNORE);
		if (neighbors[DOWN] != NONE)
			MPI_Wait(&request_receive[DOWN], MPI_STATUS_IGNORE);
		/********************/

		/* Update */
		update(1, task_X - 2, 1, task_Y - 2, task_Y, *u[iz], *u[1 - iz]);
		/***********/

		/* Wait to send any rows/columns*/
		if (neighbors[LEFT] != NONE)
			MPI_Wait(&request_send[LEFT], MPI_STATUS_IGNORE);
		if (neighbors[RIGHT] != NONE)
			MPI_Wait(&request_send[RIGHT], MPI_STATUS_IGNORE);
		if (neighbors[UP] != NONE)
			MPI_Wait(&request_send[UP], MPI_STATUS_IGNORE);
		if (neighbors[DOWN] != NONE)
			MPI_Wait(&request_send[DOWN], MPI_STATUS_IGNORE);
		/********************/

		iz = 1 - iz;

		/* Convergence check */
#if (CONVERGENCE == 1)
		if (step % CONVERGENCE_n == 0) {
			task_convergence = check_convergence(1, task_X - 2, 1, task_Y - 2, task_Y, *u[iz], *u[1 - iz]);
			MPI_Barrier(MPI_COMM_CARTESIAN);
			MPI_Allreduce(&task_convergence, &reduced_convergence, 1, MPI_INT, MPI_LAND, MPI_COMM_CARTESIAN);
			if (reduced_convergence == 1) {
				break;
			}
		}
#endif
		/************************/
	}

	/* Stop timer, calculate duration, reduce */
	end_time = MPI_Wtime();
	task_time = end_time - start_time;
	MPI_Barrier(MPI_COMM_CARTESIAN);
	MPI_Reduce(&task_time, &reduced_time, 1, MPI_DOUBLE, MPI_MAX, 0, MPI_COMM_CARTESIAN);
	/******************************************/

	/* Print stuff */
	// char fname[10];
	// itoa(rank, fname);
	// prtdat(task_X, task_Y, *u[0], fname);
	// // // printf("rank: %d\tcoords: %d, %d\t task_X_start: %d\ttask_X_end: %d\ttask_Y_start: %d\ttask_Y_end: %d, neighbors(u,d,l,r)= %d %d %d %d\n\n", rank, coords[0], coords[1],
	// task_X_start, task_X_end, task_Y_start, task_Y_end, neighbors[UP], neighbors[DOWN], neighbors[LEFT], neighbors[RIGHT]);
	if (rank == 0) {
		printf("Convergence: %d\n", CONVERGENCE);
#if (CONVERGENCE == 1)
		if(step != STEPS){
			printf("Converged in %d steps\n", step);
		}
#endif
		printf("u size: [%d][%d]\n", NXPROB, NYPROB);
		printf("tasks: %d\n", number_of_tasks);
		printf("Time elapsed: %f seconds\n", reduced_time);
	}
	/****************/

	/* Cleanup everything */
	free(u0Data);
	free(u1Data);
	free(u[0]);
	free(u[1]);
	free(u);
	MPI_Type_free(&MPI_row);
	MPI_Type_free(&MPI_column);
	MPI_Finalize();
	/*********************/
}
