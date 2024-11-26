#include "gdmx_manager.hpp"
#include "game_layer.hpp"
#include <utils/logging.hpp>
#include <Geode/Geode.hpp>
#include <Geode/modify/PlayLayer.hpp>
using namespace cocos2d;
using namespace std::chrono_literals;

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

void GDMXGameLayer::update(float delta)
{
  GJBaseGameLayer::update(delta);

  if (auto* lobby = ActiveLobby::get())
  {
    using std::chrono::steady_clock;
    if (m_fields->last_synced == steady_clock::time_point::min())
      m_fields->last_synced = steady_clock::now();
    else if ((m_fields->last_synced - steady_clock::now()) > 5s)
    {
      lobby->syncAcross(SyncData::from(m_player1));
      m_fields->last_synced = steady_clock::now();
    }
  }
}

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
    output::debug("player with id {} just entered level with id {}", player.id,
                  m_level->m_levelID.value());
    break;
  }
  case EventType::PlayerExitedLevel:
  {
    auto& [id, level_id] = std::get<PlayerExitedLevelValue>(value);
    if (level_id && level_id != m_level->m_levelID)
      return;
    m_fields->players.erase(id);
    output::debug("player with id {} just exited level with id {}", id,
                  m_level->m_levelID.value());
    break;
  }
  case EventType::PlayerSync:
  {
    auto& [id, info] = std::get<PlayerSyncValue>(value);
    auto item        = m_fields->players.find(id);
    if (item == m_fields->players.end())
      break;
    item->second.sync(info);
    output::debug("player with id {} synced", id);
    break;
  }
  default:
    output::warn("Unhandled Event - {} ({})", type, fmt::underlying(type));
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
