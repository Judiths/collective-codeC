// compile with commandline: $gcc -g -Wall -o sampleSort_multiThread sampleSort_multiThread.c -lpthread
// run with commandline:     $./sampleSort_multiThread 10000 10 data1.c
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <math.h>
#include <pthread.h>

//#define N	100000
//#define K	10
int str2int(const char *str);
/* dependency function by qsort() */
int cmp(const void *a, const void *b)
{
    return(*(int *)a-*(int *)b);
}
typedef struct _thread_data_t {
    int tid;	// 每個线程的ID
    int *array;   // 动态分配每个线程中的数组
    int count;	//动态数组中元素的个数	
} thread_data_t;

/* shared data between threads */
int shared_tid;			// 共享線程ID
int *shared_array;		// 共享輸入數組
int shared_count;		// 共享动态数组的索引数

/* declare the lock and constant variable of critical section in each threads */
pthread_mutex_t lock_tid;		// 為每個线程上鎖
pthread_mutex_t lock_count;
pthread_cond_t cond_tid;		
pthread_cond_t cond_count;		

/*　以线程ID作為輸入參數, 在线程中进行qsort()排序　*/
void *thr_func0(void *arg) {
    thread_data_t *data = (thread_data_t *)arg;

    /* get mutex before modifying and printing shared_index */
    pthread_mutex_lock(&lock_tid);
    shared_tid = data->tid;				// 共享线程ID
    
    pthread_mutex_lock(&lock_count);
    shared_count = data->count; /*printf("shared_count: %d\n",shared_count);*/
    sleep(1);
    
//    printf("\n------- thread [%d] communication. ---------\n",shared_tid);
    for(int item = 0; item < shared_count; item++) {
        shared_array[item] = data->array[item];		//共享线程數組數據
    }
    /* use the function qsort() to sort the local array */
    qsort(shared_array, shared_count, sizeof(shared_array[0]), cmp);
//    for(int i = 0; i < shared_count; i++) {  printf("thread[%d] num[%d] %d\t\t",shared_tid,i,shared_array[i]);}
//    printf("==== > thread[%d] is over!\n",shared_tid);

    pthread_mutex_unlock(&lock_count);
    pthread_cond_wait(&cond_tid,&lock_tid);
    pthread_mutex_unlock(&lock_tid);

    pthread_exit(NULL);
}

int main(int argc, char **argv) {

    int N=str2int(argv[1]);
    int K = str2int(argv[2]);
    char *filename = argv[3];
    printf( "NoofElements = %d\nNoofProcs = %d\nfilename = %s\n", N, K,filename);

    int NoofItems = N/K;		// Number of Items
    int NoofSamples = K*(K-1);	// Number of Samples

    FILE *myFile;
    int input[N], output[N];
    int unit = N/(K*K);
    int my_splitter[K-1], my_samples[NoofSamples];
    pthread_t thr[K];

    int i,j,k, rc;
    int count0 = 0;
    int count = 0;
    int tmpCount = 0;
    /* create a thread_data_t argument array */
    thread_data_t thr_data[K];

    /* initialize shared data */
    shared_tid = 0;
    shared_array = (int*)calloc(N,sizeof(int));
    shared_count = 0;
 
    /* read the file and write to the array */
    myFile = fopen(filename,"r");
    for(i = 0; i < N; i++) {
        fscanf(myFile,"%d",&input[i]);
    }
    
    /* initialize pthread mutex protecting "shared_tid" */
    pthread_mutex_init(&lock_tid, NULL);
    pthread_cond_init(&cond_tid, NULL);
    pthread_mutex_init(&lock_count, NULL);		// 初始化lock_count，使每个线程能够读出正确的thr_data[i].count值

    /* create threads and make the threads to do things like the function 'thr_func()' do */
    for (j = 0; j < K; ++j) {
        thr_data[j].tid = j;
	thr_data[j].count = count0;
	thr_data[j].array = (int*)calloc(N,sizeof(int));	//动态分配数组
        for(k = j*NoofItems; k < (j+1)*NoofItems; k++) {		//将input[N]中的元素动态分配到每个线程中去
            thr_data[j].array[k%NoofItems] = input[k];
	    count0 ++;
	    thr_data[j].array[count0-1] = thr_data[j].array[k%NoofItems];
        }
	thr_data[j].count = count0; /*printf("count0: [%d] %d %d\n", j, count0, thr_data[j].count);*/
	count0 -= count0;
        if ((rc = pthread_create(&thr[j], NULL, thr_func0, &thr_data[j]))) {
            fprintf(stderr, "error: pthread_create, rc: %d\n", rc);
            return EXIT_FAILURE;
        }
        sleep(2);
        for(int item = 0; item < NoofItems; item ++) { 
	    shared_array[j*NoofItems+item] = shared_array[item];
            if((item != NoofItems-1) && (item%unit == unit-1))
		my_splitter[(item-unit+1)/unit] = shared_array[item];
            }
//	for(int splitter= 0; splitter < K-1; splitter ++) { printf("mysplitter[%d] %d\n",splitter, my_splitter[splitter]); }
	for(int sample = 0; sample < K-1; sample ++) { my_samples[j*(K-1)+sample] = my_splitter[sample]; }

    }
    /*　pick up the splitters, and do sample sorting for the K*(K-1) splitters  */
//    for(int i = 0; i < N; i++) { printf("[%d] %d\n",i,shared_array[i]);}
//    for(int sample = 0; sample < NoofSamples; sample ++) {  printf("[%d] %d\n",sample, my_samples[sample]);}
    qsort(my_samples,NoofSamples,sizeof(my_samples[0]),cmp);
    for(int sample = 0; sample < NoofSamples; sample ++) { 
	if(sample%K == K-1)
		my_splitter[(sample-K+1)/K] = my_samples[sample];
    }
    for(int splitter= 0; splitter < K-1; splitter ++) { printf("final: mysplitter[%d] %d\n",splitter, my_splitter[splitter]);}
/* ------------------------------------------ the first step: GO TO THEIR OWN BUCKET is over--------------------------------------------------*/
    int tmp[N];
    for(i = 0; i < N; i ++) { tmp[i] = input[i]; }   
    for(i = 0; i < K; i ++) {
	thr_data[i].tid = i;
	thr_data[i].count = count;	/*printf("ooo count: \t%d\n", count);*/
	thr_data[i].array = (int*)calloc(N, sizeof(int));
//	printf("mysplitter[%d] %d\n",i,my_splitter[i]);
	if(i < K-1) {
		for(j = 0; j < N; j ++) {
			if((input[j] <= my_splitter[i]) && tmp[j] != 0) {
				count ++;
				thr_data[i].array[j%N] = tmp[j];
				thr_data[i].array[count-1] = thr_data[i].array[j%N];
//				printf("thr_data[%d].array[%d] %d\t\t",i, count-1, thr_data[i].array[count-1]);
				tmp[j] = 0;
			}
			else if((input[j] > my_splitter[i]) && tmp[j] != 0 ) {
				input[j] = tmp[j];
			}
		}
		thr_data[i].count = count; /*printf("count: [%d] %d %d\n", i, count, thr_data[i].count);*/
		count -= count;		// make ihe counter beginning with zer0 for every thread
	}
	if(i == K-1) {
		for(j = 0; j < N; j ++) {
			if(input[j] > my_splitter[i-1]) {
				count++;
				thr_data[i].array[j%N] = input[j];
				thr_data[i].array[count-1] = thr_data[i].array[j%N];
//				printf("thr_data[%d].array[%d] %d\t\t",i, count-1, thr_data[i].array[count-1]);
			}
		}
		thr_data[i].count = count; /*printf("xcount: [%d] %d %d\n", i, count, thr_data[i].count);*/
	}
	if ((rc = pthread_create(&thr[i], NULL, thr_func0, &thr_data[i]))) {
       		fprintf(stderr, "error: pthread_create, rc: %d\n", rc);
                return EXIT_FAILURE;
       	}
	sleep(3);
 /*--------------------------------------------- the second step: SLAVER'S WORK IS DONE is over. -----------------------------------------------*/
	for(int item = 0 ; item < thr_data[i].count; item ++) {
		tmpCount ++;
     		output[tmpCount-1] = shared_array[item];
//	        printf("thr_count: %d\txthread[%d] [%d] %d\n",thr_data[i].count, i, tmpCount-1, output[tmpCount-1]);
	}
	printf("thr[%d].tmpCount: %d\tthr_data[%d].count: %d\n", i, tmpCount, i, thr_data[i].count);
    }
//    for(int i = 0; i < N; i++) { printf("sorted[%d] %d\t\t",output[i]);}
//    for(int i = 0; i < N; i++) { printf("sorted[%d] %d\n",i,output[i]);}
    printf("Output[1] %d\n", output[1]);			// the second smallest one
    printf("Output[%d] %d\n",(N-1)/2, output[(N-1)/2]);		// the middle n/2-th element
    printf("Output[%d] %d\n",N-2, output[N-2]);			// the second largest one

 /*----------------------------------------- the final step: PRINT THE OUTPUT ARRAY WITH ASCEND is over-------------------------------------------*/

    /* block until all threads complete */
    for (j = 0; j < K; ++j) {
        pthread_join(thr[j], NULL);
    }
    return EXIT_SUCCESS;
    //exit(0);
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
