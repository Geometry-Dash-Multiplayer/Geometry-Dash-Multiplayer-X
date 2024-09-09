#include <Geode/binding/GJBaseGameLayer.hpp>
#include <Geode/binding/PlayerObject.hpp>
#include <Geode/utils/cocos.hpp>

class GDMXPlayer
{
public:
  GDMXPlayer() = delete;

  GDMXPlayer(GJBaseGameLayer* game_layer, size_t gdmx_user_id);

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
