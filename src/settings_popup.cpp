#include "settings_popup.h"
#include "lobby_cell.h"
#include <Geode/Geode.hpp>
using namespace geode::prelude;

LobbiesPopup* LobbiesPopup::create()
{
  auto obj = new LobbiesPopup();
  if (obj->initAnchored(440, 290))
  {
    obj->autorelease();
    return obj;
  }

  delete obj;
  return nullptr;
}

bool LobbiesPopup::setup()
{
  setTitle("GDMX Lobbies");
  auto [width, height]      = m_mainLayer->getContentSize();
  const auto buttons_height = 0.82f * height;

  auto bg2_layer = CCLayerColor::create();
  bg2_layer->setColor({});
  bg2_layer->setOpacity(75);
  bg2_layer->setAnchorPoint({});
  bg2_layer->setPosition({ width / 16, 13 });
  bg2_layer->setContentSize({ 0.8f * width, (0.8f * height) - 25 });
  m_mainLayer->addChild(bg2_layer);

  auto create_lobby_button = CCMenuItemSpriteExtra::create(
      ButtonSprite::create("Create Lobby"), this,
      menu_selector(LobbiesPopup::onCreateLobby));
  auto join_lobby_button =
      CCMenuItemSpriteExtra::create(ButtonSprite::create("Join Lobby"), this,
                                    menu_selector(LobbiesPopup::onJoinLobby));

  auto lobbies_menu = CCMenu::create();
  lobbies_menu->setAnchorPoint({ 0.5f, 0.5f });
  lobbies_menu->setPosition(
      { bg2_layer->getPositionX() + (bg2_layer->getContentWidth() / 2),
        0.82f * height });
  lobbies_menu->setContentWidth(0.95f * bg2_layer->getContentWidth());
  lobbies_menu->setLayout(RowLayout::create()
                              ->setAxisAlignment(AxisAlignment::Start)
                              ->setGap(10)
                              ->setAutoScale(true)
                              ->setDefaultScaleLimits(0.5f, 0.8f));
  lobbies_menu->setID("lobbies-menu");
  lobbies_menu->addChild(create_lobby_button);
  lobbies_menu->addChild(join_lobby_button);
  lobbies_menu->updateLayout();
  m_mainLayer->addChild(lobbies_menu, 1);

  auto lobbies_list = ScrollLayer::create(bg2_layer->getContentSize());
  lobbies_list->m_contentLayer->setLayout(
      ColumnLayout::create()
          ->setAxisReverse(true)
          ->setAxisAlignment(AxisAlignment::End)
          ->setAutoGrowAxis(bg2_layer->getContentHeight())
          ->setGap(0));
  lobbies_list->setAnchorPoint({});
  bg2_layer->addChild(lobbies_list);

  auto list_borders = ListBorders::create();
  list_borders->setAnchorPoint({ 0.5f, 0.5f });
  list_borders->setPosition(bg2_layer->getPosition() +
                            (bg2_layer->getContentSize() / 2));
  list_borders->setContentSize(bg2_layer->getContentSize() + CCSize{ 6, 6 });
  m_mainLayer->addChild(list_borders);

  bool colored = true;
  for (const char* name : { "Global", "Angel", "Alizer" })
  {
    lobbies_list->m_contentLayer->addChild(
        LobbyCell::create(std::make_shared<LobbyInfo>(
                              0, name, 0, CCSprite::create("earth.png"_spr)),
                          bg2_layer->getContentWidth(),
                          bg2_layer->getContentHeight() / 5, colored));
    lobbies_list->m_contentLayer->addChild(CCLayerColor::create(
        { 0, 0, 0, 150 }, bg2_layer->getContentWidth(), 2.2f));
    colored = !colored;
  }

  lobbies_list->m_contentLayer->updateLayout();
  return true;
}

void LobbiesPopup::show()
{
  if (m_noElasticity)
    return FLAlertLayer::show();
  GLubyte opacity = getOpacity();
  m_mainLayer->setScale(0.1f);
  m_mainLayer->runAction(
      CCEaseElasticOut::create(CCScaleTo::create(0.3f, 1.0f), 1.6f));
  if (!m_scene)
    m_scene = CCDirector::sharedDirector()->getRunningScene();
  if (!m_ZOrder)
    m_ZOrder = 105;
  m_scene->addChild(this);
  setOpacity(0);
  runAction(CCFadeTo::create(0.14, opacity));
  setVisible(true);
}

void LobbiesPopup::onCreateLobby(CCObject*)
{
  FLAlertLayer::create("GDMX", "Creating Lobby", "OK")->show();
}

void LobbiesPopup::onJoinLobby(cocos2d::CCObject*)
{
  FLAlertLayer::create("GDMX", "Joining Lobby", "OK")->show();
}
