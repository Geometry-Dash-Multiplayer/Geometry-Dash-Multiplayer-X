#pragma once
#include <boost/asio.hpp>
#include <Geode/cocos/include/cocos2d.h>

struct Tasks : cocos2d::CCNode
{
  boost::asio::io_context ctx;

  static Tasks* create()
  {
    auto obj = new Tasks();
    if (!obj->init())
    {
      delete obj;
      return nullptr;
    }

    obj->autorelease();
    obj->retain();
    obj->scheduleUpdate();
    return obj;
  }

  void update(float delta)
  {
    ctx.poll();
    if (!ctx.stopped())
      return;
    unscheduleUpdate();
    release();
  }
};

template <typename T>
Tasks* cospawnInMainThread(T&& coro)
{
  auto* tasks = Tasks::create();
  boost::asio::co_spawn(tasks->ctx, std::forward<T>(coro),
                        boost::asio::detached);
  return tasks;
}