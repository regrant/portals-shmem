dnl Copyright (c) 2016 Intel Corporation. All rights reserved.
dnl This software is available to you under the BSD license.
dnl
dnl This file is part of the Sandia OpenSHMEM software package. For license
dnl information, see the LICENSE file in the top level directory of the
dnl distribution.
dnl
dnl This file contains the m4 macros used to generate the SOS C11 generic
dnl selection interfaces
dnl
include(`shmem_bind_c11_extras.m4')
dnl
define(`SHMEM_BIND_C11_RMA', dnl args: macro_name, end
SHMEM_BIND_C11_RMA_EXTRAS($1,$2)dnl
`$1(float,      float)$2
$1(double,     double)$2
$1(longdouble, long double)$2
$1(char,       char)$2
$1(schar,      signed char)$2
$1(short,      short)$2
$1(int,        int)$2
$1(long,       long)$2
$1(longlong,   long long)$2
$1(uchar,      unsigned char)$2
$1(ushort,     unsigned short)$2
$1(uint,       unsigned int)$2
$1(ulong,      unsigned long)$2
$1(ulonglong,  unsigned long long)')dnl
dnl
define(`SHMEM_BIND_C11_AMO', dnl args: macro_name, end
SHMEM_BIND_C11_AMO_EXTRAS($1,$2)dnl
`$1(int,        int,                `SHM_INTERNAL_INT')$2
$1(long,       long,               `SHM_INTERNAL_LONG')$2
$1(longlong,   long long,          `SHM_INTERNAL_LONG_LONG')$2
$1(uint,       unsigned int,       `SHM_INTERNAL_UINT')$2
$1(ulong,      unsigned long,      `SHM_INTERNAL_ULONG')$2
$1(ulonglong,  unsigned long long, `SHM_INTERNAL_ULONG_LONG')')dnl
dnl
define(`SHMEM_BIND_C11_EXTENDED_AMO', dnl args: macro_name, end
SHMEM_BIND_C11_EXTENDED_AMO_EXTRAS($1,$2)dnl
`$1(int,        int,                `SHM_INTERNAL_INT')$2
$1(long,       long,               `SHM_INTERNAL_LONG')$2
$1(longlong,   long long,          `SHM_INTERNAL_LONG_LONG')$2
$1(uint,       unsigned int,       `SHM_INTERNAL_UINT')$2
$1(ulong,      unsigned long,      `SHM_INTERNAL_ULONG')$2
$1(ulonglong,  unsigned long long, `SHM_INTERNAL_ULONG_LONG')$2
$1(float,      float,              `SHM_INTERNAL_FLOAT')$2
$1(double,     double,             `SHM_INTERNAL_DOUBLE')')dnl
dnl
define(`SHMEM_BIND_C11_BITWISE_AMO', dnl args: macro_name, end
SHMEM_BIND_C11_BITWISE_AMO_EXTRAS($1,$2)dnl
`$1(int32,      int32_t,            `SHM_INTERNAL_INT32')$2
$1(int64,      int64_t,            `SHM_INTERNAL_INT64')$2
$1(uint,       unsigned int,       `SHM_INTERNAL_UINT')$2
$1(ulong,      unsigned long,      `SHM_INTERNAL_ULONG')$2
$1(ulonglong,  unsigned long long, `SHM_INTERNAL_ULONG_LONG')')dnl
dnl
define(`SHMEM_BIND_C11_WAIT', dnl args: macro_name
`$1(short,      short)
$1(int,        int)
$1(long,       long)
$1(longlong,   long long)')dnl
