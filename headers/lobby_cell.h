#include <Geode/cocos/include/cocos2d.h>

class LobbyCell : public cocos2d::CCLayer
{
public:
  LobbyCell(const std::string& name) : lobbyName(name) {}

  static LobbyCell* create(const std::string& name, float width, float height,
                           bool colored = false);
  bool              init(float width, float height, bool colored = false);

protected:
  std::string lobbyName;

  void onInfo(cocos2d::CCObject*);
  void onLock(cocos2d::CCObject*);
  void onJoin(cocos2d::CCObject*);
};
