#include "sync.hpp"

SyncFlags SyncFlags::from(PlayerObject* player)
{
  using player_button_underlying = std::underlying_type_t<PlayerButton>;
  constexpr auto jumpbtn =
      static_cast<player_button_underlying>(PlayerButton::Jump);
  return { .isHolding          = player->m_holdingButtons[jumpbtn],
           .isShip             = player->m_isShip,
           .isBall             = player->m_isBall,
           .isBird             = player->m_isBird,
           .isSwing            = player->m_isSwing,
           .isDart             = player->m_isDart,
           .isRobot            = player->m_isRobot,
           .isSpider           = player->m_isSpider,
           .isUpsideDown       = player->m_isUpsideDown,
           .isSideWays         = player->m_isSideways,
           .isOnGround         = player->m_isOnGround,
           .isMini             = player->m_vehicleSize != 1.f,
           .isHidden           = player->m_isHidden,
           .isGoingLeft        = player->m_isGoingLeft,
           .isDashing          = player->m_isDashing,
           .decreaseBoostSlide = player->m_decreaseBoostSlide };
}

SyncData SyncData::from(PlayerObject* player)
{
  return { .flags              = SyncFlags::from(player),
           .position           = player->m_position,
           .lastPosition       = player->m_lastPosition,
           .ghostType          = player->m_ghostType,
           .speed              = player->m_playerSpeed,
           .yVelocity          = player->m_yVelocity,
           .lastFlipTime       = player->m_lastFlipTime,
           .followRelated      = player->m_followRelated,
           .playerFollowFloats = player->m_playerFollowFloats };
}
