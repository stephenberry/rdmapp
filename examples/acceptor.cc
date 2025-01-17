#include "acceptor.h"

#include <arpa/inet.h>
#include <netdb.h>
#include <netinet/in.h>
#include <rdmapp/detail/debug.h>
#include <rdmapp/device.h>
#include <rdmapp/error.h>
#include <rdmapp/queue_pair.h>
#include <rdmapp/shared_receive_queue.h>
#include <strings.h>
#include <sys/epoll.h>
#include <sys/socket.h>
#include <unistd.h>

#include <cerrno>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <memory>
#include <stdexcept>
#include <thread>
#include <vector>

#include "qp_transmission.h"
#include "socket/channel.h"
#include "socket/tcp_connection.h"
#include "socket/tcp_listener.h"

namespace rdmapp
{

   acceptor::acceptor(std::shared_ptr<socket::event_loop> loop, uint16_t port, std::shared_ptr<protected_domain> pd,
                      std::shared_ptr<completion_queue> cq, std::shared_ptr<shared_receive_queue> srq)
      : acceptor(loop, port, pd, cq, cq, srq)
   {}

   acceptor::acceptor(std::shared_ptr<socket::event_loop> loop, uint16_t port, std::shared_ptr<protected_domain> pd,
                      std::shared_ptr<completion_queue> recv_cq, std::shared_ptr<completion_queue> send_cq, std::shared_ptr<shared_receive_queue> srq)
      : acceptor(loop, "", port, pd, recv_cq, send_cq, srq)
   {}

   acceptor::acceptor(std::shared_ptr<socket::event_loop> loop, const std::string& hostname, uint16_t port,
                      std::shared_ptr<protected_domain> pd, std::shared_ptr<completion_queue> cq, std::shared_ptr<shared_receive_queue> srq)
      : acceptor(loop, hostname, port, pd, cq, cq, srq)
   {}

   acceptor::acceptor(std::shared_ptr<socket::event_loop> loop, const std::string& hostname, uint16_t port,
                      std::shared_ptr<protected_domain> pd, std::shared_ptr<completion_queue> recv_cq, std::shared_ptr<completion_queue> send_cq,
                      std::shared_ptr<shared_receive_queue> srq)
      : listener_(std::make_unique<socket::tcp_listener>(loop, hostname, port)),
        pd_(pd),
        recv_cq_(recv_cq),
        send_cq_(send_cq),
        srq_(srq)
   {}

   task<std::shared_ptr<queue_pair>> acceptor::accept()
   {
      auto channel = co_await listener_->accept();
      auto connection = socket::tcp_connection(channel);
      auto remote_qp = co_await recv_qp(connection);
      auto local_qp = std::make_shared<queue_pair>(remote_qp.header.lid, remote_qp.header.qp_num, remote_qp.header.sq_psn, pd_,
                                           send_cq_, recv_cq_);
      local_qp->user_data() = std::move(remote_qp.user_data);
      co_await send_qp(*local_qp, connection);
      co_return local_qp;
   }

   acceptor::~acceptor() {}

} // namespace rdmapp