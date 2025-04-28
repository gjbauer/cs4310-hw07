#ifndef SSORT_H
#define SSORT_H
#include "float_vec.h"
#include "barrier.h"
#include "utils.h"
struct sort_args {
    int pnum;
    float* data;
    long size;
    int P;
    floats* samps;
    long* sizes;
    barrier* bb;
};
#endif