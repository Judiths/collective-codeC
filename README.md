# collective-codeC
Sample Sort implemented by multiple threads and MPI
1. sampleSort_multiThread.c
 Compile with 
          $gcc -g -Wall -o s_multiThr sampleSort_multiThread.c -lpthread
 Run with   
          $./s_multiThr 10000 10 data1.dat

2. samleSort_MPI.c
 Compile with 
          $mpicc -g -Wall -o sampleSort_mpi sampleSort_MPI.c
 Run with
          $mpiexec.hydra -f /home/zer0/hostfile -n [number of processes] ./sampleSort_mpi [number of Items] [filename]
    e.g   $mpiexec.hydra -f /home/zer0/hostfile -n 10 ./sampleSort 10000 data1.dat
          

