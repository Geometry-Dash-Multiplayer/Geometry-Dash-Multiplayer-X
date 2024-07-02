#include <string>
#include <Geode/utils/cocos.hpp>

struct LobbyInfo
{
  size_t                        id = 0;
  std::string                   name;
  size_t                        hostID = 0;
  geode::Ref<cocos2d::CCSprite> icon;
};
