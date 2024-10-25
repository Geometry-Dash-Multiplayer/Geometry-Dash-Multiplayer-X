#pragma once
#include <memory>
#include <boost/serialization/string.hpp>
#include <boost/unordered/unordered_flat_map.hpp>
#include <boost/asio.hpp>
#include <Geode/utils/Task.hpp>
#include <Geode/loader/Event.hpp>
#include <net/requests.hpp>
#include "player.hpp"

class BackgroundTasks;

namespace eos
{
  class portable_iarchive;
}

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
  template <typename archive>
  void serialize(archive& arch, const unsigned int version)
  {
    arch & name & type;
  }

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
  template <typename archive>
  void serialize(archive& arch, const unsigned int version)
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

  bool isHost();

  // events
  virtual void enteredLevel(uint32_t level_id) = 0;
  virtual void exitedLevel(uint32_t level_id)  = 0;

  // getters
  virtual uint64_t         hostID()      = 0;
  virtual std::string_view name()        = 0;
  virtual LobbyType        type()        = 0;
  virtual uint32_t         playerCount() = 0;
  virtual Lobby            data()        = 0;

  virtual ~ActiveLobby();

protected:
  using LobbyMainThread = geode::Task<bool, std::pair<EventType, EventValue>>;
  using Listener        = geode::EventListener<LobbyMainThread>;
  using finish_callback = LobbyMainThread::PostResult;
  using report_callback = LobbyMainThread::PostProgress;
  using cancelled_callback = LobbyMainThread::HasBeenCancelled;
  using result_type        = LobbyMainThread::Result;
  using cancel_type        = LobbyMainThread::Cancel;
  using event_type         = LobbyMainThread::Event;

  Listener         listener;
  BackgroundTasks* ptasks = nullptr;

  ActiveLobby()                              = default;
  ActiveLobby(const ActiveLobby&)            = delete;
  ActiveLobby& operator=(const ActiveLobby&) = delete;

  void spawnMainThread(const std::optional<socket_handle_type>& socket_handle);

  friend class GDMXManager;
  friend class BackgroundTasks;
};

class HostedLobby : public ActiveLobby
{
public:
  struct PlayerItem;

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

  void playerEnteredLevel(uint64_t id, uint32_t level_id);

  void playerExitedLevel(uint64_t id, uint32_t level_id);

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

  coro<void> reportPlayerEnteredLevel(GDMXPlayer player, uint32_t level_id);
  coro<void> reportPlayerExitedLevel(uint64_t id, uint32_t level_id);

  friend class BackgroundTasks;
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

  coro<void> reportPlayerLevelChange(uint32_t level_id, bool enter);
  coro<void> keepAlive();

  friend class BackgroundTasks;
};

struct HostedLobby::PlayerItem
{
  GDMXPlayer                     player{};
  boost::asio::ip::udp::endpoint source;
  time_point                     last_update = std::chrono::steady_clock::now();
  uint32_t                       level_id    = 0;

  template <typename archive>
  void serialize(archive& arch, const unsigned int version)
  {
    arch & player;
  }
};

BOOST_CLASS_IMPLEMENTATION(HostedLobby::PlayerItem,
                           boost::serialization::object_serializable);

inline bool ActiveLobby::isHost() { return dynamic_cast<HostedLobby*>(this); }
