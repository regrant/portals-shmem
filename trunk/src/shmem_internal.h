/* -*- C -*-
 *
 * Copyright 2011 Sandia Corporation. Under the terms of Contract
 * DE-AC04-94AL85000 with Sandia Corporation, the U.S.  Government
 * retains certain rights in this software.
 * 
 * This file is part of the Portals SHMEM software package. For license
 * information, see the LICENSE file in the top level directory of the
 * distribution.
 *
 */

#ifndef PORTALS_SHMEM_INTERNAL_H
#define PORTALS_SHMEM_INTERNAL_H

#include <portals4.h>
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <sys/param.h>

#ifdef USE_PORTALS4
#include "transport_portals4.h"
#endif

#ifdef USE_XPMEM
#include "transport_xpmem.h"
#endif

#ifndef MIN
#define MIN(a,b) (((a)<(b))?(a):(b))
#endif

#define DATA_IDX 10
#define HEAP_IDX 11

extern void *shmem_internal_heap_base;
extern long shmem_internal_heap_length;
extern void *shmem_internal_data_base;
extern long shmem_internal_data_length;

extern int shmem_internal_my_pe;
extern int shmem_internal_num_pes;

extern int shmem_internal_initialized;
extern int shmem_internal_finalized;
extern int shmem_internal_fence_is_quiet;


#define GET_REMOTE_ACCESS(target, pt, offset)                           \
    do {                                                                \
        if (((void*) target > shmem_internal_data_base) &&              \
            ((char*) target < (char*) shmem_internal_data_base + shmem_internal_data_length)) { \
            pt = shmem_internal_data_pt;                                \
            offset = (char*) target - (char*) shmem_internal_data_base; \
        } else if (((void*) target > shmem_internal_heap_base) &&       \
                   ((char*) target < (char*) shmem_internal_heap_base + shmem_internal_heap_length)) { \
            pt = shmem_internal_heap_pt;                                \
            offset = (char*) target - (char*) shmem_internal_heap_base; \
        } else {                                                        \
            printf("[%03d] ERROR: target (0x%lx) outside of symmetric areas\n", \
                   shmem_internal_my_pe, (unsigned long) target);       \
            abort();                                                    \
        }                                                               \
    } while (0)


#define RAISE_ERROR(ret)                                                \
    do {                                                                \
        fprintf(stderr, "[%03d] ERROR: %s:%d return code %d\n",         \
                shmem_internal_my_pe, __FILE__, __LINE__, (int) ret);   \
        abort();                                                        \
    } while (0)


#define RAISE_ERROR_STR(str)                                            \
    do {                                                                \
        fprintf(stderr, "[%03d] ERROR: %s:%d: %s\n",                    \
                shmem_internal_my_pe, __FILE__, __LINE__, str);         \
        abort();                                                        \
    } while (0)

#ifdef USE_ON_NODE_COMMS
extern char *shmem_internal_location_array;
#define SHMEM_SET_RANK_SAME_NODE(pe, node_rank)         \
    do {                                                \
        shmem_internal_location_array[pe] = node_rank;  \
    } while (0)

#define SHMEM_GET_RANK_SAME_NODE(pe) (shmem_internal_location_array[pe])
#else
#define SHMEM_GET_RANK_SAME_NODE(pe) (-1)
#endif
        
static inline
int
shmem_internal_put(void *target, const void *source, size_t len, int pe)
{
    int ret;
    ptl_process_t peer;
    ptl_pt_index_t pt;
    long offset;
    int tmp = 0;
    peer.rank = pe;
    GET_REMOTE_ACCESS(target, pt, offset);

    if (len <= sizeof(long double complex)) {
        ret = PtlPut(shmem_internal_put_md_h,
                     (ptl_size_t) source,
                     len,
                     PTL_CT_ACK_REQ,
                     peer,
                     pt,
                     0,
                     offset,
                     NULL,
                     0);
        if (PTL_OK != ret) { RAISE_ERROR(ret); }
        tmp++;
    } else {
        size_t sent = 0;

        while (sent < len) {
            size_t bufsize = MIN(len - sent, shmem_internal_max_put_size);
            ret = PtlPut(shmem_internal_put_md_h,
                         (ptl_size_t) ((char*) source + sent),
                         bufsize,
                         PTL_CT_ACK_REQ,
                         peer,
                         pt,
                         0,
                         offset + sent,
                         NULL,
                         0);
            if (PTL_OK != ret) { RAISE_ERROR(ret); }
            tmp++;
            sent += bufsize;
        }
    }

    shmem_internal_pending_put_counter += tmp;
    return tmp;
}


static inline
void
shmem_internal_put_wait(int count)
{
#if ENABLE_EVENT_COMPLETION
    int ret;
    ptl_event_t ev;

    for ( ; count > 0 ; --count) {
        ret = PtlEQWait(shmem_internal_put_eq_h, &ev);
        if (PTL_OK != ret) { RAISE_ERROR(ret); }
        if (ev.ni_fail_type != PTL_OK) { RAISE_ERROR(ev.ni_fail_type); }
    }
#endif
}


static inline
void
shmem_internal_get(void *target, const void *source, size_t len, int pe)
{
    int ret;
    ptl_process_t peer;
    ptl_pt_index_t pt;
    long offset;
    peer.rank = pe;
    GET_REMOTE_ACCESS(source, pt, offset);

    ret = PtlGet(shmem_internal_get_md_h,
                 (ptl_size_t) target,
                 len,
                 peer,
                 pt,
                 0,
                 offset,
                 0);
    if (PTL_OK != ret) { RAISE_ERROR(ret); }
    shmem_internal_pending_get_counter++;
}


static inline
void
shmem_internal_get_wait(void)
{
    int ret;
    ptl_ct_event_t ct;

    ret = PtlCTWait(shmem_internal_get_ct_h, 
                    shmem_internal_pending_get_counter,
                    &ct);
    if (PTL_OK != ret) { RAISE_ERROR(ret); }
    if (ct.failure != 0) { RAISE_ERROR(ct.failure); }
}


static inline
int
shmem_internal_swap(void *target, void *source, void *dest, size_t len, 
                    int pe, ptl_datatype_t datatype)
{
    int ret;
    ptl_process_t peer;
    ptl_pt_index_t pt;
    long offset;
    peer.rank = pe;
    GET_REMOTE_ACCESS(target, pt, offset);

    assert(len <= sizeof(long double complex));

    ret = PtlSwap(shmem_internal_get_md_h,
                  (ptl_size_t) dest,
                  shmem_internal_put_md_h,
                  (ptl_size_t) source,
                  len,
                  peer,
                  pt,
                  0,
                  offset,
                  NULL,
                  0,
                  NULL,
                  PTL_SWAP,
                  datatype);
    if (PTL_OK != ret) { RAISE_ERROR(ret); }
    shmem_internal_pending_get_counter++;

    return 1;
}


static inline
int
shmem_internal_cswap(void *target, void *source, void *dest, void *operand, size_t len, 
                     int pe, ptl_datatype_t datatype)
{
    int ret;
    ptl_process_t peer;
    ptl_pt_index_t pt;
    long offset;
    peer.rank = pe;
    GET_REMOTE_ACCESS(target, pt, offset);

    assert(len <= sizeof(long double complex));

    ret = PtlSwap(shmem_internal_get_md_h,
                  (ptl_size_t) dest,
                  shmem_internal_put_md_h,
                  (ptl_size_t) source,
                  len,
                  peer,
                  pt,
                  0,
                  offset,
                  NULL,
                  0,
                  operand,
                  PTL_CSWAP,
                  datatype);
    if (PTL_OK != ret) { RAISE_ERROR(ret); }
    shmem_internal_pending_get_counter++;

    return 1;
}


static inline
int
shmem_internal_mswap(void *target, void *source, void *dest, void *mask, size_t len, 
                     int pe, ptl_datatype_t datatype)
{
    int ret;
    ptl_process_t peer;
    ptl_pt_index_t pt;
    long offset;
    peer.rank = pe;
    GET_REMOTE_ACCESS(target, pt, offset);

    assert(len <= sizeof(long double complex));

    ret = PtlSwap(shmem_internal_get_md_h,
                  (ptl_size_t) dest,
                  shmem_internal_put_md_h,
                  (ptl_size_t) source,
                  len,
                  peer,
                  pt,
                  0,
                  offset,
                  NULL,
                  0,
                  mask,
                  PTL_MSWAP,
                  datatype);
    if (PTL_OK != ret) { RAISE_ERROR(ret); }
    shmem_internal_pending_get_counter++;

    return 1;
}


static inline
int
shmem_internal_atomic(void *target, void *source, size_t len,
                      int pe, ptl_op_t op, ptl_datatype_t datatype)
{
    int ret;
    ptl_pt_index_t pt;
    long offset;
    ptl_process_t peer;
    int tmp = 0;
    peer.rank = pe;
    GET_REMOTE_ACCESS(target, pt, offset);

    if (len <= sizeof(long double complex)) {
        ret = PtlAtomic(shmem_internal_put_md_h,
                        (ptl_size_t) source,
                        len,
                        PTL_CT_ACK_REQ,
                        peer,
                        pt,
                        0,
                        offset,
                        NULL,
                        0,
                        op,
                        datatype);
        if (PTL_OK != ret) { RAISE_ERROR(ret); }
        tmp++;
    } else {
        size_t sent = 0;

        while (sent < len) {
            size_t bufsize = MIN(len - sent, shmem_internal_max_atomic_size);
            ret = PtlAtomic(shmem_internal_put_md_h,
                            (ptl_size_t) ((char*) source + sent),
                            bufsize,
                            PTL_CT_ACK_REQ,
                            peer,
                            pt,
                            0,
                            offset + sent,
                            NULL,
                            0,
                            op,
                            datatype);
            if (PTL_OK != ret) { RAISE_ERROR(ret); }
            tmp++;
            sent += bufsize;
        }
    }

    shmem_internal_pending_put_counter += tmp;
    return tmp;
}


static inline
int
shmem_internal_fetch_atomic(void *target, void *source, void *dest, size_t len,
                            int pe, ptl_op_t op, ptl_datatype_t datatype)
{
    int ret;
    ptl_pt_index_t pt;
    long offset;
    ptl_process_t peer;
    peer.rank = pe;
    GET_REMOTE_ACCESS(target, pt, offset);

    assert(len <= sizeof(long double complex));

    ret = PtlFetchAtomic(shmem_internal_get_md_h,
                         (ptl_size_t) dest,
                         shmem_internal_put_md_h,
                         (ptl_size_t) source,
                         len,
                         peer,
                         pt,
                         0,
                         offset,
                         NULL,
                         0,
                         op,
                         datatype);
    if (PTL_OK != ret) { RAISE_ERROR(ret); }
    shmem_internal_pending_get_counter++;

    return 1;
}

/* initialization routines */
int shmem_internal_symmetric_init(void);
int shmem_internal_symmetric_fini(void);
int shmem_internal_collectives_init(int requested_crossover, 
                                    int requested_radix);

#endif
