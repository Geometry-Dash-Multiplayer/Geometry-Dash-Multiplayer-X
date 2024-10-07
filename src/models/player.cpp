#include "player.hpp"
#include "gdmx_manager.hpp"
#include <Geode/Geode.hpp>
using namespace geode::prelude;

GDMXPlayerObject::GDMXPlayerObject(GJBaseGameLayer* game_layer,
                                   size_t           gdmx_user_id)
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

void GDMXPlayerObject::update(float delta)
{
  player->update(delta);
  player->m_position = player->getPosition();
}

void GDMXPlayerObject::updateRotation(float delta)
{
  player->updateRotation(delta);
  player->m_shipRotation = player->getPosition();
}

// PlayerObject::updateEffects is inlined on windows
void GDMXPlayerObject::updateEffects(float delta)
{
  player->m_waveTrail->updateStroke(delta);
}

void GDMXPlayerObject::updateVisibility(float delta)
{
  // responsible for wave trail pulsing (see PlayerObject::update)
  player->m_waveTrailPulseRelated =
      player->m_gameLayer->m_player1->m_waveTrailPulseRelated;
}

void GDMXPlayerObject::checkForEnd()
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

void GDMXPlayerObject::registerPlayer()
{
  player->m_gameLayer->m_objectLayer->addChild(player, 59);
}

void GDMXPlayerObject::unregisterPlayer()
{
  player->m_gameLayer->m_objectLayer->removeChild(player);
}

void GDMXPlayerObject::reset()
{
  player->setStartPos({ 0, 105.f });
  player->resetObject();
}

IconIDs IconIDs::self()
{
  const auto manager = GameManager::get();
  return { .cube   = (uint32_t)manager->getPlayerFrame(),
           .ship   = (uint32_t)manager->getPlayerShip(),
           .ball   = (uint32_t)manager->getPlayerBall(),
           .ufo    = (uint32_t)manager->getPlayerBird(),
           .wave   = (uint32_t)manager->getPlayerDart(),
           .robot  = (uint32_t)manager->getPlayerRobot(),
           .spider = (uint32_t)manager->getPlayerSpider(),
           .swing  = (uint32_t)manager->getPlayerSwing() };
}

GDMXPlayer GDMXPlayer::self(bool local)
{
  auto* const manager = GameManager::get();
  GDMXPlayer instance{
    .id     = GDMXManager::get().getID(local),
    .icons  = IconIDs::self(),
    .colors = {manager->getPlayerColor(), manager->getPlayerColor2()}
  };

  if (manager->getPlayerGlow())
    instance.glow = manager->getPlayerGlowColor();
  return instance;
}
