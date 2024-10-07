#pragma once
#include <boost/asio.hpp>
#include <Geode/cocos/include/cocos2d.h>
#include <memory>
#include "lobby.hpp"

class LobbyCell : public cocos2d::CCNode
{
public:
  template <typename T>
  using coro = boost::asio::awaitable<T>;

  LobbyCell(Lobby&& lobby, bool colored)
      : lobby(std::move(lobby)), colored(colored)
  {
  }

  LobbyCell(const Lobby& lobby, bool colored) : lobby(lobby), colored(colored)
  {
  }

  ~LobbyCell();

  static LobbyCell* create(Lobby&& lobby, float width, float height,
                           bool colored = false);
  static LobbyCell* create(const Lobby& lobby, float width, float height,
                           bool colored = false);
  bool              init(float width, float height, bool colored = false);

  void update(float delta) override;

  void markJoined(bool is_host);
  void markUnjoined();

protected:
  boost::asio::io_context ctx;
  Lobby                   lobby;
  cocos2d::CCNode*        cell_main_layer = nullptr;
  CCMenuItemSpriteExtra*  join_button     = nullptr;
  cocos2d::CCLayerColor*  colored_layer   = nullptr;
  LoadingCircle*          circle          = nullptr;
  bool                    colored         = false;

  coro<void> join();
  coro<void> unjoin();

  void onInfo(cocos2d::CCObject*);
  void onLock(cocos2d::CCObject*);
  void onJoin(cocos2d::CCObject*);

private:
  coro<void> joinUnchecked();
};
