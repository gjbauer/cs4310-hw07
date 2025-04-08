#include <stdio.h>
#include <stdlib.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <unistd.h>
#include <fcntl.h>
#include <math.h>
//#include <pthread.h>
#include "float_vec.h"
#include "barrier.h"
#include "utils.h"

//static pthread_mutex_t lock = PTHREAD_MUTEX_INITIALIZER;

int compare(const void *a, const void *b) {
	float fa = *(const float*)a;
	float fb = *(const float*)b;
	if (fa < fb) return -1;
	if (fa > fb) return 1;
	return 0;
}

void
qsort_floats(floats* xs)
{
    // calls qsort to sort the array
    // see "man 3 qsort" for details
    float *dat=malloc(xs->size*sizeof(float));
    for(int i=0; i<xs->size; i++) dat[i]=xs->data[i];
    qsort(dat, xs->size, sizeof(float), compare);
    free(xs->data);
    xs->data=malloc(xs->size*sizeof(float));
    for(int i=0; i<xs->size-1; i++) xs->data[i]=dat[i];
    free(dat);
    return;
}

floats*
sample(float* data, long size, int P)
{
    // TODO: sample the input data, per the algorithm decription
    int proc_size = size / P;
    floats **samps = malloc(P * sizeof(floats*));

    int j=0;
    for (int i=0; i < P; i++) {
	    samps[i] = make_floats(proc_size);
	    for (int k=0; k < samps[i]->size && j < size; k++, j++) {
                samps[i]->data[k] = data[j];
	    }
    }

     return *samps;
}

void
sort_worker(int pnum, float* data, long size, int P, floats* samps, long* sizes, barrier* bb)
{

    qsort_floats(&samps[pnum]);
    //pthread_mutex_lock(&lock);
    for (int i=0; i<samps[pnum].size; i++) data[i]=samps[pnum].data[i];
    //pthread_mutex_unlock(&lock);
    printf("%d: start %.04f, count %ld\n", pnum, data[pnum], samps[pnum].size);
    
    exit(0);
}

void
run_sort_workers(float* data, long size, int P, floats* samps, long* sizes, barrier* bb)
{
    pid_t kids[P];
    (void) kids; // suppress unused warning
    int stat;
    sizes = malloc(P*sizeof(long));

    for (int i=0; i<P; i++) {
	    kids[i] = fork();
	    if (kids[i] < 0) {
                perror("fork fail\n");
	    }
	    if (kids[i]==0) {
		sort_worker(i, data, size, P, samps, sizes, bb);
	    }
	    else {
                int rv = waitpid(kids[i], &stat, 0);
		check_rv(rv);
	    }
    }
}

void
sample_sort(float* data, long size, int P, long* sizes, barrier* bb)
{
    floats* samps = sample(data, size, P);
    run_sort_workers(data, size, P, samps, sizes, bb);
    free_floats(samps, P);
}

int
main(int argc, char* argv[])
{
    alarm(120); // leave this here

    if (argc != 3) {
        printf("Usage:\n");
        printf("\t%s P data.dat\n", argv[0]);
        return 1;
    }

    const int P = atoi(argv[1]);
    const char* fname = argv[2];

    seed_rng();

    int rv;
    struct stat st;
    rv = stat(fname, &st);
    check_rv(rv);

    const int fsize = st.st_size;
    if (fsize < 8) {
        printf("File too small.\n");
        return 1;
    }

    int fd = open(fname, O_RDWR);
    check_rv(fd);

    void* file = malloc(1024); // TODO: load the file with mmap.
    (void) file; // suppress unused warning.

    long count = 0;
    floats xs;
    read(fd, &count, sizeof(long));
    float* data = malloc(16*1024);
    xs.data = data;

    int x, k=0;
    float z;
    while ((x = read(fd, &z, sizeof(float)))) {
	    xs.data[k++]=z;
    }
    xs.size=k;

    long sizes_bytes = P * sizeof(long);
    long* sizes = malloc(sizes_bytes); // TODO: This should be shared

    barrier* bb = make_barrier(P);

    sample_sort(xs.data, count, P, sizes, bb);
    floats_print(&xs, P);
    free_barrier(bb);

    //free_floats(P);
    free(sizes);
    free(file);
    free(data);
    // TODO: munmap your mmaps

    return 0;
}

