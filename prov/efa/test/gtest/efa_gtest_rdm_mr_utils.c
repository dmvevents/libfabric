/* SPDX-License-Identifier: BSD-2-Clause OR GPL-2.0-only */
/* SPDX-FileCopyrightText: Copyright Amazon.com, Inc. or its affiliates. All rights reserved. */

#include "efa.h"
#include "efa_mr.h"
#include "rdm/efa_rdm_mr.h"
#include "efa_gtest_rdm_mr_utils.h"

enum fi_hmem_iface efa_test_set_mr_iface(struct fid_mr *mr,
					 enum fi_hmem_iface iface)
{
	struct efa_mr *efa_mr = container_of(mr, struct efa_mr, mr_fid);
	enum fi_hmem_iface prev = efa_mr->iface;

	efa_mr->iface = iface;
	return prev;
}

bool efa_test_rdm_mr_in_cache(struct fid_mr *mr)
{
	struct efa_rdm_mr *efa_rdm_mr =
		container_of(mr, struct efa_rdm_mr, efa_mr.mr_fid);

	return efa_rdm_mr->entry != NULL;
}
