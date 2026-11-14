/* SPDX-License-Identifier: BSD-2-Clause OR GPL-2.0-only */
/* SPDX-FileCopyrightText: Copyright Amazon.com, Inc. or its affiliates. All rights reserved. */

#include "efa.h"
#include "rdm/efa_rdm_ep.h"
#include "rdm/efa_rdm_mr.h"
#include "rdm/efa_rdm_ope.h"
#include "rdm/efa_rdm_pke.h"
#include "rdm/efa_rdm_pke_utils.h"
#include "rdm/efa_rdm_pke_cmd.h"
#include "rdm/efa_rdm_pke_rtm.h"
#include "rdm/efa_rdm_protocol.h"
#include "efa_gtest_rdm_aligned128_utils.h"

static struct efa_rdm_ep *efa_test_rdm_ep(struct fid_ep *ep)
{
	return container_of(ep, struct efa_rdm_ep, base_ep.util_ep.ep_fid);
}

bool efa_test_rdm_ep_support_rdma_read(struct fid_ep *ep)
{
	return efa_rdm_ep_support_rdma_read(efa_test_rdm_ep(ep));
}

bool efa_test_rdm_ep_set_use_device_rdma(struct fid_ep *ep, bool value)
{
	struct efa_rdm_ep *efa_rdm_ep = efa_test_rdm_ep(ep);
	bool prev = efa_rdm_ep->use_device_rdma;

	efa_rdm_ep->use_device_rdma = value;
	return prev;
}

void *efa_test_rdm_ep_set_readcopy_pool_present(struct fid_ep *ep, bool present)
{
	struct efa_rdm_ep *efa_rdm_ep = efa_test_rdm_ep(ep);
	void *prev = efa_rdm_ep->rx_readcopy_pkt_pool;

	if (!present)
		efa_rdm_ep->rx_readcopy_pkt_pool = NULL;
	else if (!efa_rdm_ep->rx_readcopy_pkt_pool)
		efa_rdm_ep->rx_readcopy_pkt_pool = efa_rdm_ep->efa_rx_pkt_pool;
	return prev;
}

void efa_test_rdm_ep_restore_readcopy_pool(struct fid_ep *ep, void *pool)
{
	efa_test_rdm_ep(ep)->rx_readcopy_pkt_pool = pool;
}

int efa_test_rdm_ep_set_hmem_p2p_opt(struct fid_ep *ep, int opt)
{
	struct efa_rdm_ep *efa_rdm_ep = efa_test_rdm_ep(ep);
	int prev = efa_rdm_ep->hmem_p2p_opt;

	efa_rdm_ep->hmem_p2p_opt = opt;
	return prev;
}

size_t efa_test_rdm_ep_set_msg_prefix_size(struct fid_ep *ep, size_t size)
{
	struct efa_rdm_ep *efa_rdm_ep = efa_test_rdm_ep(ep);
	size_t prev = efa_rdm_ep->msg_prefix_size;

	efa_rdm_ep->msg_prefix_size = size;
	return prev;
}

bool efa_test_rdm_ep_sendrecv_aligned_128(struct fid_ep *ep)
{
	return efa_test_rdm_ep(ep)->sendrecv_in_order_aligned_128_bytes;
}

int efa_test_aligned128_copy_payload(struct fid_ep *ep, enum fi_hmem_iface iface,
				     bool with_desc, bool aligned, bool release_pke,
				     struct efa_test_aligned128_copy_result *result)
{
	struct efa_rdm_ep *efa_rdm_ep = efa_test_rdm_ep(ep);
	static char buf[2 * EFA_TEST_ALIGNED128_DATA_SIZE];
	struct efa_rdm_mr mr = {0};
	struct efa_rdm_ope *rxe;
	struct efa_rdm_pke *pke;
	bool saved_aligned = efa_rdm_ep->sendrecv_in_order_aligned_128_bytes;

	mr.efa_mr.iface = iface;

	rxe = efa_rdm_ep_alloc_rxe(efa_rdm_ep, NULL, ofi_op_msg);
	if (!rxe)
		return -FI_ENOMEM;
	rxe->iov_count = 1;
	rxe->iov[0].iov_base = buf;
	rxe->iov[0].iov_len = sizeof(buf);
	rxe->cq_entry.len = sizeof(buf);
	rxe->desc[0] = (with_desc || iface != FI_HMEM_SYSTEM) ? &mr.efa_mr : NULL;
	rxe->total_len = sizeof(buf);

	pke = efa_rdm_pke_alloc(efa_rdm_ep, efa_rdm_ep->efa_rx_pkt_pool,
				EFA_RDM_PKE_FROM_EFA_RX_POOL);
	if (!pke) {
		rxe->desc[0] = NULL;
		efa_rdm_rxe_release(rxe);
		return -FI_ENOMEM;
	}
	efa_rdm_pke_get_base_hdr(pke)->type = EFA_RDM_EAGER_MSGRTM_PKT;
	pke->payload = pke->wiredata;
	pke->payload_size = EFA_TEST_ALIGNED128_DATA_SIZE;

	efa_rdm_ep->sendrecv_in_order_aligned_128_bytes = aligned;
	result->ret = efa_rdm_pke_copy_payload_to_ope(pke, rxe);
	result->bytes_copied = rxe->bytes_copied;
	efa_rdm_ep->sendrecv_in_order_aligned_128_bytes = saved_aligned;

	if (release_pke)
		efa_rdm_pke_release_rx(pke);
	rxe->desc[0] = NULL;
	efa_rdm_rxe_release(rxe);
	return 0;
}
