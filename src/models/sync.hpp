#pragma once
#include <Geode/cocos/include/cocos2d.h>
#include <Geode/binding/PlayerObject.hpp>
#include <array>
#include <vector>
#include <boost/serialization/array.hpp>
#include <boost/serialization/vector.hpp>
#include <serialization/cocos2d.hpp>

class SyncFlags
{
public:
  bool isHolding          : 1;
  bool isShip             : 1;
  bool isBall             : 1;
  bool isBird             : 1;
  bool isSwing            : 1;
  bool isDart             : 1;
  bool isRobot            : 1;
  bool isSpider           : 1;
  bool isUpsideDown       : 1;
  bool isSideWays         : 1;
  bool isOnGround         : 1;
  bool isMini             : 1;
  bool isHidden           : 1;
  bool isGoingLeft        : 1;
  bool isDashing          : 1;
  bool decreaseBoostSlide : 1;

  static SyncFlags from(PlayerObject* player);

private:
  // clang-format off
  void serialize(auto& arch, const unsigned int version)
  {
    using array_type = std::array<unsigned char, sizeof(SyncFlags)>;
    arch & reinterpret_cast<array_type&>(*this);
  }
  // clang-format on

  friend class boost::serialization::access;
};

class SyncData
{
public:
  SyncFlags          flags;
  cocos2d::CCPoint   position;
  cocos2d::CCPoint   lastPosition;
  GhostType          ghostType;
  float              speed;
  double             yVelocity;
  double             lastFlipTime;
  int                followRelated;
  std::vector<float> playerFollowFloats;

  static SyncData from(PlayerObject* player);

private:
  void serialize(auto& arch, const unsigned int version)
  {
    arch & flags & position & lastPosition & ghostType & speed & yVelocity &
        lastFlipTime & followRelated & playerFollowFloats;
  }

  friend class boost::serialization::access;
};
