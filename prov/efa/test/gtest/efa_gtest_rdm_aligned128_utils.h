/* SPDX-License-Identifier: BSD-2-Clause OR GPL-2.0-only */
/* SPDX-FileCopyrightText: Copyright Amazon.com, Inc. or its affiliates. All rights reserved. */

#ifndef EFA_GTEST_RDM_ALIGNED128_UTILS_H
#define EFA_GTEST_RDM_ALIGNED128_UTILS_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <rdma/fi_domain.h>
#include <rdma/fi_endpoint.h>

#ifdef __cplusplus
extern "C" {
#endif

#define EFA_TEST_ALIGNED128_DATA_SIZE 256

/**
 * @brief Whether the RDM endpoint supports RDMA read.
 */
bool efa_test_rdm_ep_support_rdma_read(struct fid_ep *ep);

/**
 * @brief Set the RDM endpoint's use_device_rdma; returns the previous value.
 */
bool efa_test_rdm_ep_set_use_device_rdma(struct fid_ep *ep, bool value);

/**
 * @brief Make the RDM endpoint's read copy packet pool present or absent for
 * the setopt check only: "present" borrows the endpoint's rx packet pool
 * pointer when no read copy pool exists. Returns the previous pointer, to be
 * passed back to efa_test_rdm_ep_restore_readcopy_pool().
 */
void *efa_test_rdm_ep_set_readcopy_pool_present(struct fid_ep *ep, bool present);
void efa_test_rdm_ep_restore_readcopy_pool(struct fid_ep *ep, void *pool);

/**
 * @brief Set the RDM endpoint's hmem_p2p_opt; returns the previous value.
 */
int efa_test_rdm_ep_set_hmem_p2p_opt(struct fid_ep *ep, int opt);

/**
 * @brief Set the RDM endpoint's msg_prefix_size; returns the previous value.
 */
size_t efa_test_rdm_ep_set_msg_prefix_size(struct fid_ep *ep, size_t size);

/**
 * @brief Value of the RDM endpoint's sendrecv_in_order_aligned_128_bytes.
 */
bool efa_test_rdm_ep_sendrecv_aligned_128(struct fid_ep *ep);

struct efa_test_aligned128_copy_result {
	int ret;
	uint64_t bytes_copied;
};

/**
 * @brief Copy one eager rx packet of EFA_TEST_ALIGNED128_DATA_SIZE bytes into
 * an rxe through efa_rdm_pke_copy_payload_to_ope(), with
 * sendrecv_in_order_aligned_128_bytes set to @p aligned. The receive buffer
 * is host memory; its desc is NULL when @p iface is FI_HMEM_SYSTEM and
 * @p with_desc is false, else an MR with @p iface. The message is longer than
 * the packet, so it never completes. When @p release_pke is true the packet
 * is released after the call (for a mocked local read that did not take it).
 */
int efa_test_aligned128_copy_payload(struct fid_ep *ep, enum fi_hmem_iface iface,
				     bool with_desc, bool aligned, bool release_pke,
				     struct efa_test_aligned128_copy_result *result);

#ifdef __cplusplus
}
#endif

#endif /* EFA_GTEST_RDM_ALIGNED128_UTILS_H */
