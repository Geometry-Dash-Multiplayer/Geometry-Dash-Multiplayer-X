#pragma once
#include <boost/asio.hpp>
#include <Geode/cocos/include/cocos2d.h>
#include <memory>
#include <models/lobby.hpp>

class LobbyCell : public cocos2d::CCNode
{
public:
  template <typename T>
  using coro = boost::asio::awaitable<T>;

  template <std::convertible_to<Lobby> T>
  LobbyCell(T&& lobby, bool colored)
      : lobby(std::forward<T>(lobby)), colored(colored)
  {
  }

  ~LobbyCell();

  template <std::convertible_to<Lobby> T>
  static LobbyCell* create(T&& lobby, cocos2d::CCSize size,
                           bool colored = false)
  {
    auto obj = new LobbyCell(std::forward<T>(lobby), colored);
    if (obj->init(size, colored))
    {
      obj->autorelease();
      return obj;
    }

    delete obj;
    return nullptr;
  }

  // for warnings
  using cocos2d::CCNode::init;

  bool init(cocos2d::CCSize size, bool colored = false);

  void update(float delta) override;

  void markJoined(bool is_host);
  void markUnjoined();

  const Lobby& data() { return lobby; }

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
  coro<void> erase();

  void onInfo(cocos2d::CCObject*);
  void onLock(cocos2d::CCObject*);
  void onJoin(cocos2d::CCObject*);

private:
  coro<void> joinUnchecked();
};
