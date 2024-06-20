#include <Geode/Geode.hpp>
#include <Geode/modify/MenuLayer.hpp>
using namespace geode::prelude;

class $modify(EntryPoint, MenuLayer)
{
  bool init()
  {
    if (!MenuLayer::init())
      return false;

    auto myButton =
        CCMenuItemSpriteExtra::create(CCSprite::create("logo.png"_spr), this,
                                      menu_selector(EntryPoint::onMyButton));

    auto menu = this->getChildByID("bottom-menu");
    menu->addChild(myButton);

    myButton->setID("my-button"_spr);

    menu->updateLayout();

    return true;
  }

  void onMyButton(CCObject*)
  {
    FLAlertLayer::create("Angel", "Hello World!", "OK")->show();
  }
};
