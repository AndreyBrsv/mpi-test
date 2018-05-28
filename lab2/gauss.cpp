#include <mpi.h>
#include <cstdio>
#include <cstdlib>
#include <cmath>
#include <ctime>

bool usePrint = true;

double *matrix, *vector, *result; // Основные данные. Заполняются через rand()

double *matrixPart, *vectorPart, *resultPart, mess = 0.0; // Нарезка для данного процесса

int matrixSize = 3, status, partSizeOnProcess; // размер матрицы, статус решения задачи, строки для текущего процесса
int *mainRowIndexArray, *mainRowIteration; // ведущие строки для каждой итерации, номера итераций ведущих строк
int size, rank, *mass1, *range; // размер, ранг, рассылка, количество на каждый процесс

//----------------------------------------------------------------------------------------------------------------------
double getRandomDouble() {
    return rand() / 10000000.0;
}

void initMatrixAndVector() {
    if (rank == 0) //Заполняем
    {
        srand(static_cast<unsigned int>(time(0))); //псевдослучайные

        matrix = new double[matrixSize * matrixSize];
        vector = new double[matrixSize];
        result = new double[matrixSize];

        for (int i = 0; i < matrixSize * matrixSize; ++i) {
            matrix[i] = getRandomDouble();
            if (usePrint) {
                printf("\nA[%i]=%f", i, matrix[i]);
            }
        };
        for (int i = 0; i < matrixSize; i++) {
            vector[i] = getRandomDouble();
            if (usePrint) {
                printf("\nb[%i]=%f", i, vector[i]);
            }
            result[i] = 0;
        };
    }
}

void initParts() {
    int nonDistributeRowCount = matrixSize - matrixSize / (size - rank + 1);
    partSizeOnProcess = nonDistributeRowCount / (size - rank);// Определение размера части данных,на конкретном процессе
    matrixPart = new double[partSizeOnProcess * matrixSize];
    vectorPart = new double[partSizeOnProcess]; // элементы столбца свободных членов
    resultPart = new double[partSizeOnProcess];
    mainRowIndexArray = new int[matrixSize]; //массив индексов ведущих строк системы на каждой итерации
    mainRowIteration = new int[partSizeOnProcess]; /* итерация, на которой соответствующая строка системы, расположенная на процессе, выбрана  ведущей */
    mass1 = new int[size];
    range = new int[size];
    for (int i = 0; i < partSizeOnProcess; i++) {
        mainRowIteration[i] = -1;
    }
}

void initData() {
    initMatrixAndVector();
    MPI_Bcast(&matrixSize, 1, MPI_INT, 0, MPI_COMM_WORLD);
    initParts();
}

//------------------------------------------------------------------------------
void disributeDataBetweenProcesses() { // Распределение исходных данных между процессами
    int *matrElem;             //Индекс первого элемента матрицы, передаваемого процессу
    int *matrRang;            //Число элементов матрицы, передаваемых процессу
    int sizestr;
    int balance;
    int prevSize;
    int prevIndex;
    int portion;
    matrElem = new int[size];
    matrRang = new int[size];

    balance = matrixSize;

    for (int i = 0; i < size; i++)  //Определяем, сколько элементов матрицы будет передано каждому процессу
    {
        prevSize = (i == 0) ? 0 : matrRang[i - 1];
        prevIndex = (i == 0) ? 0 : matrElem[i - 1];
        portion = (i == 0) ? 0 : sizestr;             //число строк, отданных предыдущему процессу
        balance -= portion;
        sizestr = balance / (size - i);
        matrRang[i] = sizestr * matrixSize;
        matrElem[i] = prevIndex + prevSize;
    };

    //Рассылка матрицы
    MPI_Scatterv(matrix, matrRang, matrElem, MPI_DOUBLE, matrixPart, matrRang[rank], MPI_DOUBLE, 0,
                 MPI_COMM_WORLD);


    balance = matrixSize;

    for (int i = 0; i < size; i++) {
        int prevSize = (i == 0) ? 0 : range[i - 1];
        int prevIndex = (i == 0) ? 0 : mass1[i - 1];
        int portion = (i == 0) ? 0 : range[i - 1];
        balance -= portion;
        range[i] = balance / (size - i);
        mass1[i] = prevIndex + prevSize;
    };

    //Рассылка вектора
    MPI_Scatterv(vector, range, mass1, MPI_DOUBLE, vectorPart, range[rank], MPI_DOUBLE, 0, MPI_COMM_WORLD);
    status = 1;
    MPI_Bcast(&status, 1, MPI_INT, 0, MPI_COMM_WORLD);
    MPI_Bcast(&mess, 1, MPI_DOUBLE, 0, MPI_COMM_WORLD);
    delete[] matrRang;
    delete[] matrElem;
}

//------------------------------------------------------------------------------

void Raw(int numIter, const double *glStr) {
    double koef;
    //для каждой строки в процессе
    for (int i = 0; i < partSizeOnProcess; i++) {
        if (mainRowIteration[i] == -1) {
            koef = matrixPart[i * matrixSize + numIter] / glStr[numIter];
            for (int j = numIter; j < matrixSize; j++) {
                matrixPart[i * matrixSize + j] -= glStr[j] * koef;
            };
            vectorPart[i] -= glStr[matrixSize] * koef;

        };
    };
}

//------------------------------------------------------------------------------

void gauss() {
    int VedIndex;   // индекс ведущей строки на конкретном процессе
    struct {
        double maxValue;
        int currentRank;
    } localMax = {}, glbMax = {}; //максимальный элемент+номер процесса, у которого он
    double *glbWMatr = new double[matrixSize + 1]; //т.е. строка матрицы+значение вектора
    for (int i = 0; i < matrixSize; i++) {
        // Вычисление ведущей строки
        double maxValue = 0;
        int index = -1;
        for (int j = 0; j < partSizeOnProcess; j++) {
            index = j;
            if ((mainRowIteration[j] == -1) && (maxValue < fabs(matrixPart[i + matrixSize * j]))) {
                maxValue = fabs(matrixPart[i + matrixSize * j]);
                VedIndex = j;
            }
        }

        localMax.maxValue = maxValue;
        localMax.currentRank = rank;

        //каждый процесс рассылает свой локально максимальный элемент по всем столцам, все процесы принимают уже глобально максимальный элемент
        MPI_Allreduce(&localMax, &glbMax, 1, MPI_DOUBLE_INT, MPI_MAXLOC, MPI_COMM_WORLD);

        //Вычисление ведущей строки всей системы
        if (rank == glbMax.currentRank) {
            if (glbMax.maxValue == 0) {
                status = 2;
                MPI_Barrier(MPI_COMM_WORLD);
                MPI_Send(&status, 1, MPI_INT, 0, MPI_ANY_TAG, MPI_COMM_WORLD);
                mainRowIteration[index] = i;
                mainRowIndexArray[i] = mass1[rank] + VedIndex;
                continue;

            } else {
                // Номер итерации, на которой строка с локальным номером является ведущей для всей системы
                mainRowIteration[VedIndex] = i;
                //Вычисленный номер ведущей строки системы
                mainRowIndexArray[i] = mass1[rank] + VedIndex;
            };
        };
        MPI_Bcast(&mainRowIndexArray[i], 1, MPI_INT, glbMax.currentRank, MPI_COMM_WORLD);
        if (rank == glbMax.currentRank) {
            for (int j = 0; j < matrixSize; j++) {
                glbWMatr[j] = matrixPart[VedIndex * matrixSize + j];
            }
            glbWMatr[matrixSize] = vectorPart[VedIndex];
        };
        //Рассылка ведущей строки всем процессам
        MPI_Bcast(glbWMatr, matrixSize + 1, MPI_DOUBLE, glbMax.currentRank, MPI_COMM_WORLD);
        //Исключение неизвестных в столбце с номером i
        Raw(i, glbWMatr);
    };
}

/*
stringIndex - номер строки, которая была ведущей на определеной итерации
iterationcurrentRank - процесс, на котором эта строка
IterationItervedindex - локальный номер этой строки (в рамках одного процесса)
*/
void Frp(int stringIndex, int &iterationcurrentRank, int &IterationItervedindex) {
    //Определяем ранг процесса, содержащего данную строку
    for (int i = 0; i < size - 1; i++) {
        if ((mass1[i] <= stringIndex) && (stringIndex < mass1[i + 1])) {
            iterationcurrentRank = i;
        }
    }
    if (stringIndex >= mass1[size - 1]) {
        iterationcurrentRank = size - 1;
    }
    IterationItervedindex = stringIndex - mass1[iterationcurrentRank];

}


void gaussBackStroke() {
    int itCurrentRank;  // Ранг процесса, хранящего текущую ведущую строку
    int indexMain;    // локальный на своем процессе номер текущей ведущ
    double iterRes;   // значение Xi, найденное на итерации
    double val;
    // Основной цикл
    for (int i = matrixSize - 1; i >= 0; i--) {
        Frp(mainRowIndexArray[i], itCurrentRank, indexMain);
        // Определили ранг процесса, содержащего текущую ведущую строку, и номер этой строки на процессе
        // Вычисляем значение неизвестной
        if (rank == itCurrentRank) {
            if (matrixPart[indexMain * matrixSize + i] == 0) {
                if (vectorPart[indexMain] == 0) {
                    iterRes = mess;
                } else {
                    status = 0;
                    MPI_Barrier(MPI_COMM_WORLD);
                    MPI_Send(&status, 1, MPI_INT, 0, MPI_ANY_TAG, MPI_COMM_WORLD);
                    break;
                }
            } else {
                iterRes = vectorPart[indexMain] / matrixPart[indexMain * matrixSize + i];
            }
            //нашли значение переменной
            resultPart[indexMain] = iterRes;
        }
        MPI_Bcast(&iterRes, 1, MPI_DOUBLE, itCurrentRank, MPI_COMM_WORLD);
        //подстановка найденной переменной
        for (int j = 0; j < partSizeOnProcess; j++) {
            if (mainRowIteration[j] < i) {
                val = matrixPart[matrixSize * j + i] * iterRes;
                vectorPart[j] -= val;
            }
        }
    }
}

//------------------------------------------------------------------------------

void prepareMPI(int argc, char *argv[]) {
    if (rank == 0) {
        printf("Start \n");
    }
    setvbuf(stdout, 0, _IONBF, 0); // режим доступа и размер буфера
    MPI_Init(&argc, &argv);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &size);
    MPI_Barrier(MPI_COMM_WORLD);
}


void printExecutionStatus(double time) {
    if (rank == 0) {
        if (status == 1) {
            printf("\nDone \n"); // решение найдено
        } else if (status == 0) {
            printf("\nNo roots.\n"); // решений нет
        } else if (status == 2) {
            printf("\nCan't \n"); // решений бесконечно много
        };
        printf("\nTime: %f\n", time);
    }
}

void calculateWithGaussMethod() {
    MPI_Barrier(MPI_COMM_WORLD);
    double time = MPI_Wtime();
    gauss();
    gaussBackStroke();
    //сбор данных, передача от всех одному (нулевому процессу)
    MPI_Gatherv(resultPart, range[rank], MPI_DOUBLE, result, range, mass1, MPI_DOUBLE, 0, MPI_COMM_WORLD);
    MPI_Barrier(MPI_COMM_WORLD);
    printExecutionStatus(MPI_Wtime() - time);
}

void printResult() {
    for (int i = 0; i < matrixSize; i++) {
        if (usePrint) {
            printf("\nresult[%i]=%f", i, result[i]);
        }
    }
}

void finalize() {
    if (rank == 0) {
        printf("\n End");
    }
    MPI_Finalize();
    delete[] matrix, vector, result;
}

int main(int argc, char *argv[]) {
    prepareMPI(argc, argv);
    initData();
    disributeDataBetweenProcesses();
    calculateWithGaussMethod();
    printResult();
    finalize();
}