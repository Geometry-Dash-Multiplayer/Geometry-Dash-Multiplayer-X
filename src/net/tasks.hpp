#pragma once
#include "lobby.hpp"
#include <boost/asio.hpp>

class BackgroundTasks
{
public:
  using finish_callback    = ActiveLobby::finish_callback;
  using report_callback    = ActiveLobby::report_callback;
  using cancelled_callback = ActiveLobby::cancelled_callback;
  using handle_type        = boost::asio::ip::udp::socket::native_handle_type;

  BackgroundTasks(report_callback&&                 report,
                  const std::optional<handle_type>& handle);

  boost::asio::awaitable<void> listenForNewClients();
  boost::asio::awaitable<void> listen();

  void run() { ctx.run(); }

private:
  boost::asio::io_context      ctx;
  boost::asio::ip::udp::socket socket;
  boost::asio::ip::udp::socket lookup_socket;
  report_callback              report;

  BackgroundTasks(const BackgroundTasks&)            = delete;
  BackgroundTasks& operator=(const BackgroundTasks&) = delete;

  friend class ActiveLobby;
  friend class HostedLobby;
  friend class JoinedLobby;
};
