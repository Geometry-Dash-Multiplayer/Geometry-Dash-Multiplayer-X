#include "gdmx_manager.hpp"
#include "game_layer.hpp"
#include <Geode/Geode.hpp>
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

void GDMXGameLayer::dispatch(EventType type, const EventValue& value)
{
  switch (type)
  {
  case EventType::PlayerEnteredLevel:
  {
    auto& [player, level_id] = std::get<PlayerEnteredLevelValue>(value);
    if (level_id && level_id != m_level->m_levelID)
      return;
    m_fields->players[player.id] = player;
    break;
  }
  case EventType::PlayerExitedLevel:
  {
    auto& [id, level_id] = std::get<PlayerExitedLevelValue>(value);
    if (level_id && level_id != m_level->m_levelID)
      return;
    m_fields->players.erase(id);
    break;
  }
  default:
    geode::log::warn("GDMXGameLayer::dispatch: Unhandled Event - {}",
                     to_string(type));
  }
}

class $modify(PlayLayer)
{
  $override bool init(GJGameLevel* level, bool useReplay,
                      bool dontCreateObjects)
  {
    if (!PlayLayer::init(level, useReplay, dontCreateObjects))
      return false;
    if (auto* lobby = ActiveLobby::get())
      lobby->enteredLevel(level->m_levelID);
    return true;
  }

  $override void onQuit()
  {
    PlayLayer::onQuit();
    if (auto* lobby = ActiveLobby::get())
      lobby->exitedLevel(m_level->m_levelID);
  }

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
