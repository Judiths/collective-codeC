// compile with $mpicc -g -Wall -o sampleSort_MPI sampleSort_MPI.c
// run with     $mpiexec.hydra -f /home/zer0/hostfile -n [number of processes] ./s_mpi [number of Items] [filename]
// e.g:         $mpiexec.hydra -f /home/zer0/hostfile -n 10 ./s_mpi 100000 data2.dat
#include <stdio.h>
#include <stdlib.h>
#include <mpi.h>

//const int N = 100000;
int cmp(const void *a, const void *b) { return (*(int *)a - *(int *)b);}
typedef struct _proc_data_t {
    int *array;   	        // 动态分配每个进程中的数组
    int count;		// 动态数组中元素的个数	
}proc_data_t;
int str2int(const char *str);
int main(int argc, char *argv[]) {
    int N=str2int(argv[1]);
    char *filename = argv[2];
    printf( "NoofElements = %d\nfilename = %s\n", N, filename);

    FILE * myFile;
    int input[N], tmp[N];
    /* read the file's context into input[N]*/
    myFile = fopen(filename, "r");
    for(int i = 0; i < N; i ++) fscanf(myFile, "%d", &input[i]);

    int comm_sz;
    int my_rank;
    int namelen;
    char processor_name[MPI_MAX_PROCESSOR_NAME];
    double starttime = 0.0, endtime;

    MPI_Init(&argc, &argv);
    MPI_Comm_size(MPI_COMM_WORLD, &comm_sz);
    MPI_Comm_rank(MPI_COMM_WORLD, &my_rank);
    MPI_Get_processor_name(processor_name, &namelen);
    fprintf(stdout, "Processor %d f %d id on %s\n", my_rank, comm_sz, processor_name);
    fflush(stdout);
	
    proc_data_t p_data[comm_sz];
    int N0 = N/comm_sz;
    int NoofSamples = comm_sz*(comm_sz-1);
    int unit = N/(comm_sz*comm_sz);
    int my_samples[NoofSamples];
    int final_splitter[comm_sz-1];
    int count0 = 0;
    
    int array[N0];
    int my_splitter[comm_sz-1];
    int recvnum;
   
    int *global_data = NULL;
    int *local_array = NULL;
    int *output = NULL;
    
    int *tmpCount = NULL;
    int *displs = NULL;

    int i, j, k;
    global_data = (int*)calloc(N, sizeof(int));
    tmpCount = (int*)calloc(comm_sz, sizeof(int));
    local_array = (int*)calloc(2*N0, sizeof(int));
    displs = (int*)calloc(comm_sz, sizeof(int));
    displs[0] = 0;
    if(my_rank == 0) {
	output = (int*)calloc(N, sizeof(int));
        for(i = 0; i < N; i ++) { global_data[i] = input[i];}
        printf("\n");
	starttime = MPI_Wtime();		// start time with 
    }
    MPI_Scatter(global_data, N0, MPI_INT, array, N0, MPI_INT, 0, MPI_COMM_WORLD);
/* -----------------------divide the global_data in #comm_sz buckets is over.---------------------------*/
    qsort(array, N0, sizeof(array[0]), cmp);
    for(j = 0; j < N0; j ++) {
	if((j != N0-1) && (j%unit == unit-1))
	    my_splitter[(j-unit+1)/unit] = array[j];
    }    
/* -----------------------------do local sort and pick up my_splitters is over. --------------------------------------*/
    MPI_Gather(my_splitter, comm_sz-1, MPI_INT, my_samples, comm_sz-1, MPI_INT, 0, MPI_COMM_WORLD);
    qsort(my_samples, NoofSamples, sizeof(my_samples[0]), cmp);
    for(j = 0; j < N0; j ++) {
	if(j%comm_sz == comm_sz-1)
	    final_splitter[(j-comm_sz+1)/comm_sz] = my_samples[j];
    }
/* ----------------------------gathering all local splitters into my_samples, do sort and pick up final splitters is over.-------------------------------- */        
    MPI_Gather(array, N0, MPI_INT, global_data, N0, MPI_INT, 0, MPI_COMM_WORLD);
    final_splitter[comm_sz-1] = 0;
    if(my_rank == 0) {
	for(i = 0; i < N; i ++) tmp[i] = input[i];
        for(i = 0; i < comm_sz; i ++){	
	    p_data[i].count = count0;
            p_data[i].array = (int*)calloc(2*N0, sizeof(int));
/* -------------------------------go their home ----------------------------------*/
	    if(i < comm_sz-1) {
	        for(j = 0; j < N; j ++) {
                    if((input[j] <= final_splitter[i]) && tmp[j] != 0) {
		        count0 ++;
		        p_data[i].array[count0-1] = tmp[j];
		        tmp[j] = 0;
		    }	
		    else if((input[j] > final_splitter[i]) && tmp[j] != 0) {
		        input[j] = tmp[j];
		    }
	        }
		tmpCount[i] = count0;
	        count0 -= count0;
	    }
	    if(i == comm_sz-1) {
	        for(j = 0; j < N; j ++) {
		    if(input[j] > final_splitter[i-1]) {
		        count0 ++;
		        p_data[i].array[count0-1] = input[j];
		    }
	        }
		tmpCount[i] = count0;
	    }	     
       }
/* -----------------------------------------------------------------point to point: send data to their home------------------------------------------------------------------*/
        for(i = 1; i < comm_sz; i ++) { 
	    MPI_Send(&tmpCount[i], 1, MPI_INT, i, 0, MPI_COMM_WORLD); /*printf("xxcount: [%d] %d\n", i, tmpCount[i]);*/
	    MPI_Send(p_data[i].array, tmpCount[i], MPI_INT, i, 0, MPI_COMM_WORLD);	
	    displs[i] = displs[i-1]+tmpCount[i-1];    /*printf("dislps[%d] %d\n",i,displs[i]);*/
       }
	recvnum = tmpCount[my_rank];local_array = p_data[my_rank].array;
	qsort(p_data[my_rank].array, tmpCount[my_rank], sizeof(local_array[0]), cmp);
    }
     if(my_rank != 0) {	
	MPI_Recv(&recvnum, 1, MPI_INT, 0, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
	MPI_Recv(local_array, recvnum, MPI_INT, 0, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
	qsort(local_array, recvnum, sizeof(local_array[0]), cmp);
    }
    MPI_Gatherv(local_array, recvnum, MPI_INT, output, tmpCount, displs, MPI_INT, 0, MPI_COMM_WORLD);

    if(my_rank == 0) {
	endtime = MPI_Wtime();
 	for(k = 0; k < comm_sz; k ++) { printf("Process[%d].count  %d\n", k, tmpCount[k]);}
	printf("Output[%d] = %d\n", 2, output[1]);
	printf("Output[%d] = %d\n", (N-1)/2, output[(N-1)/2]);
	printf("Output[%d] = %d\n", N-2, output[N-2]);
	free(global_data);
        free(local_array);
	free(tmpCount);
	free(displs);
	free(output);
	printf("wall clock time = %f\n", endtime-starttime);
        fflush(stdout);
    }
    MPI_Finalize();
    return 0;
}
int str2int(const char *str){
    int temp = 0;
    const char *ptr = str; 
    if (*str == '-' || *str == '+'){                    
        str++;
    }
    while(*str != 0){
        if ((*str < '0') || (*str > '9')){                     
            break;
        }
        temp = temp * 10 + (*str - '0'); 
        str++;      
    }
    if (*ptr == '-'){
        temp = -temp;
    }
    return temp;
}
