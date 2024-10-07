#include "create_lobby_popup.hpp"
#include "gdmx_manager.hpp"
#include <Geode/Geode.hpp>
using namespace geode::prelude;

CreateLobbyPopup* CreateLobbyPopup::create()
{
  auto obj = new CreateLobbyPopup();
  if (obj->initAnchored(220, 150))
  {
    obj->autorelease();
    return obj;
  }

  delete obj;
  return nullptr;
}

bool CreateLobbyPopup::setup()
{
  setTitle("Create Lobby", "goldFont.fnt", 0.6f);
  auto [width, height] = m_mainLayer->getContentSize();

  // base layout
  CCNode* const layout_base = CCNode::create();
  layout_base->setAnchorPoint({ 0.5f, 0.5f });
  layout_base->setPosition({ width / 2, height * 0.52f });
  layout_base->setContentSize({ 0.7f * width, 0.5f * height });
  layout_base->setLayout(ColumnLayout::create()
                             ->setAxisReverse(true)
                             ->setAxisAlignment(AxisAlignment::End)
                             ->setDefaultScaleLimits(0.5f, 0.9f)
                             ->setGap(10));
  layout_base->setID("create-lobby-options");
  m_mainLayer->addChild(layout_base);

  // name input
  name_input = TextInput::create(width - (width / 8), "type lobby name");
  name_input->setAnchorPoint({});
  name_input->setLabel("Lobby Name");
  name_input->setID("lobby-name-input");
  auto name_input_node = CCNode::create();
  name_input_node->setContentSize(name_input->getContentSize() +
                                  CCSize{ 0, 10 });
  name_input_node->addChild(name_input);
  layout_base->addChild(name_input_node);

  // type option
  type_option = OptionsMenu::create(name_input->getContentWidth() * 0.75f);
  type_option->setID("lobby-type-options-menu");
  type_option->addOptions("Local", "Private", "Public");
  layout_base->addChild(type_option);

  layout_base->updateLayout();

  // bottom submit button
  auto menu = CCMenu::create();
  menu->setAnchorPoint({ 0.5f, 0.5f });
  menu->setPosition({ width / 2, 25 });
  menu->setContentWidth(0.95f * m_mainLayer->getContentWidth());
  menu->setLayout(RowLayout::create()
                      ->setAxisAlignment(AxisAlignment::Center)
                      ->setGap(10)
                      ->setAutoScale(true)
                      ->setDefaultScaleLimits(0.5f, 0.8f));
  menu->setID("submit-menu");
  m_mainLayer->addChild(menu);

  auto submit_button =
      CCMenuItemSpriteExtra::create(ButtonSprite::create("Submit"), this,
                                    menu_selector(CreateLobbyPopup::onSubmit));

  menu->addChild(submit_button);
  menu->updateLayout();
  return true;
}

void CreateLobbyPopup::onSubmit(CCObject*) 
{
  auto name = name_input->getString();
  if (name.empty())
  {
    FLAlertLayer::create("Incomplete", "No lobby name was specified", "OK")
        ->show();
    return;
  }

  auto type = type_option->getSelectedOption();
  if (type != "Local")
  {
    FLAlertLayer::create("Unsupported Type",
                         "The lobby type specified is not currently "
                         "supported\nExpect an update soon",
                         "OK")
        ->show();
    return;
  }

  GDMXManager::get().createLobby(LobbyType::Local, name);
  removeFromParent();
}
