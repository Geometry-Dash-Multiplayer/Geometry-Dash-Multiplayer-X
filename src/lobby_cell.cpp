#include "lobby_cell.h"
#include <Geode/Geode.hpp>
using namespace geode::prelude;

LobbyCell* LobbyCell::create(const std::string& name, float width, float height,
                             bool colored)
{
  auto obj = new LobbyCell(name);
  if (obj->init(width, height, colored))
  {
    obj->autorelease();
    return obj;
  }

  delete obj;
  return nullptr;
}

bool LobbyCell::init(float width, float height, bool colored)
{
  setContentSize({ width, height });
  setAnchorPoint({});

  if (colored)
  {
    auto clayer = CCLayerColor::create({ 0, 0, 0, 75 }, width, height);
    clayer->setAnchorPoint({});
    addChild(clayer);
  }

  auto cell_items = CCNode::create();
  cell_items->setAnchorPoint({ 0, 0.5f });
  cell_items->setPosition({ width / 32, height / 2 });
  cell_items->setContentWidth(width / 2);
  cell_items->setLayout(
      RowLayout::create()->setAxisAlignment(AxisAlignment::Start)->setGap(5));
  cell_items->setID("cell-items");
  addChild(cell_items);

  auto icon = CCSprite::create("earth.png"_spr);
  icon->setLayoutOptions(
      AxisLayoutOptions::create()->setScaleLimits(0.1f, 0.1f));
  cell_items->addChild(icon);

  auto lobby_text = CCLabelBMFont::create(lobbyName.c_str(), "bigFont.fnt");
  lobby_text->setScale(0.5f);
  lobby_text->setLayoutOptions(
      AxisLayoutOptions::create()->setAutoScale(true)->setScaleLimits(0.35f,
                                                                      0.5f));
  cell_items->addChild(lobby_text);

  int         total_user_count = rand(), user_count = rand();
  std::string counter_text =
      fmt::format("{}/{}", total_user_count % user_count, total_user_count);
  auto user_count_label =
      CCLabelBMFont::create(counter_text.c_str(), "bigFont.fnt");
  user_count_label->setScale(0.2f);
  user_count_label->setPosition(user_count_label->getScaledContentSize() / 2);
  auto label2_container = CCNode::create();
  label2_container->setContentSize(
      { user_count_label->getScaledContentWidth(),
        lobby_text->getScaledContentHeight() / 2 });
  label2_container->addChild(user_count_label);
  cell_items->addChild(label2_container);

  auto info_button = CCMenuItemSpriteExtra::create(
      CCSprite::createWithSpriteFrameName("GJ_infoIcon_001.png"), this,
      menu_selector(LobbyCell::onInfo));
  info_button->setScale(info_button->m_baseScale = 0.6f);
  info_button->setPosition(info_button->getScaledContentSize() / 2);
  auto info_menu = CCMenu::create();
  info_menu->setContentSize({ info_button->getScaledContentWidth(),
                              lobby_text->getScaledContentHeight() });
  info_menu->setID("info-menu");
  info_menu->addChild(info_button);
  cell_items->addChild(info_menu);

  auto lock_button = CCMenuItemSpriteExtra::create(
      CCSprite::createWithSpriteFrameName("GJ_lock_open_001.png"), this,
      menu_selector(LobbyCell::onLock));
  lock_button->setContentHeight(0.8f * lock_button->getContentHeight());
  lock_button->setScale(lock_button->m_baseScale = 0.5f);
  lock_button->setPosition(lock_button->getScaledContentSize() / 2);
  auto lock_menu = CCMenu::create();
  lock_menu->setContentSize({ lock_button->getScaledContentWidth(),
                              lobby_text->getScaledContentHeight() });
  lock_menu->setID("lock-menu");
  lock_menu->addChild(lock_button);
  cell_items->addChild(lock_menu);

  auto join_button = CCMenuItemSpriteExtra::create(
      ButtonSprite::create("Join"), this, menu_selector(LobbyCell::onJoin));
  join_button->setScale(join_button->m_baseScale = 0.6f);
  join_button->setPosition(
      { width - (width / 32) - (join_button->getScaledContentWidth() / 2),
        height / 2 });
  auto join_menu = CCMenu::create();
  join_menu->ignoreAnchorPointForPosition(false);
  join_menu->setID("join-menu");
  join_menu->addChild(join_button);
  addChild(join_menu);

  cell_items->updateLayout();
  return true;
}

void LobbyCell::onInfo(CCObject*)
{
  FLAlertLayer::create("Lobby Info", "Shows Info", "OK")->show();
}

void LobbyCell::onLock(CCObject*)
{
  FLAlertLayer::create("Access", "Lobby Accessible", "OK")->show();
}

void LobbyCell::onJoin(CCObject* button)
{
  FLAlertLayer::create("Join", "Joining Lobby", "OK")->show();
}
