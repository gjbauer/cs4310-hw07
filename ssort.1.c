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
#include <pthread.h>
#include <string.h>

#include "ssort.h"

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
    qsort(xs->data, xs->size, sizeof(float), compare);
}

floats*
sample(float* data, long size, int P)
{
    floats* xs = make_floats(size);
    for (int i=0; i<size; i++) xs->data[i] = data[i];
    return xs;
}

void*
sort_worker(int pnum, float* data, long size, int P, floats* samps, long* sizes, barrier* bb)
{
    printf("%d: start %.04f, count %ld\n", pnum, samps->data[0], size);
    qsort_floats(samps);
    for (int ii = 2; ii < 10; ++ii) {
        printf("%.04f\n", samps->data[ii]);
    }
    return 0;
}

void
run_sort_workers(float* data, long size, int P, floats* samps, long* sizes, barrier* bb)
{
    sort_worker(0, data, size, P, samps, sizes, bb);
}

void
sample_sort(float* data, long size, int P, long* sizes, barrier* bb)
{
    floats* samps = sample(data, size, P);
    run_sort_workers(data, size, P, samps, sizes, bb);
    memcpy(data, samps->data, 10*sizeof(float));
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

    long count = st.st_size/sizeof(float);
    float* data = (float*)file;

    long sizes_bytes = P * sizeof(long);
    long* sizes = mmap(NULL, sizes_bytes, PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, 0, 0);

    barrier* bb = make_barrier(P);

    sample_sort(data, count, P, sizes, bb);
    
    for (int ii = 2; ii < count; ++ii) {
        printf("%.04f\n", data[ii]);
    }

    free_barrier(bb);

    munmap(file, st.st_size);

    return 0;
}

