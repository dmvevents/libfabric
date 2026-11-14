/* SPDX-License-Identifier: BSD-2-Clause OR GPL-2.0-only */
/* SPDX-FileCopyrightText: Copyright Amazon.com, Inc. or its affiliates. All rights reserved. */

#include "efa_gtest_common_helpers.h"
#include "efa_gtest_common_mocks.h"
#include "efa_gtest_common_resource.h"
#include "efa_gtest_rdm_aligned128_utils.h"
#include "fi_ext_efa.h"
#include <cstring>
#include <gtest/gtest.h>

using testing::_;
using testing::Return;
using testing::StrictMock;
using testing::Test;
using testing::TestWithParam;
using testing::ValuesIn;

class EfaRdmAligned128SetoptTest : public Test
{
	protected:
	struct efa_resource resource = {};
	StrictMock<MockEfa> mock_efa;
	bool saved_use_device_rdma = false;
	void *saved_readcopy_pool = nullptr;
	size_t saved_msg_prefix_size = 0;

	void SetUp() override
	{
		memset(&resource, 0, sizeof(resource));
		efa_test_resource_construct(
			&resource, efa_test_alloc_default_hints(
					   FI_EP_RDM, EFA_FABRIC_NAME));
		ASSERT_NE(resource.ep, nullptr);
		saved_use_device_rdma =
			efa_test_rdm_ep_set_use_device_rdma(resource.ep, true);
		saved_readcopy_pool =
			efa_test_rdm_ep_set_readcopy_pool_present(resource.ep, true);
		saved_msg_prefix_size =
			efa_test_rdm_ep_set_msg_prefix_size(resource.ep, 0);
		MockEfa::set(&mock_efa);
	}

	void TearDown() override
	{
		MockEfa::set(nullptr);
		if (resource.ep) {
			efa_test_rdm_ep_set_msg_prefix_size(resource.ep,
							    saved_msg_prefix_size);
			efa_test_rdm_ep_restore_readcopy_pool(resource.ep,
							      saved_readcopy_pool);
			efa_test_rdm_ep_set_use_device_rdma(resource.ep,
							    saved_use_device_rdma);
		}
		efa_test_resource_destruct(&resource);
	}

	int set_bool(int optname, bool value)
	{
		return fi_setopt(&resource.ep->fid, FI_OPT_ENDPOINT, optname,
				 &value, sizeof(value));
	}

	int set_aligned(bool value)
	{
		return set_bool(FI_OPT_EFA_SENDRECV_IN_ORDER_ALIGNED_128_BYTES,
				value);
	}
};

class EfaRdmAligned128SetoptRdmaReadTest : public EfaRdmAligned128SetoptTest
{
	protected:
	void SetUp() override
	{
		EfaRdmAligned128SetoptTest::SetUp();
		if (!efa_test_rdm_ep_support_rdma_read(resource.ep))
			GTEST_SKIP() << "device does not support RDMA read";
	}
};

TEST_F(EfaRdmAligned128SetoptTest, refused_without_rdma_read)
{
	efa_test_rdm_ep_set_use_device_rdma(resource.ep, false);
	EFA_EXPECT_CALL(mock_efa, efa_base_ep_check_qp_in_order_aligned_128_bytes)
		.Times(0);

	EXPECT_EQ(set_aligned(true), -FI_EOPNOTSUPP);
	EXPECT_FALSE(efa_test_rdm_ep_sendrecv_aligned_128(resource.ep));
}

TEST_F(EfaRdmAligned128SetoptTest, disabling_needs_no_check)
{
	EFA_EXPECT_CALL(mock_efa, efa_base_ep_check_qp_in_order_aligned_128_bytes)
		.Times(0);

	EXPECT_EQ(set_aligned(false), 0);
	EXPECT_FALSE(efa_test_rdm_ep_sendrecv_aligned_128(resource.ep));
}

TEST_F(EfaRdmAligned128SetoptRdmaReadTest, refused_without_readcopy_pool)
{
	efa_test_rdm_ep_set_readcopy_pool_present(resource.ep, false);
	EFA_EXPECT_CALL(mock_efa, efa_base_ep_check_qp_in_order_aligned_128_bytes)
		.Times(0);

	EXPECT_EQ(set_aligned(true), -FI_EOPNOTSUPP);
	EXPECT_FALSE(efa_test_rdm_ep_sendrecv_aligned_128(resource.ep));
}

TEST_F(EfaRdmAligned128SetoptRdmaReadTest, refused_with_msg_prefix)
{
	efa_test_rdm_ep_set_msg_prefix_size(resource.ep, 64);
	EFA_EXPECT_CALL(mock_efa, efa_base_ep_check_qp_in_order_aligned_128_bytes)
		.Times(0);

	EXPECT_EQ(set_aligned(true), -FI_EOPNOTSUPP);
	EXPECT_FALSE(efa_test_rdm_ep_sendrecv_aligned_128(resource.ep));
}

TEST_F(EfaRdmAligned128SetoptRdmaReadTest, accepted_when_qp_supports_aligned_read)
{
	EFA_EXPECT_CALL(mock_efa, efa_base_ep_check_qp_in_order_aligned_128_bytes,
			_, IBV_WR_RDMA_READ)
		.Times(1)
		.WillOnce(Return(0));

	EXPECT_EQ(set_aligned(true), 0);
	EXPECT_TRUE(efa_test_rdm_ep_sendrecv_aligned_128(resource.ep));
}

TEST_F(EfaRdmAligned128SetoptRdmaReadTest, refused_when_qp_lacks_aligned_read)
{
	EFA_EXPECT_CALL(mock_efa, efa_base_ep_check_qp_in_order_aligned_128_bytes,
			_, IBV_WR_RDMA_READ)
		.Times(1)
		.WillOnce(Return(-FI_EOPNOTSUPP));

	EXPECT_EQ(set_aligned(true), -FI_EOPNOTSUPP);
	EXPECT_FALSE(efa_test_rdm_ep_sendrecv_aligned_128(resource.ep));
}

TEST_F(EfaRdmAligned128SetoptRdmaReadTest, device_rdma_cannot_be_disabled_while_set)
{
	EFA_EXPECT_CALL(mock_efa, efa_base_ep_check_qp_in_order_aligned_128_bytes,
			_, IBV_WR_RDMA_READ)
		.Times(1)
		.WillOnce(Return(0));

	ASSERT_EQ(set_aligned(true), 0);
	EXPECT_EQ(set_bool(FI_OPT_EFA_USE_DEVICE_RDMA, false), -FI_EOPNOTSUPP);
	EXPECT_TRUE(efa_test_rdm_ep_support_rdma_read(resource.ep));
}

struct EfaAligned128CopyCase {
	const char *name;
	enum fi_hmem_iface iface;
	bool with_desc;
};

static const EfaAligned128CopyCase kAligned128CopyCases[] = {
	{"system_no_desc", FI_HMEM_SYSTEM, false},
	{"system_desc", FI_HMEM_SYSTEM, true},
	{"rocr", FI_HMEM_ROCR, true},
};

class EfaRdmAligned128CopyFixture
{
	protected:
	struct efa_resource resource = {};
	StrictMock<MockEfa> mock_efa;
	bool saved_rocr_initialized = false;
	bool saved_rocr_p2p = false;
	int saved_p2p_opt = 0;

	void construct(bool rocr_p2p)
	{
		memset(&resource, 0, sizeof(resource));
		efa_test_resource_construct(
			&resource, efa_test_alloc_default_hints(
					   FI_EP_RDM, EFA_FABRIC_NAME));
		ASSERT_NE(resource.ep, nullptr);
		saved_rocr_initialized =
			efa_test_set_hmem_initialized(FI_HMEM_ROCR, true);
		saved_rocr_p2p =
			efa_test_set_hmem_p2p_supported(FI_HMEM_ROCR, rocr_p2p);
		saved_p2p_opt = efa_test_rdm_ep_set_hmem_p2p_opt(
			resource.ep, rocr_p2p ? FI_HMEM_P2P_ENABLED
					      : FI_HMEM_P2P_PREFERRED);
		MockEfa::set(&mock_efa);
	}

	void destruct()
	{
		MockEfa::set(nullptr);
		if (resource.ep)
			efa_test_rdm_ep_set_hmem_p2p_opt(resource.ep, saved_p2p_opt);
		efa_test_set_hmem_p2p_supported(FI_HMEM_ROCR, saved_rocr_p2p);
		efa_test_set_hmem_initialized(FI_HMEM_ROCR, saved_rocr_initialized);
		efa_test_resource_destruct(&resource);
	}
};

class EfaRdmAligned128CopyTest : public TestWithParam<EfaAligned128CopyCase>,
				 public EfaRdmAligned128CopyFixture
{
	protected:
	void SetUp() override { construct(true); }
	void TearDown() override { destruct(); }
};

TEST_P(EfaRdmAligned128CopyTest, aligned_rx_uses_local_read)
{
	const EfaAligned128CopyCase &c = GetParam();
	struct efa_test_aligned128_copy_result result = {};

	EFA_EXPECT_CALL(mock_efa, efa_rdm_rxe_post_local_read_or_queue, _, 0u,
			_, _, (size_t) EFA_TEST_ALIGNED128_DATA_SIZE)
		.Times(1)
		.WillOnce(Return(0));

	ASSERT_EQ(efa_test_aligned128_copy_payload(resource.ep, c.iface,
						   c.with_desc, true, true,
						   &result),
		  0);
	EXPECT_EQ(result.ret, 0);
	EXPECT_EQ(result.bytes_copied, 0u);
}

INSTANTIATE_TEST_SUITE_P(
	, EfaRdmAligned128CopyTest, ValuesIn(kAligned128CopyCases),
	[](const testing::TestParamInfo<EfaAligned128CopyCase> &info) {
		return std::string(info.param.name);
	});

class EfaRdmAligned128CopyNoP2pTest : public Test,
				      public EfaRdmAligned128CopyFixture
{
	protected:
	void SetUp() override { construct(false); }
	void TearDown() override { destruct(); }
};

TEST_F(EfaRdmAligned128CopyNoP2pTest, aligned_rx_without_p2p_fails)
{
	struct efa_test_aligned128_copy_result result = {};

	EFA_EXPECT_CALL(mock_efa, efa_rdm_rxe_post_local_read_or_queue).Times(0);

	ASSERT_EQ(efa_test_aligned128_copy_payload(resource.ep, FI_HMEM_ROCR,
						   true, true, false, &result),
		  0);
	EXPECT_EQ(result.ret, -FI_EINVAL);
	EXPECT_EQ(result.bytes_copied, 0u);
}

TEST_F(EfaRdmAligned128CopyNoP2pTest, unaligned_host_rx_copies_in_place)
{
	struct efa_test_aligned128_copy_result result = {};

	EFA_EXPECT_CALL(mock_efa, efa_rdm_rxe_post_local_read_or_queue).Times(0);

	ASSERT_EQ(efa_test_aligned128_copy_payload(resource.ep, FI_HMEM_SYSTEM,
						   false, false, false, &result),
		  0);
	EXPECT_EQ(result.ret, 0);
	EXPECT_EQ(result.bytes_copied, (uint64_t) EFA_TEST_ALIGNED128_DATA_SIZE);
}
