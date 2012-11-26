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

#include "config.h"

#include "shmem.h"
#include "shmem_internal.h"


#define FC_SHMEM_PTR FC_FUNC_(shmem_ptr, SHMEM_PTR)
void* FC_SHMEM_PTR(void **target, int *pe);
void*
FC_SHMEM_PTR(void **target, int *pe)
{
    return shmem_ptr(*target, *pe);
}
