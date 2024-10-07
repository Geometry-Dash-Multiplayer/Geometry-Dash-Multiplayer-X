#pragma once
#include <Geode/binding/GJBaseGameLayer.hpp>
#include <Geode/binding/PlayerObject.hpp>
#include <Geode/utils/cocos.hpp>
#include <boost/serialization/utility.hpp>
#include <boost/serialization/optional.hpp>

struct IconIDs
{
  uint32_t cube = 0, ship = 0, ball = 0, ufo = 0, wave = 0, robot = 0,
           spider = 0, swing = 0;

  static IconIDs self();

  template <typename archive>
  void serialize(archive& arch, const unsigned int version)
  {
    arch & cube & ship & ball & ufo & wave & robot & spider & swing;
  }
};

using ColorIDs = std::pair<uint32_t, uint32_t>;
using GlowID   = std::optional<uint32_t>;

struct GDMXPlayer
{
  uint64_t id = 0;
  IconIDs  icons{};
  ColorIDs colors{};
  GlowID   glow = std::nullopt;

  static GDMXPlayer self(bool local);

  template <typename archive>
  void serialize(archive& arch, const unsigned int version)
  {
    arch & id & icons & colors & glow;
  }
};

class GDMXPlayerObject
{
public:
  GDMXPlayerObject() = delete;

  GDMXPlayerObject(GJBaseGameLayer* game_layer, size_t gdmx_user_id);

  PlayerObject* getObject() { return player; }

  void update(float delta);
  void updateRotation(float delta);
  void updateEffects(float delta);
  void updateVisibility(float delta);

  void checkForEnd();

  void registerPlayer();
  void unregisterPlayer();
  void reset();

private:
  geode::Ref<PlayerObject> player;
  size_t                   gdmx_user_id = 0;
  bool                     finished     = false;
};
