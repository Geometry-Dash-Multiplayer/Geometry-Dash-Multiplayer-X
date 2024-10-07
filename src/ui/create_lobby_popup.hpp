#pragma once
#include <Geode/ui/Popup.hpp>
#include <Geode/ui/TextInput.hpp>
#include "options_menu.hpp"

class CreateLobbyPopup : public geode::Popup<>
{
public:
  static CreateLobbyPopup* create();

protected:
  geode::TextInput* name_input  = nullptr;
  OptionsMenu*      type_option = nullptr;

  bool setup() override;
  void onSubmit(cocos2d::CCObject*);
};