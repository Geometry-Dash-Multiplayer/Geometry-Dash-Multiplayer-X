#pragma once
#include <net/constants.hpp>
#include <models/player.hpp>
#include <Geode/modify/GJBaseGameLayer.hpp>
#include <boost/unordered/unordered_flat_map.hpp>

class $modify(GDMXGameLayer, GJBaseGameLayer)
{
  struct Fields
  {
    using time_point = std::chrono::steady_clock::time_point;
    boost::unordered_flat_map<uint64_t, GDMXPlayerObject> players;
    time_point last_synced = time_point::min();
  };

  $override int  checkCollisions(PlayerObject* player, float delta, bool flag);
  $override void resetPlayer();
  $override void update(float delta);

  void dispatch(EventType type, const EventValue& value);

  static GDMXGameLayer* get()
  {
    return static_cast<GDMXGameLayer*>(GJBaseGameLayer::get());
  }
};
