#include "player.hpp"
#include <Geode/Geode.hpp>
using namespace geode::prelude;

GDMXPlayer::GDMXPlayer(GJBaseGameLayer* game_layer, size_t gdmx_user_id)
    : gdmx_user_id(gdmx_user_id)
{
  const auto manager = GameManager::sharedState();
  player =
      PlayerObject::create(manager->getPlayerFrame(), manager->getPlayerShip(),
                           game_layer, game_layer->m_objectLayer, true);
  player->setColor(manager->colorForIdx(manager->getPlayerColor2()));
  player->setSecondColor(manager->colorForIdx(manager->getPlayerColor()));
  player->enableCustomGlowColor(
      manager->colorForIdx(manager->getPlayerGlowColor()));
  player->updateGlowColor();
  player->m_ignoreDamage = true;
  player->addAllParticles();
  player->setID(fmt::format("gdmx-player-{}", gdmx_user_id));
  player->setVisible(true);
}

void GDMXPlayer::update(float delta)
{
  player->update(delta);
  player->m_position = player->getPosition();
}

void GDMXPlayer::updateRotation(float delta)
{
  player->updateRotation(delta);
  player->m_shipRotation = player->getPosition();
}

// PlayerObject::updateEffects is inlined on windows
void GDMXPlayer::updateEffects(float delta)
{
  player->m_waveTrail->updateStroke(delta);
}

void GDMXPlayer::updateVisibility(float delta)
{
  // responsible for wave trail pulsing (see PlayerObject::update)
  player->m_waveTrailPulseRelated =
      player->m_gameLayer->m_player1->m_waveTrailPulseRelated;
}

void GDMXPlayer::checkForEnd()
{
  auto* const endportal = player->m_gameLayer->m_endPortal;
  auto [endposx, _]     = endportal->getSpawnPos();

  if (finished || endportal->m_maybeIsReversed
          ? endposx <= player->getPositionX()
          : endposx >= player->getPositionX())
    return;

  finished = true;

  player->lockPlayer();
  player->toggleGhostEffect(GhostType::Enabled);

  const auto playerpos = player->getPosition();
  auto       startpos  = endportal->getStartPos();

  // in the future this config will be retrieved from the server and not
  // calculated here
  const ccBezierConfig config = {
    .endPosition    = {startpos.x +
                            (endportal->m_maybeIsReversed ? -50.0f : 50.0f),
                       startpos.y - 20.0f },
    .controlPoint_1 = playerpos,
    .controlPoint_2 = { playerpos.x +
                            (endportal->m_maybeIsReversed ? -40.0f : 40.0f),
                       startpos.y + 150.0f}
  };

  player->runAction(CCEaseIn::create(CCBezierTo::create(1.0f, config), 1.2f));
  player->runAction(CCEaseIn::create(CCRotateBy::create(1.0f, 360.0f), 1.5f));
}

void GDMXPlayer::registerPlayer()
{
  player->m_gameLayer->m_objectLayer->addChild(player, 59);
}

void GDMXPlayer::unregisterPlayer()
{
  player->m_gameLayer->m_objectLayer->removeChild(player);
}

void GDMXPlayer::reset()
{
  player->setStartPos({ 0, 105.f });
  player->resetObject();
}
