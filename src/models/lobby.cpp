#include "lobby.hpp"
#include "gdmx_manager.hpp"
#include <boost/serialization/vector.hpp>
#include <net/listener.hpp>
#include <net/request.hpp>
#include <eos/portable_iarchive.hpp>
#include <eos/portable_oarchive.hpp>
#include <utils/logging.hpp>
#include <hooks/game_layer.hpp>
#include <Geode/binding/GJGameLevel.hpp>
#ifdef _MSC_VER
  #pragma warning(push)
  #pragma warning(disable : 4267)
#endif
#include <boost/asio/experimental/awaitable_operators.hpp>
#ifdef _MSC_VER
  #pragma warning(pop)
#endif
namespace asio = boost::asio;
using asio::ip::udp;
using namespace std::chrono_literals;
using namespace asio::experimental::awaitable_operators;

ActiveLobby* ActiveLobby::get() { return GDMXManager::get().getActiveLobby(); }

void ActiveLobby::dispatch(eos::portable_iarchive& archive, RequestType type)
{
  output::warn("Unhandled Request - {}", to_string(type));
}

ActiveLobby::~ActiveLobby()
{
  while (!prequest_listener)
    std::this_thread::yield();
  prequest_listener->stop();
}

void ActiveLobby::spawnMainThread(
    const std::optional<socket_handle_type>& socket_handle)
{
  thread_listener.bind(
      [](event_type* pevent)
      {
        if (!pevent->getProgress())
          return;
        auto& [type, data] = *pevent->getProgress();
        if (auto* game_layer = GDMXGameLayer::get())
          game_layer->dispatch(type, data);
      });

  thread_listener.setFilter(LobbyMainThread::run(
      [socket_handle](report_callback    report,
                      cancelled_callback cancelled) -> result_type
      {
        RequestListener listener{ socket_handle };
        if (auto* lobby = ActiveLobby::get())
        {
          lobby->prequest_listener = &listener;
          lobby->report            = std::move(report);
          lobby->spawnCoroutines();
          listener.run();
        }
        else
          return false;

        output::debug("main lobby thread terminated");
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
    output::debug("player with id {} requested to unjoin", id);
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
  case RequestType::PlayerSync:
  {
    uint64_t id = 0;
    SyncData info{};
    archive >> id >> info;
    playerSyncAcross(id, info);
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
        prequest_listener->ctx,
        [this, sender]() -> asio::awaitable<void>
        {
          Request request{ prequest_listener->lookup_socket,
                           RequestType::SendLobby };
          request << data();
          co_await request.send_to(sender);
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
        prequest_listener->ctx,
        [this, sender]() -> asio::awaitable<void>
        {
          Request request{ prequest_listener->lookup_socket,
                           RequestType::JoinSuccessful };
          co_await request.send_to(sender);
        },
        asio::detached);
    output::debug("player with id {} requested to join, reporting back...",
                  player.id);
    break;
  }
  default:
    output::warn("Unhandled Request - {} ({})", type, fmt::underlying(type));
  }
}

void HostedLobby::enteredLevel(uint32_t level_id)
{
  asio::co_spawn(
      prequest_listener->ctx,
      reportPlayerEnteredLevel(GDMXPlayer::self(lobby.type == LobbyType::Local),
                               level_id),
      asio::detached);
}

void HostedLobby::exitedLevel(uint32_t level_id)
{
  asio::co_spawn(
      prequest_listener->ctx,
      reportPlayerExitedLevel(
          GDMXManager::get().getID(lobby.type == LobbyType::Local), level_id),
      asio::detached);
}

void HostedLobby::syncAcross(const SyncData& info)
{
  asio::co_spawn(prequest_listener->ctx,
                 reportPlayerSyncAcross(
                     GDMXManager::get().getID(lobby.type == LobbyType::Local),
                     GJBaseGameLayer::get()->m_level->m_levelID, info),
                 asio::detached);
}

void HostedLobby::playerEnteredLevel(uint64_t id, uint32_t level_id)
{
  auto item = players.find(id);
  if (item == players.end())
    return;
  item->second.last_update = std::chrono::steady_clock::now();
  asio::co_spawn(prequest_listener->ctx,
                 sendLevelPlayers(id, level_id, item->second.source),
                 asio::detached);
  if (item->second.level_id == level_id)
    return;
  item->second.level_id = level_id;
  asio::co_spawn(prequest_listener->ctx,
                 reportPlayerEnteredLevel(item->second.player, level_id),
                 asio::detached);
  report({ EventType::PlayerEnteredLevel,
           std::pair(item->second.player, level_id) });
}

void HostedLobby::playerExitedLevel(uint64_t id, uint32_t level_id)
{
  auto item = players.find(id);
  if (item == players.end())
    return;
  item->second.last_update = std::chrono::steady_clock::now();
  asio::co_spawn(
      prequest_listener->ctx,
      [this, target = item->second.source]() -> asio::awaitable<void>
      {
        Request request{ prequest_listener->socket,
                         RequestType::PlayerExitSuccessful };
        co_await request.send_to(target);
      },
      asio::detached);
  if (item->second.level_id != level_id)
    return;
  item->second.level_id = 0;
  asio::co_spawn(prequest_listener->ctx, reportPlayerExitedLevel(id, level_id),
                 asio::detached);
  report({ EventType::PlayerExitedLevel, std::pair(id, level_id) });
}

void HostedLobby::playerSyncAcross(uint64_t id, const SyncData& info)
{
  auto item = players.find(id);
  if (item == players.end())
    return;
  item->second.last_update = std::chrono::steady_clock::now();
  asio::co_spawn(prequest_listener->ctx,
                 reportPlayerSyncAcross(id, item->second.level_id, info),
                 asio::detached);
  report({ EventType::PlayerSync, std::pair(id, info) });
}

uint64_t HostedLobby::hostID()
{
  return GDMXManager::get().getID(lobby.type == LobbyType::Local);
}

std::string_view HostedLobby::name() { return lobby.name; }

LobbyType HostedLobby::type() { return lobby.type; }

#ifdef _MSC_VER
  #pragma warning(push)
  #pragma warning(disable : 4267)
#endif

uint32_t HostedLobby::playerCount() { return players.size(); }

#ifdef _MSC_VER
  #pragma warning(pop)
#endif

asio::awaitable<void> HostedLobby::reportShutdown(udp::socket& socket)
{
  if (players.empty())
    co_return;

  Request request{ socket, RequestType::ServerShutdown };
  co_await reportTo(request, players);

  output::debug("reported shutdown to a total of {} clients", players.size());
}

void HostedLobby::spawnCoroutines()
{
  asio::co_spawn(prequest_listener->ctx, cleanup(), asio::detached);
}

asio::awaitable<void> HostedLobby::sendLevelPlayers(uint64_t      target_id,
                                                    uint32_t      level_id,
                                                    udp::endpoint target)
{
  std::vector<GDMXPlayer> level_players;
  level_players.reserve(players.size());

  for (PlayerItem& item : players |
                              std::views::filter(
                                  [target_id, level_id](auto& item) {
                                    return item.second.level_id == level_id &&
                                           item.first != target_id;
                                  }) |
                              std::views::values)
    level_players.push_back(item.player);

  Request request{ prequest_listener->socket,
                   RequestType::PlayerEnterSuccessful };
  request << level_players;
  co_await request.send_to(target);
}

asio::awaitable<void> HostedLobby::reportPlayerEnteredLevel(GDMXPlayer player,
                                                            uint32_t   level_id)
{
  const auto filter = [&player, level_id](const auto& item)
  { return item.second.level_id == level_id && item.first != player.id; };
  auto first = std::ranges::find_if(players, filter);
  if (first == players.end())
    co_return;

  Request request{ prequest_listener->socket, RequestType::PlayerEnteredLevel };
  request << player;

  co_await reportTo(request, std::ranges::subrange(first, players.end()) |
                                 std::views::filter(filter));
}

asio::awaitable<void> HostedLobby::reportPlayerExitedLevel(uint64_t id,
                                                           uint32_t level_id)
{
  const auto filter = [level_id](const auto& item)
  { return item.second.level_id == level_id; };
  auto first = std::ranges::find_if(players, filter);
  if (first == players.end())
    co_return;

  Request request{ prequest_listener->socket, RequestType::PlayerExitedLevel };
  request << id;

  co_await reportTo(request, std::ranges::subrange(first, players.end()) |
                                 std::views::filter(filter));
}

asio::awaitable<void> HostedLobby::reportPlayerSyncAcross(uint64_t id,
                                                          uint32_t level_id,
                                                          SyncData sync_info)
{
  const auto filter = [id, level_id](const auto& item)
  { return item.second.level_id == level_id && item.first != id; };
  auto first = std::ranges::find_if(players, filter);
  if (first == players.end())
    co_return;

  Request request{ prequest_listener->socket, RequestType::PlayerSync };
  request << id << sync_info;

  co_await reportTo(request, std::ranges::subrange(first, players.end()) |
                                 std::views::filter(filter));
}

asio::awaitable<void> HostedLobby::cleanup()
{
  while (true)
  {
    co_await asio::steady_timer(prequest_listener->ctx, 5s).async_wait();
    const auto current_time = std::chrono::steady_clock::now();

    for (auto itr = players.begin(); itr != players.end();)
    {
      auto& [id, item] = *itr;
      if ((current_time - item.last_update) < 5s)
      {
        ++itr;
        continue;
      }

      if (item.level_id)
        playerExitedLevel(id, item.level_id);
      itr = players.erase(itr);
      output::debug("player with id {} was automatically removed from the "
                    "lobby due to being afk for too long",
                    id);
    }
  }
}

void JoinedLobby::dispatch(eos::portable_iarchive& archive, RequestType type)
{
  switch (type)
  {
  case RequestType::PlayerEnteredLevel:
  {
    GDMXPlayer player{};
    archive >> player;
    report({ EventType::PlayerEnteredLevel, std::pair(player, 0) });
    break;
  }
  case RequestType::PlayerExitedLevel:
  {
    uint64_t id = 0;
    archive >> id;
    report({ EventType::PlayerExitedLevel, std::pair(id, 0) });
    break;
  }
  case RequestType::PlayerEnterSuccessful:
  {
    enter_success.emit(asio::cancellation_type::all);
    output::debug("entering level was reported successfully");
    break;
  }
  case RequestType::PlayerExitSuccessful:
  {
    exit_success.emit(asio::cancellation_type::all);
    output::debug("exiting level was reported successfully");
    break;
  }
  case RequestType::PlayerSync:
  {
    uint64_t id = 0;
    SyncData info{};
    archive >> id >> info;
    report({ EventType::PlayerSync, std::pair(id, info) });
    break;
  }
  case RequestType::ServerShutdown:
  {
    output::debug("received server shutdown request, unjoining lobby...");
    GDMXManager::get().unjoinLobby();
    break;
  }
  default: ActiveLobby::dispatch(archive, type);
  }
}

void JoinedLobby::enteredLevel(uint32_t level_id)
{
  asio::co_spawn(
      prequest_listener->ctx, reportPlayerLevelChange(level_id, true),
      asio::bind_cancellation_slot(enter_success.slot(), asio::detached));
}

void JoinedLobby::exitedLevel(uint32_t level_id)
{
  asio::co_spawn(
      prequest_listener->ctx, reportPlayerLevelChange(level_id, false),
      asio::bind_cancellation_slot(exit_success.slot(), asio::detached));
}

void JoinedLobby::syncAcross(const SyncData& info)
{
  asio::co_spawn(
      prequest_listener->ctx,
      [this, info]() -> asio::awaitable<void>
      {
        Request request{ prequest_listener->socket, RequestType::PlayerSync };
        request << GDMXManager::get().getID(lobby.type == LobbyType::Local)
                << info;
        co_await request.send_to(server);
      },
      asio::detached);
}

void JoinedLobby::spawnCoroutines()
{
  asio::co_spawn(prequest_listener->ctx, keepAlive(), asio::detached);
}

asio::awaitable<void> JoinedLobby::reportPlayerLevelChange(uint32_t level_id,
                                                           bool     enter)
{
  Request request{ prequest_listener->socket,
                   enter ? RequestType::PlayerEnteredLevel
                         : RequestType::PlayerExitedLevel };
  request << GDMXManager::get().getID(lobby.type == LobbyType::Local)
          << level_id;

  while (true)
    co_await (request.send_to(server, asio::use_awaitable) &&
              asio::steady_timer(prequest_listener->ctx, 250ms)
                  .async_wait(asio::use_awaitable));
}

asio::awaitable<void> JoinedLobby::keepAlive()
{
  Request request{ prequest_listener->socket, RequestType::RetainJoin };
  request << GDMXManager::get().getID(lobby.type == LobbyType::Local);

  while (true)
    co_await (request.send_to(server, asio::use_awaitable) &&
              asio::steady_timer(prequest_listener->ctx, 500ms)
                  .async_wait(asio::use_awaitable));
}
