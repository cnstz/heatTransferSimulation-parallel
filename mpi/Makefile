EXECS=mpi_heat2d
MPICC?=mpicc
GCC?=gcc

all: ${EXECS}

mpi_heat2d: mpi_heat2d.o mpi_heat_functions.o
	${MPICC} -o mpi_heat2d mpi_heat2d.o mpi_heat_functions.o -lm

mpi_heat2d.o: mpi_heat2d.c
	${MPICC} -c mpi_heat2d.c -lm

mpi_heat_functions.o: mpi_heat_functions.c
	${GCC} -c mpi_heat_functions.c -lm

clean:
	rm ${EXECS} *.o