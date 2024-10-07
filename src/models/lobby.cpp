#include "lobby.hpp"
#include "gdmx_manager.hpp"
#include "requests.hpp"
#include "tasks.hpp"
#include "game_layer.hpp"
#include <boost/archive/binary_oarchive.hpp>
#include <boost/asio/experimental/parallel_group.hpp>
#include <ranges>
namespace asio = boost::asio;
using asio::ip::udp;

ActiveLobby* ActiveLobby::get() { return GDMXManager::get().getActiveLobby(); }

ActiveLobby::ActiveLobby(LobbyType type, std::string_view name)
    : lobby(GDMXManager::get().getID(type == LobbyType::Local), type, name)
{
  spawnMainThread(std::nullopt);
}

ActiveLobby::ActiveLobby(const Lobby&                             lobby,
                         const std::optional<socket_handle_type>& socket_handle)
    : lobby(lobby)
{
  spawnMainThread(socket_handle);
}

ActiveLobby::~ActiveLobby()
{
  while (!ptasks)
    std::this_thread::yield();
  ptasks->ctx.stop();
}

void ActiveLobby::spawnMainThread(
    const std::optional<socket_handle_type>& socket_handle)
{
  listener.bind(
      [](event_type* pevent)
      {
        if (!pevent->getProgress())
          return;
        // auto& [type, data] = *pevent->getProgress();
        assert(false && "unhandled event type");
      });

  listener.setFilter(LobbyMainThread::run(
      [socket_handle](report_callback    report,
                      cancelled_callback cancelled) -> result_type
      {
        BackgroundTasks tasks{ std::move(report), socket_handle };
        if (auto* lobby = ActiveLobby::get())
        {
          lobby->ptasks = &tasks;
          tasks.run();
        }
        else
          return false;

        geode::log::debug("main lobby thread terminated");
        if (cancelled())
          return cancel_type{};
        return true;
      },
      "GDMX Lobby thread"));
}

HostedLobby* HostedLobby::get()
{
  if (auto* lobby = ActiveLobby::get())
    return lobby->isHost() ? static_cast<HostedLobby*>(lobby) : nullptr;
  return nullptr;
}

void HostedLobby::erasePlayer(uint64_t id)
{
  if (!players.erase(id))
    return;
  asio::co_spawn(ptasks->ctx, reportPlayerRemoval(id), asio::detached);
}

void HostedLobby::registerPlayer(const GDMXPlayer& player,
                                 const endpoint&   sender)
{
  players[player.id] = { player, sender };
  asio::co_spawn(ptasks->ctx, reportPlayerRegistration(player.id),
                 asio::detached);
}

asio::awaitable<void> HostedLobby::reportPlayerRemoval(uint64_t id)
{
  asio::streambuf                 buffer;
  std::ostream                    stream{ &buffer };
  boost::archive::binary_oarchive archive{ stream };
  archive << RequestType::PlayerRemove << id;

  using op_type = decltype(ptasks->socket.async_send_to(
      buffer.data(), std::declval<udp::endpoint>()));
  std::vector<op_type> operations;
  operations.reserve(players.size());

  for (const PlayerItem& item : players | std::views::values)
    operations.push_back(
        ptasks->socket.async_send_to(buffer.data(), item.source));

  co_await asio::experimental::make_parallel_group(operations)
      .async_wait(asio::experimental::wait_for_all(), asio::deferred);
}

asio::awaitable<void> HostedLobby::reportPlayerRegistration(uint64_t id)
{
  asio::streambuf                 buffer;
  std::ostream                    stream{ &buffer };
  boost::archive::binary_oarchive archive{ stream };

  {
    auto item = players.find(id);
    if (item == players.end())
      co_return;
    archive << RequestType::PlayerAdd << item->second.player;
  }

  using op_type = decltype(ptasks->socket.async_send_to(
      buffer.data(), std::declval<udp::endpoint>()));
  std::vector<op_type> operations;
  operations.reserve(players.size());

  for (const PlayerItem& item :
       players |
           std::views::filter([id](auto& item) { return item.first != id; }) |
           std::views::values)
    operations.push_back(
        ptasks->socket.async_send_to(buffer.data(), item.source));

  co_await asio::experimental::make_parallel_group(operations)
      .async_wait(asio::experimental::wait_for_all(), asio::deferred);
}

JoinedLobby* JoinedLobby::get()
{
  if (auto* lobby = ActiveLobby::get())
    return !lobby->isHost() ? static_cast<JoinedLobby*>(lobby) : nullptr;
  return nullptr;
}

void JoinedLobby::erasePlayer(uint64_t id) { players.erase(id); }

void JoinedLobby::registerPlayer(const GDMXPlayer& player,
                                 const endpoint&   sender)
{
  players[player.id] = player;
}
