#include "lobby_cell.h"
#include "gdmx_manager.h"
#include <Geode/Geode.hpp>
using namespace geode::prelude;

LobbyCell* LobbyCell::create(const std::shared_ptr<LobbyInfo>& lobby,
                             float width, float height, bool colored)
{
  auto obj = new LobbyCell(lobby, colored);
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
  if (!CCLayer::init())
    return false;
  setContentSize({ width, height });
  setAnchorPoint({});

  if (colored)
  {
    coloredLayer = CCLayerColor::create({ 0, 0, 0, 75 }, width, height);
    coloredLayer->setAnchorPoint({});
    addChild(coloredLayer);
  }

  auto cell_items = CCNode::create();
  cell_items->setAnchorPoint({ 0, 0.5f });
  cell_items->setPosition({ width / 32, height / 2 });
  cell_items->setContentWidth(width / 2);
  cell_items->setLayout(
      RowLayout::create()->setAxisAlignment(AxisAlignment::Start)->setGap(5));
  cell_items->setID("cell-items");
  addChild(cell_items);

  lobby->icon->setLayoutOptions(
      AxisLayoutOptions::create()->setScaleLimits(0.1f, 0.1f));
  cell_items->addChild(lobby->icon);

  auto lobby_text = CCLabelBMFont::create(lobby->name.c_str(), "bigFont.fnt");
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

  joinButton = CCMenuItemSpriteExtra::create(ButtonSprite::create("Join"), this,
                                             menu_selector(LobbyCell::onJoin));
  joinButton->setTag(true);
  joinButton->setScale(joinButton->m_baseScale = 0.6f);
  joinButton->setPosition(
      { width - (width / 32) - (joinButton->getScaledContentWidth() / 2),
        height / 2 });
  auto join_menu = CCMenu::create();
  join_menu->ignoreAnchorPointForPosition(false);
  join_menu->setID("join-menu");
  join_menu->addChild(joinButton);
  addChild(join_menu);

  cell_items->updateLayout();
  return true;
}

void LobbyCell::unjoinFromOther(const LobbyInfo* otherLobby)
{
  if (!otherLobby || otherLobby == lobby.get())
    return;
  auto cells  = getParent()->getChildren()->data;
  auto result = std::find_if(cells->arr, cells->arr + cells->num,
                             [otherLobby](CCObject* item)
                             {
                               if (auto cell = dynamic_cast<LobbyCell*>(item))
                                 return cell->lobby.get() == otherLobby;
                               return false;
                             });
  if (result != (cells->arr + cells->num))
    static_cast<LobbyCell*>(*result)->unjoin();
}

void LobbyCell::join()
{
  static_cast<ButtonSprite*>(joinButton->getNormalImage())->setString("Unjoin");
  if (coloredLayer)
    coloredLayer->setColor({ 0, 0xFF, 0 });
  else
  {
    coloredLayer = CCLayerColor::create({ 0, 0xFF, 0, 75 }, getContentWidth(),
                                        getContentHeight());
    addChild(coloredLayer, -1);
  }

  unjoinFromOther(GDMXManager::get().joinedLobby());
  GDMXManager::get().join(lobby);
  joinButton->setTag(false);
}

void LobbyCell::unjoin()
{
  static_cast<ButtonSprite*>(joinButton->getNormalImage())->setString("Join");
  if (colored)
    coloredLayer->setColor({});
  else
    removeChild(std::exchange(coloredLayer, nullptr));
  GDMXManager::get().unjoin();
  joinButton->setTag(true);
}

void LobbyCell::onInfo(CCObject*)
{
  FLAlertLayer::create("Lobby Info", "Shows Info", "OK")->show();
}

void LobbyCell::onLock(CCObject*)
{
  FLAlertLayer::create("Access", "Lobby Accessible", "OK")->show();
}

void LobbyCell::onJoin(CCObject*)
{
  if (joinButton->getTag())
    return join();
  unjoin();
}
