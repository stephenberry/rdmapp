#pragma once

#include "socket/tcp_connection.h"

#include <rdmapp/queue_pair.h>
#include <rdmapp/task.h>

namespace rdmapp {

task<deserialized_qp> recv_qp(socket::tcp_connection &connection);

task<void> send_qp(queue_pair const &qp, socket::tcp_connection &connection);

} // namespace rdmapp