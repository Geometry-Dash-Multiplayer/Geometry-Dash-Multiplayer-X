#include "tasks.hpp"
#include "requests.hpp"
#include <utils/logging.hpp>
#include <eos/portable_iarchive.hpp>
#include <boost/asio/experimental/awaitable_operators.hpp>
namespace asio = boost::asio;
using namespace std::chrono_literals;
using namespace asio::experimental::awaitable_operators;
using asio::ip::udp;

BackgroundTasks::BackgroundTasks(report_callback&&                 report,
                                 const std::optional<handle_type>& handle)
    : socket(ctx), lookup_socket(ctx), report(std::move(report))
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

    asio::co_spawn(ctx, cleanup(), asio::detached);
  }
  else
    asio::co_spawn(ctx, dynamic_cast<JoinedLobby&>(*lobby).keepAlive(),
                   asio::detached);

  asio::co_spawn(ctx, listen(), asio::detached);
}

asio::awaitable<void> BackgroundTasks::listenForNewClients()
{
  while (true)
  {
    asio::streambuf buffer;
    udp::endpoint   sender;

    size_t len = co_await lookup_socket.async_receive_from(
        buffer.prepare(max_response_size), sender);

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
      log::error("Archive Exception - {}", exception.what());
      if (type != RequestType::Empty)
        log::error("Request was {}: {}", fmt::underlying(type), type);
    }
  }
}

asio::awaitable<void> BackgroundTasks::listen()
{
  while (true)
  {
    asio::streambuf buffer;
    udp::endpoint   sender;

    size_t len = co_await socket.async_receive_from(
        buffer.prepare(max_response_size), sender);

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
      log::error("Archive Exception! Reason: {}", exception.what());
      if (type != RequestType::Empty)
        log::error("Request was {}: {}", fmt::underlying(type), type);
    }
  }
}

asio::awaitable<void> BackgroundTasks::cleanup()
{
  auto* const lobby = HostedLobby::get();

  while (true)
  {
    co_await asio::steady_timer(ctx, 5s).async_wait();
    const auto current = std::chrono::steady_clock::now();

    for (auto itr = lobby->players.begin(); itr != lobby->players.end();)
    {
      auto& [id, item] = *itr;
      if ((current - item.last_update) < 5s)
      {
        ++itr;
        continue;
      }

      if (item.level_id)
        lobby->playerExitedLevel(id, item.level_id);
      itr = lobby->players.erase(itr);
    }
  }
}
