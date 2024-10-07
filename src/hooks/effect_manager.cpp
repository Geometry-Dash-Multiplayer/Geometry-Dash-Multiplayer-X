#include "game_layer.hpp"
#include <Geode/modify/GJEffectManager.hpp>

class $modify(GJEffectManager)
{
  $override void updateEffects(float delta)
  {
    GJEffectManager::updateEffects(delta);

    if (auto glayer = GDMXGameLayer::get())
    {
      for (auto& [id, player] : glayer->m_fields->players)
        player.updateEffects(delta);
    }
  }
};
