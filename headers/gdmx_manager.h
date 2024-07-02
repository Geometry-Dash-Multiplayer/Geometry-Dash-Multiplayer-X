#include <shared_mutex>
#include <memory>
#include "lobby_info.h"

class GDMXManager
{
public:
  static GDMXManager& get();

  void join(size_t id);
  void unjoin(size_t id);

private:
  mutable std::shared_mutex managerLock;
  std::shared_ptr<LobbyInfo> joinedLobby;

  GDMXManager() {}

  GDMXManager(const GDMXManager&)            = delete;
  GDMXManager& operator=(const GDMXManager&) = delete;
};
