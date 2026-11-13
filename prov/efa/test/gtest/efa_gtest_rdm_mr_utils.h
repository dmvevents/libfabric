/* SPDX-License-Identifier: BSD-2-Clause OR GPL-2.0-only */
/* SPDX-FileCopyrightText: Copyright Amazon.com, Inc. or its affiliates. All rights reserved. */

#ifndef EFA_GTEST_RDM_MR_UTILS_H
#define EFA_GTEST_RDM_MR_UTILS_H

#include <stdbool.h>
#include <rdma/fi_domain.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Set the hmem iface recorded on an MR; returns the previous iface.
 */
enum fi_hmem_iface efa_test_set_mr_iface(struct fid_mr *mr,
					 enum fi_hmem_iface iface);

/**
 * @brief Whether an RDM MR is owned by the MR cache.
 */
bool efa_test_rdm_mr_in_cache(struct fid_mr *mr);

#ifdef __cplusplus
}
#endif

#endif /* EFA_GTEST_RDM_MR_UTILS_H */
