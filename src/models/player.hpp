#pragma once
#include <Geode/binding/GJBaseGameLayer.hpp>
#include <Geode/binding/PlayerObject.hpp>
#include <Geode/utils/cocos.hpp>
#include <boost/serialization/utility.hpp>
#include <boost/serialization/optional.hpp>
#include "sync.hpp"

class IconIDs
{
public:
  uint32_t cube = 0, ship = 0, ball = 0, ufo = 0, wave = 0, robot = 0,
           spider = 0, swing = 0;

  static IconIDs self();

private:
  void serialize(auto& arch, const unsigned int version)
  {
    arch & cube & ship & ball & ufo & wave & robot & spider & swing;
  }

  friend class boost::serialization::access;
};

using ColorIDs = std::pair<uint32_t, uint32_t>;
using GlowID   = std::optional<uint32_t>;

class GDMXPlayer
{
public:
  uint64_t id = 0;
  IconIDs  icons{};
  ColorIDs colors{};
  GlowID   glow = std::nullopt;

  static GDMXPlayer self(bool local);

private:
  void serialize(auto& arch, const unsigned int version)
  {
    arch & id & icons & colors & glow;
  }

  friend class boost::serialization::access;
};

class GDMXPlayerObject
{
public:
  GDMXPlayerObject() = default;

  GDMXPlayerObject(const GDMXPlayer& info);

  GDMXPlayerObject(GDMXPlayerObject&& other) noexcept
      : player(std::move(other.player)),
        finished(std::exchange(other.finished, false))
  {
  }

  GDMXPlayerObject& operator=(GDMXPlayerObject&& other) noexcept
  {
    if (this == &other)
      return *this;
    player   = std::move(other.player);
    finished = std::exchange(other.finished, false);
    return *this;
  }

  ~GDMXPlayerObject();

  PlayerObject* getObject() { return player; }

  void update(float delta);
  void updateRotation(float delta);
  void updateEffects(float delta);
  void updateVisibility(float delta);

  void checkForEnd();

  void registerPlayer();
  void reset();
  void setupPlayerStart();

  void sync(const SyncData& info);

  SyncData createSyncPoint() { return SyncData::from(player); }

  void switchToFlyMode(GameObjectType type);
  void switchToRollMode();
  void switchToRobotMode();
  void switchToSpiderMode();

private:
  geode::Ref<PlayerObject> player;
  bool                     finished = false;
};
