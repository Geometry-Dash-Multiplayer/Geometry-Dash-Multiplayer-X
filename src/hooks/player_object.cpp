#include "game_layer.hpp"
#include <Geode/utils/cocos.hpp>
#include <Geode/modify/PlayerObject.hpp>
#include <Geode/cocos/include/cocos2d.h>

class $modify(PlayerObject)
{
  $override void update(float delta)
  {
    PlayerObject::update(delta);

    if (!GJBaseGameLayer::get() || this != m_gameLayer->m_player1)
      return;

    for (auto& [id, player] : GDMXGameLayer::get()->m_fields->players)
      player.update(delta);
  }

  $override void updateRotation(float delta)
  {
    PlayerObject::updateRotation(delta);

    if (!GJBaseGameLayer::get() || this != m_gameLayer->m_player1)
      return;

    for (auto& [id, player] : GDMXGameLayer::get()->m_fields->players)
      player.updateRotation(delta);
  }
};