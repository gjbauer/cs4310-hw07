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
	if (fa < fb) return -1;
	if (fa > fb) return 1;
	return 0;
}

void
qsort_floats(floats* xs)
{
    // TODO: call qsort to sort the array
    // see "man 3 qsort" for details
    qsort(xs->data, xs->size, sizeof(float), compare);
}

floats*
sample(float* data, long size, int P)
{
    // TODO: sample the input data, per the algorithm decription
    int samp[size];
    for (int i=0; i<size; i++) samp[i] = rand() % (size + 1);
    floats* xs = make_floats(size); // TODO: Implement random samples...
    for (int i=0; i<size; i++) xs->data[i] = data[samp[i]];
    return xs;
}

void
sort_worker(int pnum, float* data, long size, int P, floats* samps, long* sizes, barrier* bb)
{
    floats* xs = make_floats(3*pnum);
    int pos = 0;
    
    for(int i=pnum; i>0; i++) pos+=3*i;
    // TODO: select the floats to be sorted by this worker

    printf("%d: start %.04f, count %ld\n", pnum, samps->data[pnum], xs->size);

    // TODO: some other stuff

    qsort_floats(xs);

    // TODO: probably more stuff

    free_floats(xs);
}

void
run_sort_workers(float* data, long size, int P, floats* samps, long* sizes, barrier* bb)
{
    pid_t kids[P];
    (void) kids; // suppress unused warning

    // TODO: spawn P processes, each running sort_worker
    for (int ii = 0; ii < P; ++ii) {
    	sizes[ii] = 3*ii;
        kids[ii] = fork();
        sort_worker(ii, data, size, P, samps, sizes, bb);
    }

    /*for (int ii = 0; ii < P; ++ii) {
        //int rv = waitpid(kids[ii], 0, 0);
        //check_rv(rv);
    }*/
}

void
sample_sort(float* data, long size, int P, long* sizes, barrier* bb)
{
    floats* samps = sample(data, size, P);
    run_sort_workers(data, size, P, samps, sizes, bb);
    free_floats(samps);
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

    int  fd;
    long count = 0;
    FILE* data = fopen(fname, "r");
    assert(data != NULL);
    fd = fread(&count, sizeof(long), 1, data);
    assert(fd == 1);

    void* file = malloc(1024); // TODO: load the file with mmap.
    (void) file; // suppress unused warning.

    // TODO: These should probably be from the input file.
    
    float* data = (float*)file;

    //printf("...", count);
    //printf("...", data[0]);

    long sizes_bytes = P * sizeof(long);
    long* sizes = malloc(sizes_bytes); // TODO: This should be shared

    barrier* bb = make_barrier(P);

    sample_sort(data, count, P, sizes, bb);

    free_barrier(bb);

    // TODO: munmap your mmaps

    return 0;
}

