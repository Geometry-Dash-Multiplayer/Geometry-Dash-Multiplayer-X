#pragma once
#include <boost/asio.hpp>
#include <optional>

class RequestListener
{
public:
  template <typename T>
  using coro        = boost::asio::awaitable<T>;
  using handle_type = boost::asio::ip::udp::socket::native_handle_type;

  RequestListener(const std::optional<handle_type>& handle);

  coro<void> listenForNewClients();
  coro<void> listen();

  void run() { ctx.run(); }

  void stop() { ctx.stop(); }

private:
  boost::asio::io_context      ctx;
  boost::asio::ip::udp::socket socket;
  boost::asio::ip::udp::socket lookup_socket;

  RequestListener(const RequestListener&)            = delete;
  RequestListener& operator=(const RequestListener&) = delete;

  friend class ActiveLobby;
  friend class HostedLobby;
  friend class JoinedLobby;
};
