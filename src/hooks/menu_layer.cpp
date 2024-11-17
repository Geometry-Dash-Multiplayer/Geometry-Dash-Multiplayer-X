#include <ui/lobbies_popup.hpp>
#include <Geode/Geode.hpp>
#include <Geode/modify/MenuLayer.hpp>
using namespace cocos2d;

class $modify(GDMXEntry, MenuLayer)
{
  $override bool init()
  {
    if (!MenuLayer::init())
      return false;
    auto scale_factor = CCDirector::get()->getContentScaleFactor();
    auto sprite =
        geode::CircleButtonSprite::create(CCSprite::create("logo.png"_spr));
    sprite->setScale(1.1f);
    if (scale_factor < 4)
      sprite->setTopRelativeScale(scale_factor * 0.3f);
    auto button = CCMenuItemSpriteExtra::create(
        sprite, this, menu_selector(GDMXEntry::onGDMXLobbies));
    button->setID("gdmx-lobbies-button");
    auto menu = getChildByID("bottom-menu");
    assert(dynamic_cast<CCMenu*>(menu));
    menu->addChild(button);
    menu->updateLayout();
    return true;
  }

  void onGDMXLobbies(CCObject*)
  {
    auto* popup = LobbiesPopup::create();
    popup->setID("gdmx-lobbies-popup");
    popup->show();
  }
};
