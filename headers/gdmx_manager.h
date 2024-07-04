#pragma once
#include "lobby_info.h"

class GDMXManager
{
public:
  static GDMXManager& get();

  LobbyInfo* joinedLobby() { return joinedLobbyInfo.get(); }

  void join(const std::shared_ptr<LobbyInfo>& lobby);
  void unjoin();

private:
  std::shared_ptr<LobbyInfo> joinedLobbyInfo;

  GDMXManager() {}

  GDMXManager(const GDMXManager&)            = delete;
  GDMXManager& operator=(const GDMXManager&) = delete;
};
