#include "macros.h"
#include <Geode/Geode.hpp>
#include <Geode/modify/LevelInfoLayer.hpp>
using namespace geode::prelude;

class $modify(GDMXSettings, LevelInfoLayer)
{
  bool init(GJGameLevel* level, bool challenge)
  {
    if (!LevelInfoLayer::init(level, challenge))
      return false;
    auto sprite = CircleButtonSprite::createWithSprite("logo.png"_spr);
    auto button = CCMenuItemSpriteExtra::create(
        sprite, this, menu_selector(GDMXSettings::onGDMXSettings));
    auto* menu = m_likeBtn->getParent();
    assert(dynamic_cast<cocos2d::CCMenu*>(menu));
    menu->addChild(button);
    button->setID("gdmx-settings");
    button->setPosition(m_likeBtn->getPosition() + CCPoint{ 0, -50 });
    return true;
  }

  void onGDMXSettings(CCObject*)
  {
    FLAlertLayer::create("Angel", "Hello World!", "OK")->show();
  }
};
