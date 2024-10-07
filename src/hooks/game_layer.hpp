#pragma once
#include "player.hpp"
#include <Geode/modify/GJBaseGameLayer.hpp>
#include <boost/unordered/unordered_flat_map.hpp>

class $modify(GDMXGameLayer, GJBaseGameLayer)
{
  struct Fields
  {
    boost::unordered_flat_map<uint64_t, GDMXPlayerObject> players;
  };

  $override int  checkCollisions(PlayerObject* player, float delta, bool flag);
  $override void resetPlayer();

  static GDMXGameLayer* get()
  {
    return static_cast<GDMXGameLayer*>(GJBaseGameLayer::get());
  }
};
