#include "options_menu.hpp"
#include <Geode/Geode.hpp>
using namespace geode::prelude;

OptionsMenu::OptionsMenu(float width)
{
  constexpr float height = 30;
  CCNode::init();
  setContentSize({ width, height });

  const auto menu = CCMenu::create();
  menu->ignoreAnchorPointForPosition(false);
  menu->setID("options-arrows");
  addChild(menu);

  const auto left_arrow = CCMenuItemSpriteExtra::create(
      CCSprite::createWithSpriteFrameName("edit_leftBtn_001.png"), this,
      menu_selector(OptionsMenu::onLeftArrow));
  left_arrow->setPosition({ 0.05f * width, height / 2 });
  menu->addChild(left_arrow);

  const auto right_arrow = CCMenuItemSpriteExtra::create(
      CCSprite::createWithSpriteFrameName("edit_rightBtn_001.png"), this,
      menu_selector(OptionsMenu::onRightArrow));
  right_arrow->setPosition({ 0.95f * width, height / 2 });
  menu->addChild(right_arrow);
}

OptionsMenu* OptionsMenu::create(float width)
{
  auto obj = new OptionsMenu(width);
  obj->autorelease();
  return obj;
}

void OptionsMenu::setupOptions()
{
  auto [width, height] = getContentSize();
  option_text_layout   = CCNode::create();
  option_text_layout->setAnchorPoint({});
  option_text_layout->setContentSize({ width * 0.8f, height });
  option_text_layout->setPosition({ 0.1f * width, 5.5f });
  option_text_layout->setLayout(RowLayout::create()
                                    ->setAxisAlignment(AxisAlignment::Center)
                                    ->setAutoScale(true)
                                    ->setDefaultScaleLimits(0.35f, 0.7f));
  option_text_layout->setID("option-text-layout");
  addChild(option_text_layout);

  option_text = CCLabelBMFont::create(options.front().c_str(), "bigFont.fnt");
  option_text->setID("option-text");
  option_text_layout->addChild(option_text);
  option_text_layout->updateLayout();
}

void OptionsMenu::onLeftArrow(CCObject*)
{
  if (current_options_index)
    --current_options_index;
  else
    current_options_index = options.size() - 1;

  option_text->setString(options[current_options_index].c_str());
  option_text_layout->updateLayout();
}

void OptionsMenu::onRightArrow(CCObject*) 
{
  if (current_options_index != (options.size() - 1))
    ++current_options_index;
  else
    current_options_index = 0;

  option_text->setString(options[current_options_index].c_str());
  option_text_layout->updateLayout();
}
