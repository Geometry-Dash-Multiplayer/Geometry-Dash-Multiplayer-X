#pragma once
#include <models/lobby.hpp>

class BackgroundTasks
{
public:
  template <typename T>
  using coro = boost::asio::awaitable<T>;
  using finish_callback    = ActiveLobby::finish_callback;
  using report_callback    = ActiveLobby::report_callback;
  using cancelled_callback = ActiveLobby::cancelled_callback;
  using handle_type        = boost::asio::ip::udp::socket::native_handle_type;

  BackgroundTasks(report_callback&&                 report,
                  const std::optional<handle_type>& handle);

  coro<void> listenForNewClients();
  coro<void> listen();
  coro<void> cleanup();

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
