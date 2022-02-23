// Copyright 2021 Google LLC
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//      http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#include "cases/loopback_fixture.h"

#include <algorithm>
#include <cstdint>
#include <utility>

#include "absl/status/statusor.h"
#include "infiniband/verbs.h"
#include "public/status_matchers.h"
#include "public/verbs_util.h"

namespace rdma_unit_test {

absl::StatusOr<LoopbackFixture::Client> LoopbackFixture::CreateClient(
    ibv_qp_type qp_type, int pages) {
  Client client;
  client.buffer = ibv_.AllocBuffer(pages);
  std::fill_n(client.buffer.data(), client.buffer.size(), '-');
  ASSIGN_OR_RETURN(client.context, ibv_.OpenDevice());
  client.port_gid = ibv_.GetLocalPortGid(client.context);
  client.pd = ibv_.AllocPd(client.context);
  if (!client.pd) {
    return absl::InternalError("Failed to allocate pd.");
  }
  client.cq = ibv_.CreateCq(client.context);
  if (!client.cq) {
    return absl::InternalError("Failed to create cq.");
  }
  ibv_qp_init_attr attr = {.send_cq = client.cq,
                           .recv_cq = client.cq,
                           .srq = nullptr,
                           .cap = verbs_util::DefaultQpCap(),
                           .qp_type = qp_type,
                           .sq_sig_all = 0};
  client.qp = ibv_.CreateQp(client.pd, attr);
  if (!client.qp) {
    return absl::InternalError("Failed to create qp.");
  }
  // memory setup.
  client.mr = ibv_.RegMr(client.pd, client.buffer);
  if (!client.mr) {
    return absl::InternalError("Failed to register mr.");
  }
  return client;
}

absl::StatusOr<LoopbackFixture::BasicSetup> LoopbackFixture::CreateBasicSetup(
    int pages) {
  BasicSetup setup;
  setup.buffer = ibv_.AllocBuffer(pages);
  std::fill_n(setup.buffer.data(), setup.buffer.size(), '-');
  ASSIGN_OR_RETURN(setup.context, ibv_.OpenDevice());
  setup.port_gid = ibv_.GetLocalPortGid(setup.context);
  setup.pd = ibv_.AllocPd(setup.context);
  if (!setup.pd) {
    return absl::InternalError("Failed to allocate pd.");
  }
  setup.cq = ibv_.CreateCq(setup.context);
  if (!setup.cq) {
    return absl::InternalError("Failed to create cq.");
  }
  ibv_qp_init_attr attr = {.send_cq = setup.cq,
                           .recv_cq = setup.cq,
                           .srq = nullptr,
                           .cap = verbs_util::DefaultQpCap(),
                           .qp_type = IBV_QPT_RC,
                           .sq_sig_all = 0};
  setup.local_qp = ibv_.CreateQp(setup.pd, attr);
  if (!setup.local_qp) {
    return absl::InternalError("Failed to create local qp.");
  }
  setup.remote_qp = ibv_.CreateQp(setup.pd, attr);
  if (!setup.remote_qp) {
    return absl::InternalError("Failed to create remote qp.");
  }
  RETURN_IF_ERROR(ibv_.SetUpLoopbackRcQps(setup.local_qp, setup.remote_qp));
  setup.mr = ibv_.RegMr(setup.pd, setup.buffer);
  if (!setup.mr) {
    return absl::InternalError("Failed to register mr.");
  }
  return setup;
}

}  // namespace rdma_unit_test
