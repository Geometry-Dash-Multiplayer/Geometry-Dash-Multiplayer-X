#pragma once
#include <boost/asio.hpp>
#include <Geode/ui/Popup.hpp>
#include <Geode/ui/ScrollLayer.hpp>
#include <Geode/cocos/include/cocos2d.h>

class LobbiesPopup : public geode::Popup<>
{
public:
  static LobbiesPopup* create();

  void show() override;

  void update(float delta) override;

  boost::asio::awaitable<void> refresh(cocos2d::CCPoint circle_pos);

protected:
  boost::asio::io_context ctx;
  cocos2d::CCLayerColor*  background   = nullptr;
  geode::ScrollLayer*     lobbies_list = nullptr;

  bool setup() override;
  void onCreateLobby(cocos2d::CCObject*);
  void onJoinLobby(cocos2d::CCObject*);
};
