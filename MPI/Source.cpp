#include <stdio.h>
#include <iostream>
#include <stdlib.h>
#include <time.h>
#include <mpi.h>

#define N1 1000 //Количество строк матрицы A
#define N2 1000 //Количество столбцов матрицы A и количество строк матрицы B
#define N3 1000 //Количество столбцов матрицы B
#define MASTER_TO_WORKER_TAG 1
#define WORKER_TO_MASTER_TAG 4

void fillMatrix(); //Заполнить матрицы, которые будут перемножаться
void printMatrix(); //Вывести матрицы

int rank; //Ранг процессов
int size; //Количество процессов
int i, j, k; //Вспомогательные переменные
double matrixA[N1][N2]; //Объявление матрицы A
double matrixB[N2][N3]; //Объявление матрицы B
double matrixRes[N1][N3]; //Объявление матрицы перемножения 
double start_time; //Начальное время
double end_time; //Время конца
int low_bound; //Нижняя граница строк матрицы A, передаваемых работнику
int upper_bound; //Верхняя граница строк матрицы A, передаваемых работнику
int portion; // Часть строк матрицы A, передаваемых работнику
MPI_Status status;
MPI_Request request;

int main(int argc, char* argv[])
{
	//Инициализация MPI
	MPI_Init(&argc, &argv);
	MPI_Comm_rank(MPI_COMM_WORLD, &rank);
	MPI_Comm_size(MPI_COMM_WORLD, &size);

	// Мастер инициализирует работу. Передает матрицу A по работникам.
	if (rank == 0) {
		fillMatrix();
		start_time = MPI_Wtime();
		for (i = 1; i < size; i++) {
			portion = (N1 / (size - 1));
			low_bound = (i - 1) * portion;
			if (((i + 1) == size) && ((N1 % (size - 1)) != 0)) {
				upper_bound = N1;
			}
			else {
				upper_bound = low_bound + portion;
			}
			MPI_Isend(&low_bound, 1, MPI_INT, i, MASTER_TO_WORKER_TAG, MPI_COMM_WORLD, &request);
			MPI_Isend(&upper_bound, 1, MPI_INT, i, MASTER_TO_WORKER_TAG + 1, MPI_COMM_WORLD, &request);
			MPI_Isend(&matrixA[low_bound][0], (upper_bound - low_bound) * N2, MPI_DOUBLE, i, MASTER_TO_WORKER_TAG + 2, MPI_COMM_WORLD, &request);
		}
	}
	//Передать матрицу B работникам
	MPI_Bcast(&matrixB, N2 * N3, MPI_DOUBLE, 0, MPI_COMM_WORLD);

	// Работники вычисляют
	if (rank > 0) {
		MPI_Recv(&low_bound, 1, MPI_INT, 0, MASTER_TO_WORKER_TAG, MPI_COMM_WORLD, &status);
		MPI_Recv(&upper_bound, 1, MPI_INT, 0, MASTER_TO_WORKER_TAG + 1, MPI_COMM_WORLD, &status);
		MPI_Recv(&matrixA[low_bound][0], (upper_bound - low_bound) * N2, MPI_DOUBLE, 0, MASTER_TO_WORKER_TAG + 2, MPI_COMM_WORLD, &status);
		for (i = low_bound; i < upper_bound; i++) {
			for (j = 0; j < N3; j++) {
				for (k = 0; k < N2; k++) {
					matrixRes[i][j] += (matrixA[i][k] * matrixB[k][j]);
				}
			}
		}
		MPI_Isend(&low_bound, 1, MPI_INT, 0, WORKER_TO_MASTER_TAG, MPI_COMM_WORLD, &request);
		MPI_Isend(&upper_bound, 1, MPI_INT, 0, WORKER_TO_MASTER_TAG + 1, MPI_COMM_WORLD, &request);
		MPI_Isend(&matrixRes[low_bound][0], (upper_bound - low_bound) * N3, MPI_DOUBLE, 0, WORKER_TO_MASTER_TAG + 2, MPI_COMM_WORLD, &request);
	}

	// Мастер собирает выполненное вместе
	if (rank == 0) {
		for (i = 1; i < size; i++) {
			MPI_Recv(&low_bound, 1, MPI_INT, i, WORKER_TO_MASTER_TAG, MPI_COMM_WORLD, &status);
			MPI_Recv(&upper_bound, 1, MPI_INT, i, WORKER_TO_MASTER_TAG + 1, MPI_COMM_WORLD, &status);
			MPI_Recv(&matrixRes[low_bound][0], (upper_bound - low_bound) * N3, MPI_DOUBLE, i, WORKER_TO_MASTER_TAG + 2, MPI_COMM_WORLD, &status);
		}
		end_time = MPI_Wtime();
		printf("\nЗатраченное время = %f\n\n", end_time - start_time);
		//printMatrix();
	}
	MPI_Finalize();
	return 0;
}

void fillMatrix()
{
	srand(time(NULL));
	for (i = 0; i < N1; i++) {
		for (j = 0; j < N2; j++) {
			matrixA[i][j] = rand() / 10.0;
		}
	}
	for (i = 0; i < N2; i++) {
		for (j = 0; j < N3; j++) {
			matrixB[i][j] = rand() / 10.0;
		}
	}
}

void printMatrix()
{
	for (i = 0; i < N1; i++) {
		printf("\n");
		for (j = 0; j < N2; j++)
			printf("%8.2f  ", matrixA[i][j]);
	}
	printf("\n\n\n");
	for (i = 0; i < N2; i++) {
		printf("\n");
		for (j = 0; j < N3; j++)
			printf("%8.2f  ", matrixB[i][j]);
	}
	printf("\n\n\n");
	for (i = 0; i < N1; i++) {
		printf("\n");
		for (j = 0; j < N3; j++)
			printf("%8.2f  ", matrixRes[i][j]);
	}
	printf("\n\n");
}