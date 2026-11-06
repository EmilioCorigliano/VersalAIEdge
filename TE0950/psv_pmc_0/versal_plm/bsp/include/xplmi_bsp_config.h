/******************************************************************************
# Copyright (C) 2024 - 2025 Advanced Micro Devices, Inc.  All rights reserved.
# SPDX-License-Identifier: MIT
******************************************************************************/
#ifndef XPLMI_BSP_CONFIG_H
#define XPLMI_BSP_CONFIG_H

#include "xparameters.h"

/* #undef PLM_PRINT_NO_UART */
#define 	PLM_SEM_PRINT_OVERRIDE
/* #undef PLM_PRINT_LEVEL0 */
#if ((((defined(XSEM_CFRSCAN_EN)) || (defined(XSEM_NPISCAN_EN))) && (defined(PLM_SEM_PRINT_OVERRIDE))) || (defined(PLM_PRINT_LEVEL0)))
#define PLM_PRINT
#else
#define    PLM_DEBUG
/* #undef PLM_DEBUG_INFO */
/* #undef PLM_DEBUG_DETAILED */
#endif
/* #undef PLM_DEBUG_MODE */
#define    PLM_PRINT_PERF
/* #undef PLM_QSPI_EXCLUDE */
/* #undef PLM_SD_EXCLUDE */
/* #undef PLM_OSPI_EXCLUDE */
/* #undef PLM_SEM_EXCLUDE */
/* #undef PLM_SECURE_EXCLUDE */
#define    PLM_USB_EXCLUDE
#define    PLM_NVM_EXCLUDE
#define    PLM_PUF_EXCLUDE
/* #undef PLM_ENABLE_STL */
/* #undef PLM_OCP_EXCLUDE */
/* #undef PLM_ECDSA_EXCLUDE */
/* #undef PLM_RSA_EXCLUDE */
/* #undef PLM_AUTH_JTAG_EXCLUDE */
/* #undef PLM_EN_ADD_PPKS */
/* #undef PLM_ENABLE_CFI_SELECTIVE_READ */
/* #undef PLM_ENABLE_SHA_AES_EVENTS_QUEUING */
/* #undef PLM_BANNER_TIMESTAMP_EXCLUDE */
/* #undef PLM_ENABLE_RESTORE_MULTIBOOT */
#define    PLM_TPM_EXCLUDE
/* #undef PLM_OCP_ASUFW_KEY_MGMT_EXCLUDE */
/* #undef PLM_CFG_LIMITER_EN */

#define XPAR_MAX_USER_MODULES (0U)
#define XPAR_PLM_VERSION_USER_DEFINED (0U)

#if ((defined(NUMBER_OF_SLRS)) && (NUMBER_OF_SLRS > 1))
#define    PLM_ENABLE_PLM_TO_PLM_COMM
/* #undef PLM_ENABLE_SECURE_PLM_TO_PLM_COMM */
#endif

/* #undef XPLMI_CACHE_DISABLE */

#endif /* XPLMI_BSP_CONFIG_H */
