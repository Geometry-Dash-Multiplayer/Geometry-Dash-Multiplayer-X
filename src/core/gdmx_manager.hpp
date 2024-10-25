#pragma once
#include <memory>
#include <models/lobby.hpp>

class GDMXManager
{
public:
  using socket_handle_type = ActiveLobby::socket_handle_type;

  static GDMXManager& get();

  ActiveLobby* getActiveLobby() { return active_lobby.get(); }

  void createLobby(std::string_view name, LobbyType type);

  void joinLobby(const Lobby&                             lobby,
                 const boost::asio::ip::udp::endpoint&    server,
                 const std::optional<socket_handle_type>& socket_handle);

  void unjoinLobby();

  uint64_t getUserID() const { return user_id; }

  uint64_t getLocalID() const;

  uint64_t getID(bool local) { return local ? getLocalID() : getUserID(); }

private:
  uint64_t                     user_id      = 0;
  std::unique_ptr<ActiveLobby> active_lobby = nullptr;

  GDMXManager() {}

  GDMXManager(const GDMXManager&)            = delete;
  GDMXManager& operator=(const GDMXManager&) = delete;
};
