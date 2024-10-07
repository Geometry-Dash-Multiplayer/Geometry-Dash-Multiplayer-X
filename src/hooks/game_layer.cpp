#include "gdmx_manager.hpp"
#include "lobby.hpp"
#include "game_layer.hpp"
#include <Geode/modify/PlayLayer.hpp>
using namespace geode::prelude;

int GDMXGameLayer::checkCollisions(PlayerObject* player, float delta, bool flag)
{
  int result = GJBaseGameLayer::checkCollisions(player, delta, flag);
  if (player != m_player1)
    return result;
  for (auto& [id, player] : m_fields->players)
    GJBaseGameLayer::checkCollisions(player.getObject(), delta, true);
  return result;
}

void GDMXGameLayer::resetPlayer() { GJBaseGameLayer::resetPlayer(); }

class $modify(PlayLayer)
{
  $override void destroyPlayer(PlayerObject* player, GameObject* obj)
  {
    if (player != m_player1 && player != m_player2)
      return;
    PlayLayer::destroyPlayer(player, obj);
  }

  $override void updateVisibility(float delta)
  {
    PlayLayer::updateVisibility(delta);

    for (auto& [id, player] : GDMXGameLayer::get()->m_fields->players)
      player.updateVisibility(delta);
  }

  $override void checkForEnd()
  {
    PlayLayer::checkForEnd();

    if (!m_endPortal)
      return;

    for (auto& [id, player] : GDMXGameLayer::get()->m_fields->players)
      player.checkForEnd();
  }
};
