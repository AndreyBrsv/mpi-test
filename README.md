# mpi-test

##Настройки для macOS:

`brew install openmpi`

##MPI:

`MPI_Send(void* buf, int count, MPI_Datatype datatype, int dest, int tag, MPI_Comm comm)`
IN buf	-	адрес начала расположения пересылаемых данных;
IN count	-	число пересылаемых элементов;
IN datatype	-	тип посылаемых элементов;
IN dest	-	номер процесса-получателя в группе, связанной с коммуникатором comm;
IN tag	-	идентификатор сообщения (аналог типа сообщения функций nread и nwrite PSE nCUBE2);
IN comm	-	коммуникатор области связи.


`int MPI_Recv(void* buf, int count, MPI_Datatype datatype, int source, int tag, MPI_Comm comm, MPI_Status *solvingStatus)`
OUT	buf	-	адрес начала расположения принимаемого сообщения;
IN	count	-	максимальное число принимаемых элементов;
IN	datatype	-	тип элементов принимаемого сообщения;
IN	source	-	номер процесса-отправителя;
IN	tag	-	идентификатор сообщения;
IN	comm	-	коммуникатор области связи;
OUT	solvingStatus	-	атрибуты принятого сообщения.


`int MPI_Barrier(MPI_Comm comm)`
Функция барьерной синхронизации MPI_BARRIER блокирует вызывающий процесс, пока все процессы группы не вызовут её.
В каждом процессе управление возвращается только тогда, когда все процессы в группе вызовут процедуру.

Широковещательная рассылка данных выполняется с помощью функции MPI_Bcast. Процесс с номером root рассылает сообщение
из своего буфера передачи всем процессам области связи коммуникатора comm.

Математические операции над блоками данных, распределенных по процессорам, называют глобальными операциями редукции.
В общем случае операцией редукции называется операция, аргументом которой является вектор,
а результатом - скалярная величина, полученная применением некоторой математической операции ко всем компонентам вектора

В MPI глобальные операции редукции представлены в нескольких вариантах:
с сохранением результата в адресном пространстве одного процесса (MPI_Reduce).
с сохранением результата в адресном пространстве всех процессов (MPI_Allreduce).


Семейство функций распределения блоков данных по всем процессам группы состоит из двух подпрограмм: MPI_Scatter и MPI_Scaterv.

Функция MPI_Scatter разбивает сообщение из буфера посылки процесса root на равные части размером sendcount и посылает
i-ю часть в буфер приема процесса с номером i (в том числе и самому себе). Процесс root использует оба буфера (посылки
и приема), поэтому в вызываемой им подпрограмме все параметры являются существенными. Остальные процессы группы с
коммуникатором comm являются только получателями, поэтому для них параметры, специфицирующие буфер посылки, не существенны.

int MPI_Scatter(void* sendbuf, int sendcount, MPI_Datatype sendtype,
void* recvbuf, int recvcount, MPI_Datatype recvtype,
int root, MPI_Comm comm)

IN	sendbuf	-	адрес начала размещения блоков распределяемых данных (используется только в процессе-отправителе root);
IN	sendcount	-	число элементов, посылаемых каждому процессу;
IN	sendtype	-	тип посылаемых элементов;
OUT	recvbuf	-	адрес начала буфера приема;
IN	recvcount	-	число получаемых элементов;
IN	recvtype	-	тип получаемых элементов;
IN	root	-	номер процесса-отправителя;
IN	comm	-	коммуникатор.

Функция MPI_Scaterv является векторным вариантом функции MPI_Scatter, позволяющим посылать каждому процессу различное
количество элементов. Начало расположения элементов блока, посылаемого i-му процессу, задается в массиве смещений
displs, а число посылаемых элементов - в массиве sendcounts. Эта функция является обратной по отношению к функции MPI_Gatherv.

int MPI_Scatterv(void* sendbuf, int *sendcounts, int *displs, MPI_Datatype sendtype, void* recvbuf, int recvcount,
MPI_Datatype recvtype, int root, MPI_Comm comm)

IN	sendbuf	-	адрес начала буфера посылки (используется только в процессе-отправителе root);
IN	sendcounts	-	целочисленный массив (размер равен числу процессов в группе), содержащий число элементов,
посылаемых каждому процессу;
IN	displs	-	целочисленный массив (размер равен числу процессов в группе), i-ое значение определяет смещение
относительно начала sendbuf для данных, посылаемых процессу i;
IN	sendtype	-	тип посылаемых элементов;
OUT	recvbuf	-	адрес начала буфера приема;
IN	recvcount	-	число получаемых элементов;
IN	recvtype	-	тип получаемых элементов;
IN	root	-	номер процесса-отправителя;
IN	comm	-	коммуникатор.

Функция MPI_Gather производит сборку блоков данных, посылаемых всеми процессами группы, в один массив процесса с номером
root. Длина блоков предполагается одинаковой. Объединение происходит в порядке увеличения номеров процессов-отправителей.
То есть данные, посланные процессом i из своего буфера sendbuf, помещаются в i-ю порцию буфера recvbuf процесса root.
Длина массива, в который собираются данные, должна быть достаточной для их размещения.

int MPI_Gather(void* sendbuf, int sendcount, MPI_Datatype sendtype, void* recvbuf, int recvcount, MPI_Datatype recvtype,
int root, MPI_Comm comm)

IN	sendbuf	-	адрес начала размещения посылаемых данных;
IN	sendcount	-	число посылаемых элементов;
IN	sendtype	-	тип посылаемых элементов;
OUT	recvbuf	-	адрес начала буфера приема (используется только в процессе-получателе root);
IN	recvcount	-	число элементов, получаемых от каждого процесса (используется только в процессе-получателе root);
IN	recvtype	-	тип получаемых элементов;
IN	root	-	номер процесса-получателя;
IN	comm	-	коммуникатор.