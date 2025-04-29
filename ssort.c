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
#include <float.h>

#include "ssort.h"

static pthread_mutex_t lock = PTHREAD_MUTEX_INITIALIZER;

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
    float samp[(3*(P-1))];
    data = data + 2;	// Move our pointer, because it appears to be off...
    /*
     * Randomly select 3*(P-1) items from the array.
     * Sort those items.
     * Take the median of each group of three in the sorted array, producing an array (samples) of (P-1) items.
     * Add 0 at the start and +inf at the end (or the min and max values of the type being sorted) of the samples array so it has (P+1) items numbered (0 to P).
     */
    int j;
    for(int i=0; i<(3*(P-1)); i++) {
       j = arc4random_uniform(size-2);
       printf("data[%d] = %.04f\n", j, data[j]);
       samp[i]=data[j];
    }
    qsort(samp, (3*(P-1)), sizeof(float), compare);
    /*for(int i=0; i<(3*(P-1)); i++) {
        printf("%.04f\n", samp[i]);
    }*/
    //printf("find medians\n");
    floats* xs = make_floats(P+1);
    xs->data[0]=0;
    for(int i=0; i<(3*(P-1)); i+=3) {
    	xs->data[(i/3)+1]=((samp[i]+samp[i+1]+samp[i+2])/3);
        printf("%.04f\n", ((samp[i]+samp[i+1]+samp[i+2])/3));
    }
    xs->data[P]=FLT_MAX;
    return xs;
}

void*
sort_worker(void* arg)
{
    struct sort_args *sa = (struct sort_args*)arg;
    float base = sa->samps->data[sa->pnum];
    float top = sa->samps->data[sa->pnum + 1];
    int s=0;
    for(int i=0; sa->data[i]; i++) {
    	if (sa->data[i]>=base && sa->data[i]<=top) s++;
    }
    sa->sizes[sa->pnum]=s;
    floats* xs = make_floats(s);
    for(int i=0; sa->data[i]; i++) {
    	if (sa->data[i]>=base && sa->data[i]<=top) {
    		xs->data[s] = sa->data[i];
    		s++;
    	}
    }

    printf("%d: start %.04f, count %ld\n", sa->pnum, sa->samps->data[sa->pnum], xs->size);

    // TODO: some other stuff
    
    // Each process uses quicksort to sort the local array.

    qsort_floats(xs);

    // TODO: probably more stuff
    
    /* Each process calculates where to put its result array as follows:
     *     start = sum(sizes[0 to p - 1]) # that’s sum(zero items) = 0 for p = 0
     *     end = sum(sizes[0 to p]) # that’s sum(1 item) for p = 0
     * Warning: Data race if you don’t synchronize here.
     * Each process copies its sorted array to input[start..end]
     */
     pthread_mutex_lock(&lock);
     int pos=0;
     for(int i=0; i<sa->pnum; i++) pos+=sa->sizes[sa->pnum-1];
     
     for(int i=0; i<xs->size; i++, pos++) sa->data[pos] = xs->data[i]; 
	pthread_mutex_unlock(&lock);
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
     
    for (int ii = 0; ii < P; ++ii) {
        struct sort_args sa;
        sa.pnum=ii;
        sa.data=data;
        sa.size=size;
        sa.P=P;
        sa.samps=samps;
        sa.sizes=sizes;
        sa.bb=bb;
        int rv;
        rv = pthread_create(&kids[ii], NULL, &sort_worker, (void *)&sa);
        if (rv) {
            printf("Error:unable to create thread, %d\n", rv);
            exit(1);
        }
    }
    
    for (int ii = 0; ii < P; ++ii) {
        int rv;
        rv = pthread_join(kids[ii], NULL);
        if (rv) {
                printf("Error:unable to join thread %d, %d\n", ii, rv);
                exit(1);
        }
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
    
    /*for (int ii = 2; ii < count; ++ii) {
        printf("%.04f\n", data[ii]);
    }*/

    free_barrier(bb);

    munmap(file, st.st_size);

    return 0;
}

