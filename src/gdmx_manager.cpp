#include "gdmx_manager.h"

GDMXManager& GDMXManager::get()
{
  static GDMXManager instance{};
  return instance;
}

void GDMXManager::join(const std::shared_ptr<LobbyInfo>& lobby)
{
  joinedLobbyInfo = lobby;
  // do other stuff, such as fetching joined players info
}

void GDMXManager::unjoin() { joinedLobbyInfo = nullptr; }
