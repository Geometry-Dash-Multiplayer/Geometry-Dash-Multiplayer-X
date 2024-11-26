#pragma once
#include <ranges>
#include <memory>
#include <boost/asio.hpp>
#include <boost/asio/experimental/parallel_group.hpp>
#include <boost/serialization/string.hpp>
#include <boost/unordered/unordered_flat_map.hpp>
#include <Geode/utils/Task.hpp>
#include <Geode/loader/Event.hpp>
#include <net/request.hpp>
#include "player.hpp"

namespace eos
{
  class portable_iarchive;
}

class RequestListener;

enum class LobbyType : uint8_t
{
  Local,
  Private,
  Public,
  Global
};

class PartialLobby
{
public:
  std::string name;
  LobbyType   type;

  PartialLobby() = default;

  PartialLobby(std::string_view name, LobbyType type) : name(name), type(type)
  {
  }

private:
  void serialize(auto& arch, const unsigned int version) { arch & name & type; }

  friend class boost::serialization::access;
};

class Lobby : public PartialLobby
{
public:
  uint64_t host_id      = 0;
  uint32_t player_count = 0;

  Lobby() = default;

  Lobby(uint64_t host_id, std::string_view name, LobbyType type,
        uint32_t player_count)
      : PartialLobby(name, type), host_id(host_id), player_count(player_count)
  {
  }

  Lobby(const PartialLobby& other, uint64_t host_id, uint32_t player_count)
      : PartialLobby(other), host_id(host_id), player_count(player_count)
  {
  }

private:
  // clang-format off
  void serialize(auto& arch, const unsigned int version)
  {
    arch & boost::serialization::base_object<PartialLobby>(*this);
    arch & host_id & player_count;
  }

  // clang-format on
  friend class boost::serialization::access;
};

class ActiveLobby
{
public:
  template <typename T>
  using coro               = boost::asio::awaitable<T>;
  using socket_handle_type = boost::asio::ip::udp::socket::native_handle_type;
  using endpoint           = boost::asio::ip::udp::endpoint;

  static ActiveLobby* get();

  virtual void dispatch(eos::portable_iarchive& archive, RequestType type);

  // events
  virtual void enteredLevel(uint32_t level_id)  = 0;
  virtual void exitedLevel(uint32_t level_id)   = 0;
  virtual void syncAcross(const SyncData& info) = 0;

  // getters
  virtual bool             isHost()      = 0;
  virtual uint64_t         hostID()      = 0;
  virtual std::string_view name()        = 0;
  virtual LobbyType        type()        = 0;
  virtual uint32_t         playerCount() = 0;
  virtual Lobby            data()        = 0;

  virtual ~ActiveLobby();

protected:
  using LobbyMainThread = geode::Task<bool, std::pair<EventType, EventValue>>;
  using ThreadListener  = geode::EventListener<LobbyMainThread>;
  using finish_callback = LobbyMainThread::PostResult;
  using report_callback = LobbyMainThread::PostProgress;
  using cancelled_callback = LobbyMainThread::HasBeenCancelled;
  using result_type        = LobbyMainThread::Result;
  using cancel_type        = LobbyMainThread::Cancel;
  using event_type         = LobbyMainThread::Event;

  ThreadListener   thread_listener;
  RequestListener* prequest_listener = nullptr;
  report_callback  report;

  ActiveLobby()                              = default;
  ActiveLobby(const ActiveLobby&)            = delete;
  ActiveLobby& operator=(const ActiveLobby&) = delete;

  void spawnMainThread(const std::optional<socket_handle_type>& socket_handle);
  virtual void spawnCoroutines() = 0;

  friend class GDMXManager;
  friend class RequestListener;
};

class HostedLobby : public ActiveLobby
{
public:
  class PlayerItem;

  static HostedLobby* get()
  {
    return dynamic_cast<HostedLobby*>(ActiveLobby::get());
  }

  HostedLobby(std::string_view name, LobbyType type) : lobby(name, type)
  {
    spawnMainThread(std::nullopt);
  }

  void dispatch(eos::portable_iarchive& archive, RequestType type) override;

  void dispatchLookup(eos::portable_iarchive& archive, RequestType type,
                      const endpoint& sender);

  void enteredLevel(uint32_t level_id) override;

  void exitedLevel(uint32_t level_id) override;

  void syncAcross(const SyncData& info) override;

  void playerEnteredLevel(uint64_t id, uint32_t level_id);

  void playerExitedLevel(uint64_t id, uint32_t level_id);

  void playerSyncAcross(uint64_t id, const SyncData& info);

  bool isHost() override { return true; }

  uint64_t         hostID() override;
  std::string_view name() override;
  LobbyType        type() override;
  uint32_t         playerCount() override;

  Lobby data() override { return { lobby, hostID(), playerCount() }; }

  coro<void> reportShutdown(boost::asio::ip::udp::socket& socket);

private:
  using player_map = boost::unordered_flat_map<uint64_t, PlayerItem>;
  using time_point = std::chrono::steady_clock::time_point;

  PartialLobby lobby;
  player_map   players;

  void spawnCoroutines() override;

  coro<void> reportTo(Request& request, std::ranges::range auto&& range);
  coro<void> sendLevelPlayers(uint64_t target_id, uint32_t level_id,
                              endpoint target);
  coro<void> reportPlayerEnteredLevel(GDMXPlayer player, uint32_t level_id);
  coro<void> reportPlayerExitedLevel(uint64_t id, uint32_t level_id);
  coro<void> reportPlayerSyncAcross(uint64_t id, uint32_t level_id,
                                    SyncData sync_info);
  coro<void> cleanup();

  friend class RequestListener;
};

class JoinedLobby : public ActiveLobby
{
public:
  static JoinedLobby* get()
  {
    return dynamic_cast<JoinedLobby*>(ActiveLobby::get());
  }

  JoinedLobby(const Lobby& lobby, const endpoint& server,
              const std::optional<socket_handle_type>& socket_handle)
      : lobby(lobby), server(server)
  {
    spawnMainThread(socket_handle);
  }

  void dispatch(eos::portable_iarchive& archive, RequestType type) override;

  void enteredLevel(uint32_t level_id) override;

  void exitedLevel(uint32_t level_id) override;

  void syncAcross(const SyncData& info) override;

  bool isHost() override { return false; }

  uint64_t hostID() override { return lobby.host_id; }

  std::string_view name() override { return lobby.name; }

  LobbyType type() override { return lobby.type; }

  uint32_t playerCount() override { return lobby.player_count; }

  Lobby data() override { return lobby; }

private:
  Lobby                            lobby;
  endpoint                         server;
  boost::asio::cancellation_signal enter_success;
  boost::asio::cancellation_signal exit_success;

  void spawnCoroutines() override;

  coro<void> reportPlayerLevelChange(uint32_t level_id, bool enter);
  coro<void> keepAlive();

  friend class RequestListener;
};

class HostedLobby::PlayerItem
{
public:
  GDMXPlayer                     player{};
  boost::asio::ip::udp::endpoint source;
  time_point                     last_update = std::chrono::steady_clock::now();
  uint32_t                       level_id    = 0;

private:
  void serialize(auto& arch, const unsigned int version) { arch & player; }

  friend class boost::serialization::access;
};

BOOST_CLASS_IMPLEMENTATION(HostedLobby::PlayerItem,
                           boost::serialization::object_serializable);

boost::asio::awaitable<void>
    HostedLobby::reportTo(Request& request, std::ranges::range auto&& range)
{
  using send_operation = decltype(request.send_to(std::declval<endpoint>()));
  std::vector<send_operation> operations;
  operations.reserve(players.size());

  for (const PlayerItem& item : range | std::views::values)
    operations.push_back(request.send_to(item.source));

  co_await boost::asio::experimental::make_parallel_group(operations)
      .async_wait(boost::asio::experimental::wait_for_all(),
                  boost::asio::deferred);
}
