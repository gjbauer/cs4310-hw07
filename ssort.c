#include <stdio.h>
#include <stdlib.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <unistd.h>
#include <fcntl.h>
#include <math.h>

#include "float_vec.h"
#include "barrier.h"
#include "utils.h"

int compare(const void *a, const void *b) {
	float fa = *(const float*)a;
	float fb = *(const float*)b;
	//printf("a = %f , b = %f\n", fa, fb);
	if (fa < fb) return -1;
	if (fa > fb) return 1;
	return 0;
}

void
qsort_floats(floats* xs)
{
    // calls qsort to sort the array
    // see "man 3 qsort" for details
    int i;
    for(i=0; xs->data[i]!=0; i++);
    xs->size=i;
    float dat[xs->size];
    for(int i=0; xs->data[i]!=0; i++) dat[i]=xs->data[i];
    //printf("i = %d\n", i);
    //printf("xs->size = %ld\n", xs->size);
    qsort(dat, xs->size, sizeof(float), compare);
    /*for(int i=0; dat[i]; i++)
    	printf("dat: %.4f\n", dat[i]);*/
    free(xs->data);
    xs->data=malloc(xs->size*sizeof(float));
    for(int i=0; i<xs->size; i++) xs->data[i]=dat[i];
    /*for(int i=0; xs->data[i]; i++)
    	printf("data: %.4f\n", xs->data[i]);*/
    return;
}

floats*
sample(float* data, long size, int P)
{
    // TODO: sample the input data, per the algorithm decription
    int proc_size = size / P;
    floats **samps = malloc(P * sizeof(floats));
    for(int i=0; i<P; i++)
	    samps[i] = make_floats(proc_size);
    //printf("%d\n", proc_size);

    int j;
    for (int i=0; i < P; i++) {
	    samps[i] = make_floats(proc_size);
	    for (int k=0; k < proc_size && j < size; k++, j++) {
                samps[i]->data[k] = data[j];
	    }
    }

     //floats* samp = make_floats(10);
     //for(int i=0; i<10&&*data; i++) samp->data[i] = *data++;
     return *samps;
}

void
sort_worker(int pnum, float* data, long size, int P, floats* samps, long* sizes, barrier* bb)
{
    floats* xs = make_floats(10);
    // TODO: select the floats to be sorted by this worker
    barrier_wait(bb);
    for(int i=0; i<10&&*data; i++) xs->data[i] = data[i];

    //printf("%d: start %.04f, count %ld\n", pnum, samps->data[pnum], xs->size);

    // TODO: some other stuff
    floats *pntr = (floats*)(&samps+pnum);
    //int mul = &samps[pnum];
    //printf("samps = %p\n", (void*)(char*)samps);
    //printf("pnum = %d\n", pnum);
    //printf("samps+pnum = %p\n", &samps[pnum]);
    //printf("sizeof(floats) = %ld\n", sizeof(floats));

    qsort_floats(pntr);
    for (int i=0; i<pntr->size; i++) data[i]=pntr->data[i];
    printf("%d: start %.04f, count %ld\n", pnum, data[pnum], xs->size);
    // TODO: probably more stuff

    free_floats(xs, P);
}

void
run_sort_workers(float* data, long size, int P, floats* samps, long* sizes, barrier* bb)
{
    pid_t kids[P];
    (void) kids; // suppress unused warning

    //sort_worker(0, data, size, P, samps, sizes, bb);
    /*for(int i=0; data[i]; i++)
    	printf("%.4f\n", data[i]);*/
    // TODO: spawn P processes, each running sort_worker
    for (int i=0; i<P; i++) {
	    // TODO: Create processes...
	    kids[i] = fork();
	    if (kids[i] < 0) {
                perror("fork fail\n");
	    }
	    if (kids[i]==0) {
		sort_worker(i, data, size, P, samps, sizes, bb);
	    }
    }

    for (int ii = 0; ii < P; ++ii) {
        int rv = waitpid(kids[ii], 0, 0);
        check_rv(rv);
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
/*	int arr[8] = {6, 93, 45, 124, 5, 807, 30, 404};
	int n;
    for(n=0; arr[n]; n++);
    qsort(arr, n, sizeof(int), compare);

    for (int i = 0; arr[i]; i++) {
	    printf("arr[%d] = %d\n", i, arr[i]);
    }*/

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

    //printf("count = %ld\n", count);
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

    //for (int i=P-1; i>=0; i--)
//	    free(&xs[i]);

    //free_floats(P);
    free(sizes);
    free(file);
    free(data);
    // TODO: munmap your mmaps

    return 0;
}

