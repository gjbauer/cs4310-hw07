// Author: Nat Tuck
// CS3650 starter code

#include <stdlib.h>
#include <stdio.h>

#include "float_vec.h"

/*
void merge(int arr[], int l, int m, int r)
{
    int i, j, k;
    int n1 = m - l + 1;
    int n2 = r - m;

    // Create temp arrays
    int L[n1], R[n2];

    // Copy data to temp arrays L[] and R[]
    for (i = 0; i < n1; i++)
        L[i] = arr[l + i];
    for (j = 0; j < n2; j++)
        R[j] = arr[m + 1 + j];

    // Merge the temp arrays back into arr[l..r
    i = 0;
    j = 0;
    k = l;
    while (i < n1 && j < n2) {
        if (L[i] <= R[j]) {
            arr[k] = L[i];
            i++;
        }
        else {
            arr[k] = R[j];
            j++;
        }
        k++;
    }

    // Copy the remaining elements of L[],
    // if there are any
    while (i < n1) {
        arr[k] = L[i];
        i++;
        k++;
    }

    // Copy the remaining elements of R[],
    // if there are any
    while (j < n2) {
        arr[k] = R[j];
        j++;
        k++;
    }
}
*/

floats*
make_floats(long nn)
{
    floats* xs = malloc(sizeof(floats));
    xs->size = nn;
    xs->cap  = (nn > 1) ? nn : 2;
    xs->data = malloc(xs->cap * sizeof(float));
    return xs;
}

void
floats_push(floats* xs, float xx)
{
    if (xs->size >= xs->cap) {
        xs->cap *= 2;
        xs->data = realloc(xs->data, xs->cap * sizeof(float));
    }

    xs->data[xs->size] = xx;
    xs->size += 1;
}

void
free_floats(floats* xs, int P)
{
    free(xs);
}

void
floats_print(floats* xs, int P)
{
    floats *pntr;
    for (int i=0; i<P; i++) {
	pntr = &xs[i];
        for (int ii = 0; ii < pntr->size; ++ii) {
            printf("%.04f\n", pntr->data[ii]);
        }
        printf("\n");
    }
}


