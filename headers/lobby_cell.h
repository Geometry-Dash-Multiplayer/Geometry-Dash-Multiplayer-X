#pragma once
#include <Geode/cocos/include/cocos2d.h>
#include <memory>
#include "lobby_info.h"

class LobbyCell : public cocos2d::CCLayer
{
public:
  LobbyCell(const std::shared_ptr<LobbyInfo>& info, bool colored)
      : lobby(info), colored(colored)
  {
  }

  static LobbyCell* create(const std::shared_ptr<LobbyInfo>& lobby, float width,
                           float height, bool colored = false);
  bool              init(float width, float height, bool colored = false);

protected:
  std::shared_ptr<LobbyInfo> lobby;
  CCMenuItemSpriteExtra*     joinButton   = nullptr;
  cocos2d::CCLayerColor*     coloredLayer = nullptr;
  bool                       colored      = false;

  void unjoinFromOther(const LobbyInfo* otherLobby);
  void join();
  void unjoin();

  void onInfo(cocos2d::CCObject*);
  void onLock(cocos2d::CCObject*);
  void onJoin(cocos2d::CCObject*);
};
