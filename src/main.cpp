#include <Geode/Geode.hpp>
#include <Geode/modify/LevelInfoLayer.hpp>
#include "settings_popup.h"
using namespace geode::prelude;

class $modify(GDMXEntry, LevelInfoLayer)
{
  bool init(GJGameLevel* level, bool challenge)
  {
    if (!LevelInfoLayer::init(level, challenge))
      return false;
    auto sprite = CircleButtonSprite::createWithSprite("logo.png"_spr);
    auto button = CCMenuItemSpriteExtra::create(
        sprite, this, menu_selector(GDMXEntry::onGDMXSettings));
    auto* menu = m_likeBtn->getParent();
    assert(dynamic_cast<cocos2d::CCMenu*>(menu));
    menu->addChild(button);
    button->setID("gdmx-settings");
    button->setPosition(findGDMXPosition());
    return true;
  }

  void onGDMXSettings(CCObject*) { LobbiesPopup::create()->show(); }
  CCPoint findGDMXPosition()
  {
    if (m_cloneBtn)
    {
      if (m_cloneBtn->isVisible())
        return m_cloneBtn->getPosition() + CCPoint{ 0, -50 };
      else
        return m_cloneBtn->getPosition();
    }

    auto*     data   = m_likeBtn->getParent()->getChildren()->data;
    CCObject* result = *std::find_if(
        data->arr, data->arr + data->num,
        [data](CCObject* elem)
        {
          return std::all_of(
              data->arr, data->arr + data->num,
              [elem](CCObject* elem2)
              {
                auto pos1 = static_cast<CCNode*>(elem)->getPosition(),
                     pos2 = static_cast<CCNode*>(elem2)->getPosition();
                return pos1.x < pos2.x ||
                       (pos1.x == pos2.x && pos1.y >= pos2.y);
              });
        });
    return static_cast<CCNode*>(result)->getPosition() + CCPoint{ 0, 50 };
  }
};
