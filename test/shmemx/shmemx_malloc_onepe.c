/*
 *  Copyright (c) 2023 Intel Corporation. All rights reserved.
 *  This software is available to you under the BSD license below:
 *
 *      Redistribution and use in source and binary forms, with or
 *      without modification, are permitted provided that the following
 *      conditions are met:
 *
 *      - Redistributions of source code must retain the above
 *        copyright notice, this list of conditions and the following
 *        disclaimer.
 *
 *      - Redistributions in binary form must reproduce the above
 *        copyright notice, this list of conditions and the following
 *        disclaimer in the documentation and/or other materials
 *        provided with the distribution.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 * NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS
 * BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN
 * ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

#include <stdio.h>
#include <stdlib.h>
#include <shmem.h>
#include <shmemx.h>

int main(int argc, char **argv) {
    int ret = 0;

    int onepe_data_size = 10;
    int *onepe_alloc = NULL;
    int i;

    static ptrdiff_t onepe_alloc_ptr_original = 0, onepe_alloc_ptr_retrieved = 0;
    int *onepe_alloc_val_pe1;

    shmem_init();

    int me = shmem_my_pe();
    int npes = shmem_n_pes();

    if (me == 1) {
        onepe_alloc = (int *) shmemx_malloc_onepe(onepe_data_size * sizeof(int));
        for (i = 0; i < onepe_data_size; i++)
            onepe_alloc[i] = npes + 78 + i;
        onepe_alloc_ptr_original = shmemx_ptrdiff_of_ptr(onepe_alloc);	
    }

    shmem_barrier_all();

    if (me != 1) {
        onepe_alloc_val_pe1 = (int *) malloc(onepe_data_size * sizeof(int));

        shmem_ptrdiff_get(&onepe_alloc_ptr_retrieved, &onepe_alloc_ptr_original, 1, 1);
        void *onepe_alloc_pe1 = shmemx_ptr_of_ptrdiff(onepe_alloc_ptr_retrieved);
        shmem_int_get(onepe_alloc_val_pe1, onepe_alloc_pe1, onepe_data_size, 1);
    }

    shmem_barrier_all();

    if (me != 1) {
        for (i = 0; i < onepe_data_size; i++) {
            if (onepe_alloc_val_pe1[i] != (npes + 78 + i)) {
                fprintf(stderr, "ERROR: Wrong value retrieved from one_pe allocation %d, expected = %d at index %d\n", 
                                 onepe_alloc_val_pe1[i], npes + 78 + i, i);
                ret = 1;
                break;
            }
        }
    }

    if (me == 1)
        shmemx_free_onepe(onepe_alloc);
    else
        free(onepe_alloc_val_pe1);

    shmem_finalize();

    return ret;
}
