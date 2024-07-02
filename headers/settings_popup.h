#include <Geode/ui/Popup.hpp>
#include <Geode/cocos/include/cocos2d.h>
#include <string_view>

class LobbiesPopup : public geode::Popup<>
{
public:
  static LobbiesPopup* create();

  void show() override;

protected:
  bool setup() override;
  void onCreateLobby(cocos2d::CCObject*);
  void onJoinLobby(cocos2d::CCObject*);
};
