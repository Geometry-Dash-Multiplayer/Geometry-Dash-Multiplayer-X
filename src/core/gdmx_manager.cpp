#include "gdmx_manager.hpp"
#include <gmlc/netif/NetIF.hpp>
namespace asio = boost::asio;
using asio::ip::udp;

GDMXManager& GDMXManager::get()
{
  static GDMXManager instance{};
  return instance;
}

void GDMXManager::createLobby(LobbyType type, std::string_view name)
{
  assert(!active_lobby);
  active_lobby = std::make_unique<HostedLobby>(type, name);
}

void GDMXManager::joinLobby(
    const udp::endpoint& server_endpoint, const Lobby& lobby,
    const std::optional<socket_handle_type>& socket_handle)
{
  assert(!active_lobby);
  active_lobby =
      std::make_unique<JoinedLobby>(server_endpoint, lobby, socket_handle);
}

void GDMXManager::unjoinLobby() { active_lobby = nullptr; }

static uint64_t get_local_id_impl()
{
  auto addresses = gmlc::netif::getInterfaceAddressesV4();
  assert(addresses.size());
  return boost::asio::ip::address_v4::from_string(addresses[0]).to_uint();
}

uint64_t GDMXManager::getLocalID() const
{
  static uint64_t id = get_local_id_impl();
  return id;
}
