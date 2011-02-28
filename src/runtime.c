/* -*- C -*-
 *
 * Copyright (c) 2011 Sandia National Laboratories. All rights reserved.
 */

#include "config.h"

#include <portals4.h>
#include <portals4_runtime.h>
#include <stdlib.h>
#include <unistd.h>

#include "mpp/shmem.h"
#include "shmem_internal.h"

ptl_handle_ni_t ni_h;
ptl_pt_index_t pt_entry = PTL_PT_ANY;
ptl_handle_md_t md_h;
ptl_handle_le_t le_h;
ptl_handle_ct_t target_ct_h;
ptl_handle_ct_t source_ct_h;
ptl_handle_ct_t source_eq_h;
ptl_handle_eq_t err_eq_h;
ptl_size_t max_ordered_size = 0;
ptl_size_t pending_counter = 0;

void
start_pes(int npes)
{
    int ret;
    ptl_process_t *mapping;
    ptl_md_t md;
    ptl_le_t le;
    ptl_jid_t jid = PTL_JID_ANY;
    ptl_ni_limits_t ni_limits;

    /* Fix me: PTL_INVALID_HANDLE isn't constant in the current
       implementation.  Needs to be, but work around for now */
    ni_h = PTL_INVALID_HANDLE;
    md_h = PTL_INVALID_HANDLE;
    le_h = PTL_INVALID_HANDLE;
    target_ct_h = PTL_INVALID_HANDLE;
    source_ct_h = PTL_INVALID_HANDLE;
    source_eq_h = PTL_INVALID_HANDLE;
    err_eq_h = PTL_INVALID_HANDLE;
    
    /* Initialize Portals */
    ret = PtlInit();
    if (PTL_OK != ret) goto cleanup;
    mapping  = malloc(sizeof(ptl_process_t) * runtime_get_size());
    ret = PtlNIInit(PTL_IFACE_DEFAULT,
                    PTL_NI_NO_MATCHING | PTL_NI_LOGICAL,
                    PTL_PID_ANY,
                    NULL,
                    &ni_limits,
                    runtime_get_size(),
                    NULL,
                    mapping,
                    &ni_h);
    if (PTL_OK != ret) goto cleanup;
    max_ordered_size = ni_limits.max_ordered_size;

#if 0
    PtlGetJid(ni_h, &jid);
#endif

    /* create symmetric allocation */
    ret = symmetric_init();
    if (0 != ret) goto cleanup;

    /* create portal table entry */
    ret = PtlEQAlloc(ni_h, 64, &err_eq_h);
    if (PTL_OK != ret) goto cleanup;
    ret = PtlPTAlloc(ni_h,
                     0,
                     err_eq_h,
                     SHMEM_IDX,
                     &pt_entry);
    if (PTL_OK != ret) goto cleanup;

    /* Open LE to all memory */
    ret = PtlCTAlloc(ni_h, &target_ct_h);
    if (PTL_OK != ret) goto cleanup;

    le.start = 0;
    le.length = SIZE_MAX;
    le.ct_handle = target_ct_h;
    le.ac_id.jid = jid;
    le.options = PTL_LE_OP_PUT | PTL_LE_OP_GET | 
        PTL_LE_EVENT_SUCCESS_DISABLE | 
        PTL_LE_EVENT_CT_COMM;
    ret = PtlLEAppend(ni_h,
                      pt_entry,
                      &le,
                      PTL_PRIORITY_LIST,
                      NULL,
                      &le_h);
    if (PTL_OK != ret) goto cleanup;

    /* Open MD to all memory */
    ret = PtlCTAlloc(ni_h, &source_ct_h);
    if (PTL_OK != ret) goto cleanup;
    ret = PtlEQAlloc(ni_h, 64, &source_eq_h);
    if (PTL_OK != ret) goto cleanup;

    md.start = 0;
    md.length = SIZE_MAX;
    md.options = PTL_MD_EVENT_CT_ACK | 
        PTL_MD_REMOTE_FAILURE_DISABLE;
    md.eq_handle = source_eq_h;
    md.ct_handle = source_ct_h;
    ret = PtlMDBind(ni_h,
                    &md,
                    &md_h);
    if (PTL_OK != ret) goto cleanup;

    /* finish up */
    runtime_barrier();
    return;

 cleanup:
    if (!PtlHandleIsEqual(md_h, PTL_INVALID_HANDLE)) {
        PtlMDRelease(md_h);
    }
    if (!PtlHandleIsEqual(source_ct_h, PTL_INVALID_HANDLE)) {
        PtlCTFree(source_ct_h);
    }
    if (!PtlHandleIsEqual(source_eq_h, PTL_INVALID_HANDLE)) {
        PtlEQFree(source_eq_h);
    }
    if (!PtlHandleIsEqual(le_h, PTL_INVALID_HANDLE)) {
        PtlLEUnlink(le_h);
    }
    if (!PtlHandleIsEqual(target_ct_h, PTL_INVALID_HANDLE)) {
        PtlCTFree(target_ct_h);
    }
    if (PTL_PT_ANY != pt_entry) {
        PtlPTFree(ni_h, pt_entry);
    }
    if (PtlHandleIsEqual(err_eq_h, PTL_INVALID_HANDLE)) {
        PtlEQFree(err_eq_h);
    }
    if (PtlHandleIsEqual(ni_h, PTL_INVALID_HANDLE)) {
        PtlNIFini(ni_h);
    }
    PtlFini();
    /* BWB: FIX ME: should probably be a bit more subtle here */
    abort();
}


int
shmem_my_pe(void)
{
    return runtime_get_rank();
}


int
_my_pe(void)
{
    return shmem_my_pe();
}


int
shmem_n_pes(void)
{
    return runtime_get_size();
}


int
_num_pes(void)
{
    return shmem_n_pes();
}


int 
shmem_pe_accessible(int pe)
{
    if (pe > 0 && pe < shmem_n_pes()) {
        return 1;
    } else {
        return 0;
    }
}


int
shmem_addr_accessible(void *addr, int pe)
{
    /* BWB: This could probably be implemented better */
    return 1;
}
