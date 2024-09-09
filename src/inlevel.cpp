#include "gdmx_manager.h"
#include <Geode/Geode.hpp>
#include <Geode/modify/PlayLayer.hpp>
#include <Geode/modify/GJBaseGameLayer.hpp>
#include <Geode/modify/PlayerObject.hpp>
#include <Geode/modify/GJEffectManager.hpp>
using namespace geode::prelude;

class $modify(GDMXGameLayer, GJBaseGameLayer)
{
  struct Fields
  {
    std::vector<GDMXPlayer> players;
  };

  $override void createPlayer()
  {
    GJBaseGameLayer::createPlayer();
    if (!GDMXManager::get().joinedLobby() || !PlayLayer::get())
      return;
    m_fields->players.emplace_back(this, 0).registerPlayer();
    schedule(schedule_selector(GDMXGameLayer::doJump), 1.5f);
    schedule(schedule_selector(GDMXGameLayer::releaseJump), 1.5f,
             kCCRepeatForever, 1.0f);
  }

  $override int checkCollisions(PlayerObject* player, float delta, bool flag)
  {
    int result = GJBaseGameLayer::checkCollisions(player, delta, flag);
    if (player != m_player1)
      return result;
    for (auto& player : m_fields->players)
      GJBaseGameLayer::checkCollisions(player.getObject(), delta, true);
    return result;
  }

  $override void resetPlayer()
  {
    GJBaseGameLayer::resetPlayer();
    for (auto& player : m_fields->players)
      player.reset();
  }

  void doJump(float delta)
  {
    for (auto& player : m_fields->players)
      player.getObject()->pushButton(PlayerButton::Jump);
  }

  void releaseJump(float delta)
  {
    for (auto& player : m_fields->players)
      player.getObject()->releaseButton(PlayerButton::Jump);
  }

  static GDMXGameLayer* get()
  {
    return static_cast<GDMXGameLayer*>(GJBaseGameLayer::get());
  }
};

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

    for (auto& player : GDMXGameLayer::get()->m_fields->players)
      player.updateVisibility(delta);
  }

  $override void checkForEnd()
  {
    PlayLayer::checkForEnd();

    if (!m_endPortal)
      return;

    for (auto& player : GDMXGameLayer::get()->m_fields->players)
      player.checkForEnd();
  }
};

class $modify(PlayerObject)
{
  $override void update(float delta)
  {
    PlayerObject::update(delta);

    if (!GJBaseGameLayer::get() || this != m_gameLayer->m_player1)
      return;

    for (auto& player : GDMXGameLayer::get()->m_fields->players)
      player.update(delta);
  }

  $override void updateRotation(float delta)
  {
    PlayerObject::updateRotation(delta);
    if (!GJBaseGameLayer::get() || this != m_gameLayer->m_player1)
      return;
    for (auto& player : GDMXGameLayer::get()->m_fields->players)
      player.updateRotation(delta);
  }
};

class $modify(GJEffectManager)
{
  $override void updateEffects(float delta)
  {
    GJEffectManager::updateEffects(delta);

    if (auto glayer = GDMXGameLayer::get())
    {
      for (auto& player : glayer->m_fields->players)
        player.updateEffects(delta);
    }
  }
};
