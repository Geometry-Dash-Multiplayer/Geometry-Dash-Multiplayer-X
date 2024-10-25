#include "lobby.hpp"
#include "gdmx_manager.hpp"
#include <net/tasks.hpp>
#include <eos/portable_iarchive.hpp>
#include <eos/portable_oarchive.hpp>
#include <utils/logging.hpp>
#include <hooks/game_layer.hpp>
#include <boost/asio/experimental/parallel_group.hpp>
#include <boost/asio/experimental/awaitable_operators.hpp>
#include <ranges>
namespace asio = boost::asio;
using asio::ip::udp;
using namespace std::chrono_literals;
using namespace asio::experimental::awaitable_operators;

ActiveLobby* ActiveLobby::get() { return GDMXManager::get().getActiveLobby(); }

void ActiveLobby::dispatch(eos::portable_iarchive& archive, RequestType type)
{
  switch (type)
  {
  default: log::warn("Unhandled Request - {}", to_string(type));
  }
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
        auto& [type, data] = *pevent->getProgress();
        if (auto* game_layer = GDMXGameLayer::get())
          game_layer->dispatch(type, data);
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

void HostedLobby::dispatch(eos::portable_iarchive& archive, RequestType type)
{
  switch (type)
  {
  case RequestType::UnjoinLobby:
  {
    uint64_t id = 0;
    archive >> id;
    players.erase(id);
    break;
  }
  case RequestType::RetainJoin:
  {
    uint64_t id = 0;
    archive >> id;
    players[id].last_update = std::chrono::steady_clock::now();
    break;
  }
  case RequestType::PlayerEnteredLevel:
  {
    uint64_t id       = 0;
    uint32_t level_id = 0;
    archive >> id >> level_id;
    playerEnteredLevel(id, level_id);
    break;
  }
  case RequestType::PlayerExitedLevel:
  {
    uint64_t id       = 0;
    uint32_t level_id = 0;
    archive >> id >> level_id;
    playerExitedLevel(id, level_id);
    break;
  }
  default: ActiveLobby::dispatch(archive, type);
  }
}

void HostedLobby::dispatchLookup(eos::portable_iarchive& archive,
                                 RequestType type, const endpoint& sender)
{
  switch (type)
  {
  case RequestType::FetchLobbies:
  {
    asio::co_spawn(
        ptasks->ctx,
        [this, sender]() -> asio::awaitable<void>
        {
          asio::streambuf        buffer;
          eos::portable_oarchive archive{ buffer };
          archive << RequestType::SendLobby << data();

          co_await ptasks->lookup_socket.async_send_to(buffer.data(), sender);
        },
        asio::detached);
    break;
  }
  case RequestType::JoinLobby:
  {
    GDMXPlayer player{};
    archive >> player;
    players[player.id] = { player, sender };
    asio::co_spawn(
        ptasks->ctx,
        [this, sender]() -> asio::awaitable<void>
        {
          asio::streambuf        buffer;
          eos::portable_oarchive archive{ buffer };
          archive << RequestType::JoinSuccessful;

          co_await ptasks->lookup_socket.async_send_to(buffer.data(), sender);
        },
        asio::detached);
    break;
  }
  default:
    geode::log::warn("HostedLobby::dispatchLookup: Unhandled Request - {}",
                     to_string(type));
  }
}

void HostedLobby::enteredLevel(uint32_t level_id)
{
  asio::co_spawn(
      ptasks->ctx,
      reportPlayerEnteredLevel(GDMXPlayer::self(lobby.type == LobbyType::Local),
                               level_id),
      asio::detached);
}

void HostedLobby::exitedLevel(uint32_t level_id)
{
  asio::co_spawn(
      ptasks->ctx,
      reportPlayerExitedLevel(
          GDMXManager::get().getID(lobby.type == LobbyType::Local), level_id),
      asio::detached);
}

void HostedLobby::playerEnteredLevel(uint64_t id, uint32_t level_id)
{
  auto item = players.find(id);
  if (item == players.end())
    return;
  item->second.last_update = std::chrono::steady_clock::now();
  asio::co_spawn(
      ptasks->ctx,
      [this, target = item->second.source]() -> asio::awaitable<void>
      {
        asio::streambuf        buffer;
        eos::portable_oarchive archive{ buffer };
        archive << RequestType::PlayerEnterSuccessful;

        co_await ptasks->socket.async_send_to(buffer.data(), target);
      },
      asio::detached);
  if (item->second.level_id == level_id)
    return;
  item->second.level_id = level_id;
  asio::co_spawn(ptasks->ctx,
                 reportPlayerEnteredLevel(item->second.player, level_id),
                 asio::detached);
  ptasks->report({ EventType::PlayerEnteredLevel,
                   std::pair(item->second.player, level_id) });
}

void HostedLobby::playerExitedLevel(uint64_t id, uint32_t level_id)
{
  auto item = players.find(id);
  if (item == players.end())
    return;
  item->second.last_update = std::chrono::steady_clock::now();
  asio::co_spawn(
      ptasks->ctx,
      [this, target = item->second.source]() -> asio::awaitable<void>
      {
        asio::streambuf        buffer;
        eos::portable_oarchive archive{ buffer };
        archive << RequestType::PlayerExitSuccessful;

        co_await ptasks->socket.async_send_to(buffer.data(), target);
      },
      asio::detached);
  if (item->second.level_id != level_id)
    return;
  item->second.level_id = 0;
  asio::co_spawn(ptasks->ctx, reportPlayerExitedLevel(id, level_id),
                 asio::detached);
  ptasks->report({ EventType::PlayerExitedLevel, std::pair(id, level_id) });
}

uint64_t HostedLobby::hostID()
{
  return GDMXManager::get().getID(lobby.type == LobbyType::Local);
}

std::string_view HostedLobby::name() { return lobby.name; }

LobbyType HostedLobby::type() { return lobby.type; }

uint32_t HostedLobby::playerCount() { return players.size(); }

using send_operation = decltype(std::declval<udp::socket>().async_send_to(
    std::declval<asio::streambuf>().data(), std::declval<udp::endpoint>()));

asio::awaitable<void> HostedLobby::reportShutdown(udp::socket& socket)
{
  if (players.empty())
    co_return;

  asio::streambuf        buffer;
  eos::portable_oarchive archive{ buffer };
  archive << RequestType::ServerShutdown;

  std::vector<send_operation> operations;
  operations.reserve(players.size());

  for (const PlayerItem& item : players | std::views::values)
    operations.push_back(socket.async_send_to(buffer.data(), item.source));

  co_await asio::experimental::make_parallel_group(operations)
      .async_wait(asio::experimental::wait_for_all(), asio::deferred);
}

asio::awaitable<void> HostedLobby::reportPlayerEnteredLevel(GDMXPlayer player,
                                                            uint32_t   level_id)
{
  auto first = std::ranges::find_if(
      players, [&player, level_id](const auto& item)
      { return item.second.level_id == level_id && item.first != player.id; });
  if (first == players.end())
    co_return;

  asio::streambuf        buffer;
  eos::portable_oarchive archive{ buffer };
  archive << RequestType::PlayerEnteredLevel << player;

  std::vector<send_operation> operations;
  operations.reserve(players.size());

  for (const PlayerItem& item : std::ranges::subrange(first, players.end()) |
                                    std::views::filter(
                                        [&player, level_id](const auto& item) {
                                          return item.second.level_id ==
                                                     level_id &&
                                                 item.first != player.id;
                                        }) |
                                    std::views::values)
    operations.push_back(
        ptasks->socket.async_send_to(buffer.data(), item.source));

  co_await asio::experimental::make_parallel_group(operations)
      .async_wait(asio::experimental::wait_for_all(), asio::deferred);
}

asio::awaitable<void> HostedLobby::reportPlayerExitedLevel(uint64_t id,
                                                           uint32_t level_id)
{
  auto first =
      std::ranges::find_if(players, [level_id](const auto& item)
                           { return item.second.level_id == level_id; });
  if (first == players.end())
    co_return;

  asio::streambuf        buffer;
  eos::portable_oarchive archive{ buffer };
  archive << RequestType::PlayerExitedLevel << id;

  std::vector<send_operation> operations;
  operations.reserve(players.size());

  for (const PlayerItem& item :
       std::ranges::subrange(first, players.end()) |
           std::views::filter([level_id](const auto& item)
                              { return item.second.level_id == level_id; }) |
           std::views::values)
    operations.push_back(
        ptasks->socket.async_send_to(buffer.data(), item.source));

  co_await asio::experimental::make_parallel_group(operations)
      .async_wait(asio::experimental::wait_for_all(), asio::deferred);
}

void JoinedLobby::dispatch(eos::portable_iarchive& archive, RequestType type)
{
  switch (type)
  {
  case RequestType::PlayerEnteredLevel:
  {
    GDMXPlayer player{};
    archive >> player;
    ptasks->report({ EventType::PlayerEnteredLevel, std::pair(player, 0) });
    break;
  }
  case RequestType::PlayerExitedLevel:
  {
    uint64_t id = 0;
    archive >> id;
    ptasks->report({ EventType::PlayerExitedLevel, std::pair(id, 0) });
    break;
  }
  case RequestType::PlayerEnterSuccessful:
  {
    enter_success.emit(asio::cancellation_type::all);
    break;
  }
  case RequestType::PlayerExitSuccessful:
  {
    exit_success.emit(asio::cancellation_type::all);
    break;
  }
  default: ActiveLobby::dispatch(archive, type);
  }
}

void JoinedLobby::enteredLevel(uint32_t level_id)
{
  asio::co_spawn(
      ptasks->ctx, reportPlayerLevelChange(level_id, true),
      asio::bind_cancellation_slot(enter_success.slot(), asio::detached));
}

void JoinedLobby::exitedLevel(uint32_t level_id)
{
  asio::co_spawn(
      ptasks->ctx, reportPlayerLevelChange(level_id, false),
      asio::bind_cancellation_slot(exit_success.slot(), asio::detached));
}

asio::awaitable<void> JoinedLobby::reportPlayerLevelChange(uint32_t level_id,
                                                           bool     enter)
{
  asio::streambuf        buffer;
  eos::portable_oarchive archive{ buffer };
  archive << (enter ? RequestType::PlayerEnteredLevel
                    : RequestType::PlayerExitSuccessful)
          << GDMXManager::get().getID(lobby.type == LobbyType::Local)
          << level_id;

  while (true)
    co_await (
        ptasks->socket.async_send_to(buffer.data(), server,
                                     asio::use_awaitable) &&
        asio::steady_timer(ptasks->ctx, 250ms).async_wait(asio::use_awaitable));
}

asio::awaitable<void> JoinedLobby::keepAlive()
{
  asio::streambuf        buffer;
  eos::portable_oarchive archive{ buffer };
  archive << RequestType::RetainJoin;

  while (true)
    co_await (
        ptasks->socket.async_send_to(buffer.data(), server,
                                     asio::use_awaitable) &&
        asio::steady_timer(ptasks->ctx, 500ms).async_wait(asio::use_awaitable));
}
