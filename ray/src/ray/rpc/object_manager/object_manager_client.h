#ifndef RAY_RPC_OBJECT_MANAGER_CLIENT_H
#define RAY_RPC_OBJECT_MANAGER_CLIENT_H

#include <thread>

#include <grpcpp/grpcpp.h>
#include <grpcpp/resource_quota.h>
#include <grpcpp/support/channel_arguments.h>

#include "ray/common/status.h"
#include "ray/util/logging.h"
#include "src/ray/protobuf/object_manager.grpc.pb.h"
#include "src/ray/protobuf/object_manager.pb.h"
#include "src/ray/rpc/grpc_client.h"

namespace ray {
namespace rpc {

/// Client used for communicating with a remote node manager server.
class ObjectManagerClient {
 public:
  /// Constructor.
  ///
  /// \param[in] address Address of the node manager server.
  /// \param[in] port Port of the node manager server.
  /// \param[in] client_call_manager The `ClientCallManager` used for managing requests.
  ObjectManagerClient(const std::string &address, const int port,
                      ClientCallManager &client_call_manager, int num_connections = 4)
      : client_call_manager_(client_call_manager), num_connections_(num_connections) {
    push_rr_index_ = rand() % num_connections_;
    pull_rr_index_ = rand() % num_connections_;
    freeobjects_rr_index_ = rand() % num_connections_;
    grpc_clients_.reserve(num_connections_);
    for (int i = 0; i < num_connections_; i++) {
      grpc_clients_.emplace_back(new GrpcClient<ObjectManagerService>(
          address, port, client_call_manager, num_connections_));
    }
  };

  /// Push object to remote object manager
  ///
  /// \param request The request message.
  /// \param callback The callback function that handles reply from server
  VOID_RPC_CLIENT_METHOD(ObjectManagerService, Push,
                         grpc_clients_[push_rr_index_++ % num_connections_], )

  /// Pull object from remote object manager
  ///
  /// \param request The request message
  /// \param callback The callback function that handles reply from server
  VOID_RPC_CLIENT_METHOD(ObjectManagerService, Pull,
                         grpc_clients_[pull_rr_index_++ % num_connections_], )

  /// Tell remote object manager to free objects
  ///
  /// \param request The request message
  /// \param callback  The callback function that handles reply
  VOID_RPC_CLIENT_METHOD(ObjectManagerService, FreeObjects,
                         grpc_clients_[freeobjects_rr_index_++ % num_connections_], )

 private:
  /// To optimize object manager performance we create multiple concurrent
  /// GRPC connections, and use these connections in a round-robin way.
  int num_connections_;

  /// Current connection index for `Push`.
  std::atomic<unsigned int> push_rr_index_;
  /// Current connection index for `Pull`.
  std::atomic<unsigned int> pull_rr_index_;
  /// Current connection index for `FreeObjects`.
  std::atomic<unsigned int> freeobjects_rr_index_;

  /// The RPC clients.
  std::vector<std::unique_ptr<GrpcClient<ObjectManagerService>>> grpc_clients_;

  /// The `ClientCallManager` used for managing requests.
  ClientCallManager &client_call_manager_;
};

}  // namespace rpc
}  // namespace ray

#endif  // RAY_RPC_OBJECT_MANAGER_CLIENT_H