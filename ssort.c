#include <stdio.h>
#include <stdlib.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <unistd.h>
#include <fcntl.h>
#include <math.h>
#include <assert.h>

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
    floats_print(samps);
    run_sort_workers(data, size, P, samps, sizes, bb);
    floats_print(samps);
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

    int fd = open(fname, O_RDWR);
    check_rv(fd);

    void* file = mmap(NULL, st.st_size, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    (void) file; // suppress unused warning.
    assert (file != MAP_FAILED);

    long count = st.st_size/8;
    float* data = (float*)file;
    
    /*for (int ii = 0; ii < count; ++ii) {
        printf("%.04f ", data[ii]);
    }*/

    //printf("...", count);
    //printf("...", data[0]);

    long sizes_bytes = P * sizeof(long);
    long* sizes = mmap(NULL, sizes_bytes, PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, 0, 0);

    barrier* bb = make_barrier(P);

    sample_sort(data, count, P, sizes, bb);

    free_barrier(bb);

    munmap(file, st.st_size);

    return 0;
}

