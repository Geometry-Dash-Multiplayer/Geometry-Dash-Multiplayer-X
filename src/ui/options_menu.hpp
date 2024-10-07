#pragma once
#include <vector>
#include <string>
#include <string_view>
#include <Geode/cocos/include/cocos2d.h>
#include <Geode/ui/General.hpp>

class OptionsMenu : public cocos2d::CCNode
{
public:
  OptionsMenu(float width);

  static OptionsMenu* create(float width);

  template <typename... types>
  void addOptions(types&&... args)
      requires((std::convertible_to<types, std::string> && ...) &&
               sizeof...(args) != 0)
  {
    (options.emplace_back(std::forward<types>(args)), ...);
    if (!option_text)
      setupOptions();
  }

  std::string_view getSelectedOption()
  {
    return options[current_options_index];
  }

protected:
  std::vector<std::string> options;
  size_t                   current_options_index = 0;
  cocos2d::CCNode*         option_text_layout    = nullptr;
  cocos2d::CCLabelBMFont*  option_text           = nullptr;

  void setupOptions();
  void onLeftArrow(cocos2d::CCObject*);
  void onRightArrow(cocos2d::CCObject*);
};
