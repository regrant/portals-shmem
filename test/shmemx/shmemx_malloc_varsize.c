/*
 *  Copyright (c) 2020 Intel Corporation. All rights reserved.
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

#include <shmem.h>
#include <shmemx.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

int main(int argc, char *argv[]) {
  int ret = 0, i, errors = 0;

  shmem_init();

  int me = shmem_my_pe();
  int npes = shmem_n_pes();

  int *symm_malloc_1 = (int *) shmem_malloc(npes * sizeof(int));
  int *var_alloc = (int *) shmemx_malloc_varsize((me + 1) * 1024 * 1024 * sizeof(int), npes * 1024 * 1024 * sizeof(int) + 1024 * 1024);
  int *symm_malloc_2 = (int *) shmem_align((size_t) sysconf(_SC_PAGESIZE), npes * 1024 * 1024 * sizeof(int));

  int *src = (int *) malloc((me + 1) * sizeof(int));

  if (!var_alloc || !src || !symm_malloc_1 || !symm_malloc_2) {
    fprintf(stdout, "malloc failed\n");
    ret = -1;
    goto fn_end;
  }

  fprintf(stdout, "[PE %d]: symm_1 %p, var %p, symm_2 %p\n", me, symm_malloc_1, var_alloc, symm_malloc_2);

  for (i = 0; i <= me; i++) {
    src[i] = me + 1 + i;
    var_alloc[i] = 0;
  }


  shmem_barrier_all();
  shmem_int_put(var_alloc, src, ((me + 1) % npes) + 1, (me + 1) % npes);
  shmem_barrier_all();

  for (i = 0; i < me; i++) {
    if (var_alloc[i] != (me + i)) {
      errors++;
      fprintf(stdout, "[PE %d]: Invalid data found for var_alloc at %d is %d\n", me, i, var_alloc[i]);
    }
  }
  ret = errors;

  int temp = 109 + me;
  shmem_int_put(var_alloc + (me + 1) * 1024 * 1024 + 4096 + 24, &temp, 1, (me + 1) % npes);
  shmem_barrier_all();
  fprintf(stdout, "[PE %d] Undefined behv?: %d\n", me, var_alloc[(me + 1) * 1024 * 1024 + 4096 + 24]);

  shmem_int_put(symm_malloc_1, src, ((me + 1) % npes) + 1, (me + 1) % npes);
  shmem_barrier_all();

  for (i = 0; i < me; i++) {
    if (symm_malloc_1[i] != (me + i)) {
      errors++;
      fprintf(stdout, "[PE %d]: Invalid data found for symm_malloc_1 at %d is %d\n", me, i, symm_malloc_1[i]);
    }
  }
  ret = errors;

  shmem_int_put(symm_malloc_2, src, ((me + 1) % npes) + 1, (me + 1) % npes);
  shmem_barrier_all();

  for (i = 0; i < me; i++) {
    if (symm_malloc_2[i] != (me + i)) {
      errors++;
      fprintf(stdout, "[PE %d]: Invalid data found for symm_malloc_2 at %d is %d\n", me, i, symm_malloc_2[i]);
    }
  }
  ret = errors;

  shmem_free(symm_malloc_1);
  free(src);
  shmem_free(var_alloc);
  shmem_free(symm_malloc_2);

  if (ret == 0) fprintf(stdout, "[PE %d] TEST SUCCESS\n", me);
 
fn_end:
  shmem_finalize();
  return ret;
}
