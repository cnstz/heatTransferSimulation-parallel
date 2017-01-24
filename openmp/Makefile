EXECS=mpi_heat2d_openmp
MPICC?=mpicc 

all: ${EXECS}

mpi_heat2d_openmp: mpi_heat2d_openmp.o mpi_heat_openmp_functions.o
	${MPICC} -openmp -o mpi_heat2d_openmp mpi_heat2d_openmp.o mpi_heat_openmp_functions.o -lm

mpi_heat2d_openmp.o: mpi_heat2d_openmp.c
	${MPICC} -openmp -o mpi_heat2d_openmp.o -c mpi_heat2d_openmp.c -lm

mpi_heat_openmp_functions.o: mpi_heat_openmp_functions.c
	${MPICC} -openmp -o mpi_heat_openmp_functions.o -c mpi_heat_openmp_functions.c -lm

clean:
	rm ${EXECS} *.o