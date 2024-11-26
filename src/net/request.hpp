#pragma once
#include <boost/asio.hpp>
#include <eos/portable_oarchive.hpp>
#include "constants.hpp"

class Request
{
public:
  using socket    = boost::asio::ip::udp::socket;
  using endpoint  = boost::asio::ip::udp::endpoint;
  using streambuf = boost::asio::streambuf;
  using default_completion_token =
      boost::asio::default_completion_token_t<socket::executor_type>;

  Request(socket& sock, RequestType type) : sock(sock) { archive << type; }

  template <typename write_token = default_completion_token>
  auto send_to(const endpoint& destination,
               write_token&&   token = default_completion_token{})
  {
    return sock.async_send_to(buffer.data(), destination,
                              std::forward<write_token>(token));
  }

  template <typename T>
  Request& operator<<(T&& arg)
  {
    archive << std::forward<T>(arg);
    return *this;
  }

private:
  socket&                sock;
  streambuf              buffer;
  eos::portable_oarchive archive{ buffer };
};
