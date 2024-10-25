#include "gdmx_manager.hpp"
namespace asio = boost::asio;
using asio::ip::udp;

GDMXManager& GDMXManager::get()
{
  static GDMXManager instance{};
  return instance;
}

void GDMXManager::createLobby(std::string_view name, LobbyType type)
{
  assert(!active_lobby);
  active_lobby = std::make_unique<HostedLobby>(name, type);
}

void GDMXManager::joinLobby(
    const Lobby& lobby, const udp::endpoint& server,
    const std::optional<socket_handle_type>& socket_handle)
{
  assert(!active_lobby);
  active_lobby = std::make_unique<JoinedLobby>(lobby, server, socket_handle);
}

void GDMXManager::unjoinLobby() { active_lobby = nullptr; }

static uint64_t getLocalIDImpl()
{
  asio::io_context ctx;
  udp::socket      socket{ ctx, udp::v4() };
  socket.connect(udp::endpoint(asio::ip::make_address_v4("8.8.8.8"), 9));
  return socket.local_endpoint().address().to_v4().to_uint();
}

uint64_t GDMXManager::getLocalID() const
{
  static uint64_t id = getLocalIDImpl();
  return id;
}
