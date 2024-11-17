#include "player.hpp"
#include "gdmx_manager.hpp"
#include "sync.hpp"
#include <Geode/Geode.hpp>
using namespace cocos2d;

GDMXPlayerObject::GDMXPlayerObject(const GDMXPlayer& info)
{
  auto* const manager    = GameManager::get();
  auto* const game_layer = GJBaseGameLayer::get();
  player = PlayerObject::create(info.icons.cube, info.icons.ship, game_layer,
                                game_layer->m_objectLayer, true);
  player->setColor(manager->colorForIdx(info.colors.first));
  player->setSecondColor(manager->colorForIdx(info.colors.second));
  if (info.glow)
    player->enableCustomGlowColor(manager->colorForIdx(*info.glow));
  player->updateGlowColor();
  player->addAllParticles();
  player->setID(fmt::format("gdmx-player-{}", info.id));
  player->setVisible(true);
  player->setUserObject(geode::ObjWrapper<GDMXPlayer>::create(info));
  player->m_ignoreDamage = true;

  registerPlayer();
  reset();
  setupPlayerStart();
}

GDMXPlayerObject::~GDMXPlayerObject()
{
  if (player && player->getParent())
    player->removeFromParent();
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

  if (finished || endportal->m_flippedX ? endposx <= player->getPositionX()
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
    .endPosition    = {startpos.x + (endportal->m_flippedX ? -50.0f : 50.0f),
                       startpos.y - 20.0f },
    .controlPoint_1 = playerpos,
    .controlPoint_2 = { playerpos.x + (endportal->m_flippedX ? -40.0f : 40.0f),
                       startpos.y + 150.0f}
  };

  player->runAction(CCEaseIn::create(CCBezierTo::create(1.0f, config), 1.2f));
  player->runAction(CCEaseIn::create(CCRotateBy::create(1.0f, 360.0f), 1.5f));
}

void GDMXPlayerObject::registerPlayer()
{
  player->m_gameLayer->m_objectLayer->addChild(player, 59);
}

void GDMXPlayerObject::reset()
{
  player->setStartPos({ 0, 105.f });
  player->resetObject();
}

// see GJBaseGameLayer::setupLevelStart for reference
void GDMXPlayerObject::setupPlayerStart()
{
  auto* const       game_layer = player->m_gameLayer;
  const auto* const settings   = game_layer->m_levelSettings;
  player->flipGravity(settings->m_isFlipped, true);
  player->doReversePlayer(settings->m_reverseGameplay);
  player->rotateGameplayOnly(settings->m_rotateGameplay);
  assert(!settings->m_startDual && "toggle dual mode not implemented");
  player->togglePlayerScale(settings->m_startMini, false);
  assert(!settings->m_platformerMode && !game_layer->m_isPlatformer &&
         "platformer mode not implemented");

  switch (settings->m_startMode)
  {
  case 1: game_layer->switchToFlyMode(player, nullptr, true, 5); break;
  case 2: game_layer->switchToRollMode(player, nullptr, true); break;
  case 3: game_layer->switchToFlyMode(player, nullptr, true, 19); break;
  case 4: game_layer->switchToFlyMode(player, nullptr, true, 26); break;
  case 5: game_layer->switchToRobotMode(player, nullptr, true); break;
  case 6: game_layer->switchToSpiderMode(player, nullptr, true); break;
  case 7: game_layer->switchToFlyMode(player, nullptr, true, 41); break;
  }

  static const std::array speed_map = { 0.9f, 0.7f, 1.1f, 1.3f, 1.6f };
  player->updateTimeMod(speed_map[fmt::underlying(settings->m_startSpeed)],
                        false);
}

// heavily inspired from PlayerObject::loadFromCheckpoint
void GDMXPlayerObject::sync(const SyncData& info)
{
  player->toggleVisibility(!info.flags.isHidden);
  player->setPosition(info.position);
  player->flipGravity(info.flags.isUpsideDown, false);
  player->rotateGameplayOnly(info.flags.isSideWays);
  player->toggleGhostEffect(info.ghostType);
  player->togglePlayerScale(info.flags.isMini, false);
  player->m_lastPosition = info.lastPosition;
  if (info.flags.isGoingLeft)
    player->doReversePlayer(true);
  player->updateTimeMod(info.speed, false);

  if (info.flags.isShip)
    switchToFlyMode(GameObjectType::ShipPortal);
  else if (info.flags.isBall)
    switchToRollMode();
  else if (info.flags.isBird)
    switchToFlyMode(GameObjectType::UfoPortal);
  else if (info.flags.isSwing)
    switchToFlyMode(GameObjectType::SwingPortal);
  else if (info.flags.isDart)
    switchToFlyMode(GameObjectType::WavePortal);
  else if (info.flags.isRobot)
    switchToRobotMode();
  else if (info.flags.isSpider)
    switchToSpiderMode();

  player->resetStreak();
  assert(!info.flags.isDashing && "syncing on dashing is not yet implemented");
  player->setYVelocity(info.yVelocity, 50);
  player->m_isOnGround         = info.flags.isOnGround;
  player->m_decreaseBoostSlide = info.flags.decreaseBoostSlide;
  player->m_lastFlipTime       = info.lastFlipTime;

  if (info.flags.isHolding)
    player->pushButton(PlayerButton::Jump);
}

void GDMXPlayerObject::switchToFlyMode(GameObjectType type)
{
  player->switchedToMode(type);
  switch (type)
  {
  case GameObjectType::ShipPortal: player->toggleFlyMode(true, true); break;
  case GameObjectType::UfoPortal: player->toggleBirdMode(true, true); break;
  case GameObjectType::WavePortal: player->toggleDartMode(true, true); break;
  case GameObjectType::SwingPortal: player->toggleSwingMode(true, true); break;
  default: assert(!"unknown fly mode passed");
  }
}

void GDMXPlayerObject::switchToRollMode()
{
  player->switchedToMode(GameObjectType::BallPortal);
  player->toggleRollMode(true, true);
}

void GDMXPlayerObject::switchToRobotMode()
{
  player->switchedToMode(GameObjectType::RobotPortal);
  player->toggleRobotMode(true, true);
}

void GDMXPlayerObject::switchToSpiderMode()
{
  player->switchedToMode(GameObjectType::SpiderPortal);
  player->toggleSpiderMode(true, true);
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
  GDMXPlayer  instance{
     .id     = GDMXManager::get().getID(local),
     .icons  = IconIDs::self(),
     .colors = {manager->getPlayerColor(), manager->getPlayerColor2()}
  };

  if (manager->getPlayerGlow())
    instance.glow = manager->getPlayerGlowColor();
  return instance;
}
