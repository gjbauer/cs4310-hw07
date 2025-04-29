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
    // TODO: call qsort to sort the array
    // see "man 3 qsort" for details
}

floats*
sample(float* data, long size, int P)
{
    // TODO: sample the input data, per the algorithm decription
    int samp[size];
    /*
     * Randomly select 3*(P-1) items from the array.
     * Sort those items.
     * Take the median of each group of three in the sorted array, producing an array (samples) of (P-1) items.
     * Add 0 at the start and +inf at the end (or the min and max values of the type being sorted) of the samples array so it has (P+1) items numbered (0 to P).
     */
     // arc4random_uniform(size + 1);
    return xs;
}

void*
sort_worker(void* arg)
{
    struct sort_args *sa = (struct sort_args*)arg;
    floats* xs = make_floats(3*sa->pnum);
    
    // Each process uses quicksort to sort the local array.
    
    // TODO: select the floats to be sorted by this worker

    printf("%d: start %.04f, count %ld\n", sa->pnum, sa->samps->data[pos], xs->size);

    // TODO: some other stuff

    qsort_floats(xs);

    // TODO: probably more stuff
    
    /* Each process calculates where to put its result array as follows:
     *     start = sum(sizes[0 to p - 1]) # that’s sum(zero items) = 0 for p = 0
     *     end = sum(sizes[0 to p]) # that’s sum(1 item) for p = 0
     * Warning: Data race if you don’t synchronize here.
     * Each process copies its sorted array to input[start..end]
     */

    free_floats(xs);
    return 0;
}

void
run_sort_workers(float* data, long size, int P, floats* samps, long* sizes, barrier* bb)
{
    pthread_t kids[P];
    (void) kids; // suppress unused warning

    // TODO: spawn P processes, each running sort_worker
    
    /* Spawn P processes, numbered p in (0 to P-1).
     * Each process builds a local array of items to be sorted by scanning the full input and taking items between samples[p] and samples[p+1].
     * Write the number of items (n) taken to a shared array sizes at slot p.
     */

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

    int fd = open(fname, O_RDWR);
    check_rv(fd);

    void* file = mmap(NULL, st.st_size, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    (void) file; // suppress unused warning.
    assert (file != MAP_FAILED);

    long count = st.st_size/sizeof(float);
    float* data = (float*)file;
    
    for (int ii = 2; ii < count; ++ii) {
        printf("%.04f ", data[ii]);
    }

    long sizes_bytes = P * sizeof(long);
    long* sizes = mmap(NULL, sizes_bytes, PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, 0, 0);

    barrier* bb = make_barrier(P);

    sample_sort(data, count, P, sizes, bb);

    free_barrier(bb);

    munmap(file, st.st_size);

    return 0;
}

