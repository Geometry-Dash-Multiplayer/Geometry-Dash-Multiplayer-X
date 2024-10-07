#pragma once
#include <memory>
#include <chrono>
#include <boost/serialization/string.hpp>
#include <boost/unordered/unordered_flat_map.hpp>
#include <boost/asio.hpp>
#include <Geode/utils/Task.hpp>
#include <Geode/loader/Event.hpp>
#include "player.hpp"
#include "requests.hpp"

class BackgroundTasks;

enum class LobbyType : uint8_t
{
  Local,
  Private,
  Public,
  Global
};

struct Lobby
{
  // note that the host id is also the lobby id since the host can only have one
  // lobby at a time
  uint64_t    host_id = 0;
  std::string name;
  LobbyType   type         = LobbyType::Local;
  uint32_t    player_count = 0;

  Lobby() = default;

  Lobby(uint64_t host_id, LobbyType type, std::string_view name)
      : host_id(host_id), name(name), type(type)
  {
  }

  template <typename archive>
  void serialize(archive& arch, const unsigned int version)
  {
    arch & host_id & name & type & player_count;
  }
};

class ActiveLobby
{
public:
  template <typename T>
  using coro               = boost::asio::awaitable<T>;
  using socket_handle_type = boost::asio::ip::udp::socket::native_handle_type;
  using endpoint           = boost::asio::ip::udp::endpoint;

  static ActiveLobby* get();

  ActiveLobby(LobbyType type, std::string_view name);
  ActiveLobby(const Lobby&                             lobby,
              const std::optional<socket_handle_type>& socket_handle);

  const Lobby& info()
  {
    lobby.player_count = playerCount();
    return lobby;
  }

  virtual bool   isHost()                               = 0;
  virtual size_t playerCount()                          = 0;
  virtual void   erasePlayer(uint64_t id)               = 0;
  virtual void   registerPlayer(const GDMXPlayer& player,
                                const endpoint&   sender) = 0;

  virtual ~ActiveLobby();

protected:
  using LobbyMainThread    = geode::Task<bool, std::pair<EventType, uint64_t>>;
  using Listener           = geode::EventListener<LobbyMainThread>;
  using finish_callback    = LobbyMainThread::PostResult;
  using report_callback    = LobbyMainThread::PostProgress;
  using cancelled_callback = LobbyMainThread::HasBeenCancelled;
  using result_type        = LobbyMainThread::Result;
  using cancel_type        = LobbyMainThread::Cancel;
  using event_type         = LobbyMainThread::Event;

  Lobby            lobby;
  Listener         listener;
  BackgroundTasks* ptasks = nullptr;

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

  static HostedLobby* get();

  HostedLobby(LobbyType type, std::string_view name) : ActiveLobby(type, name)
  {
  }

  bool isHost() override { return true; }

  size_t playerCount() override { return players.size(); }

  void erasePlayer(uint64_t id) override;

  void registerPlayer(const GDMXPlayer& player,
                      const endpoint&   sender) override;

private:
  using player_map = boost::unordered_flat_map<uint64_t, PlayerItem>;
  using time_point = std::chrono::steady_clock::time_point;

  player_map players;

  coro<void> reportPlayerRemoval(uint64_t id);
  coro<void> reportPlayerRegistration(uint64_t id);

  friend class BackgroundTasks;
};

class JoinedLobby : public ActiveLobby
{
public:
  static JoinedLobby* get();

  JoinedLobby(const endpoint& server_endpoint, const Lobby& lobby,
              const std::optional<socket_handle_type>& socket_handle)
      : server(server_endpoint), ActiveLobby(lobby, socket_handle)
  {
  }

  bool isHost() override { return false; }

  size_t playerCount() override { return players.size(); }

  void erasePlayer(uint64_t id) override;

  void registerPlayer(const GDMXPlayer& player,
                      const endpoint&   sender) override;

private:
  using player_map = boost::unordered_flat_map<uint64_t, GDMXPlayer>;

  player_map players;
  endpoint   server;
};

struct HostedLobby::PlayerItem
{
  GDMXPlayer                     player{};
  boost::asio::ip::udp::endpoint source;
  time_point                     last_update = std::chrono::steady_clock::now();

  template <typename archive>
  void serialize(archive& arch, const unsigned int version)
  {
    arch & player;
  }
};

BOOST_CLASS_IMPLEMENTATION(HostedLobby::PlayerItem,
                           boost::serialization::object_serializable);
