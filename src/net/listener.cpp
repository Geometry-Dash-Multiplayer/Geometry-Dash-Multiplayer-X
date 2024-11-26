#include "listener.hpp"
#include "constants.hpp"
#include <models/lobby.hpp>
#include <utils/logging.hpp>
#include <eos/portable_iarchive.hpp>
#include <boost/asio/experimental/awaitable_operators.hpp>
namespace asio = boost::asio;
using namespace std::chrono_literals;
using namespace asio::experimental::awaitable_operators;
using asio::ip::udp;

RequestListener::RequestListener(const std::optional<handle_type>& handle)
    : socket(ctx), lookup_socket(ctx)
{
  if (handle)
    socket.assign(udp::v4(), *handle);
  else
    socket.open(udp::v4());

  auto* lobby = ActiveLobby::get();
  if (lobby->isHost())
  {
    lookup_socket.open(udp::v4());

    if (lobby->type() == LobbyType::Local)
    {
      if (!handle)
        socket.bind(udp::endpoint(udp::v4(), main_socket_port));
      lookup_socket.bind(udp::endpoint(udp::v4(), lookup_socket_port));

      asio::co_spawn(ctx, listenForNewClients(), asio::detached);
    }
  }

  asio::co_spawn(ctx, listen(), asio::detached);
}

asio::awaitable<void> RequestListener::listenForNewClients()
{
  while (true)
  {
    asio::streambuf buffer;
    udp::endpoint   sender;

    size_t len = co_await lookup_socket.async_receive_from(
        buffer.prepare(max_response_size), sender);

    if (len >= max_response_size)
      output::warn(
          "received a response that is potentially of larger size than the "
          "maximum allowed, the data stored may be corrupted");

    buffer.commit(len);

    auto type = RequestType::Empty;

    try
    {
      eos::portable_iarchive archive{ buffer };
      archive >> type;

      HostedLobby::get()->dispatchLookup(archive, type, sender);
    }
    catch (const boost::archive::archive_exception& exception)
    {
      output::error("Archive Exception - {}", exception.what());
      if (type != RequestType::Empty)
        output::error("Request was {}: {}", fmt::underlying(type), type);
    }
  }
}

asio::awaitable<void> RequestListener::listen()
{
  while (true)
  {
    asio::streambuf buffer;
    udp::endpoint   sender;

    size_t len = co_await socket.async_receive_from(
        buffer.prepare(max_response_size), sender);

    if (len >= max_response_size)
      output::warn(
          "received a response that is potentially of larger size than the "
          "maximum allowed, the data stored may be corrupted");

    buffer.commit(len);

    auto type = RequestType::Empty;

    try
    {
      eos::portable_iarchive archive{ buffer };
      archive >> type;

      ActiveLobby::get()->dispatch(archive, type);
    }
    catch (const boost::archive::archive_exception& exception)
    {
      output::error("Archive Exception! Reason: {}", exception.what());
      if (type != RequestType::Empty)
        output::error("Request was {}: {}", fmt::underlying(type), type);
    }
  }
}
