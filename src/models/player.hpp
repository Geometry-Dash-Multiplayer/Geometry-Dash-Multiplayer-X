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

private:
  geode::Ref<PlayerObject> player;
  bool                     finished = false;
};
