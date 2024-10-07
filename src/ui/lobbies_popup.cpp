#include "lobbies_popup.hpp"
#include "lobby_cell.hpp"
#include "create_lobby_popup.hpp"
#include "lobby.hpp"
#include "gdmx_manager.hpp"
#include "requests.hpp"
#include <boost/archive/binary_oarchive.hpp>
#include <boost/archive/binary_iarchive.hpp>
#include <boost/asio/experimental/awaitable_operators.hpp>
#include <boost/iostreams/device/array.hpp>
#include <boost/iostreams/stream.hpp>
#include <variant>
#include <Geode/Geode.hpp>
namespace asio = boost::asio;
namespace bio  = boost::iostreams;
using namespace asio::experimental::awaitable_operators;
using namespace cocos2d;
using asio::ip::udp;

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

  background =
      CCLayerColor::create({ .a = 75 }, 0.8f * width, (0.8f * height) - 25);
  background->setAnchorPoint({});
  background->setPosition({ width / 16, 13 });
  m_mainLayer->addChild(background);

  auto [bg_width, bg_height] = background->getContentSize();

  auto create_lobby_button =
      CCMenuItemSpriteExtra::create(ButtonSprite::create("Create Lobby"), this,
                                    menu_selector(LobbiesPopup::onCreateLobby));
  auto join_lobby_button =
      CCMenuItemSpriteExtra::create(ButtonSprite::create("Join Lobby"), this,
                                    menu_selector(LobbiesPopup::onJoinLobby));

  auto lobbies_menu = CCMenu::create();
  lobbies_menu->setAnchorPoint({ 0.5f, 0.5f });
  lobbies_menu->setPosition(
      { background->getPositionX() + (bg_width / 2), 0.82f * height });
  lobbies_menu->setContentWidth(0.95f * bg_width);
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

  lobbies_list = geode::ScrollLayer::create(background->getContentSize());
  lobbies_list->m_contentLayer->setLayout(
      ColumnLayout::create()
          ->setAxisReverse(true)
          ->setAxisAlignment(AxisAlignment::End)
          ->setAutoGrowAxis(bg_height)
          ->setGap(0));
  lobbies_list->setAnchorPoint({});
  background->addChild(lobbies_list);

  auto list_borders = geode::ListBorders::create();
  list_borders->setAnchorPoint({ 0.5f, 0.5f });
  list_borders->setPosition(background->getPosition() +
                            (background->getContentSize() / 2));
  list_borders->setContentSize(background->getContentSize() + CCSize{ 6, 6 });
  m_mainLayer->addChild(list_borders);

  asio::co_spawn(ctx,
                 refresh(background->convertToWorldSpace(
                     background->getContentSize() / 2)),
                 asio::detached);
  scheduleUpdate();
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

void LobbiesPopup::update(float delta)
{
  ctx.poll();
  if (ctx.stopped())
    unscheduleUpdate();
}

// this runs in the gd thread so cocos ui manipulations are safe
asio::awaitable<void> LobbiesPopup::refresh(CCPoint circle_pos)
{
  using namespace std::chrono_literals;
  auto [bg_width, bg_height] = background->getContentSize();
  bool colored               = true;

  auto* circle = LoadingCircle::create();
  circle->m_sprite->setPosition(circle_pos);
  circle->show();

  if (auto lobby = ActiveLobby::get())
  {
    auto* cell = LobbyCell::create(lobby->info(), bg_width, bg_height, colored);
    cell->markJoined(lobby->isHost());
    lobbies_list->m_contentLayer->addChild(cell);
    lobbies_list->m_contentLayer->updateLayout();
    colored = false;
  }

  udp::socket   socket{ ctx, udp::v4() };
  udp::endpoint local_target{ asio::ip::address_v4::broadcast(),
                              lookup_socket_port };
  socket.set_option(udp::socket::reuse_address(true));
  socket.set_option(asio::socket_base::broadcast(true));

  {
    asio::streambuf                 buffer;
    std::ostream                    stream{ &buffer };
    boost::archive::binary_oarchive archive{ stream };
    archive << RequestType::FetchLobbies;

    co_await socket.async_send_to(buffer.data(), local_target,
                                  asio::use_awaitable);
  }

  while (true)
  {
    udp::endpoint      sender{};
    std::vector<char>  response;
    asio::steady_timer timer{ ctx, response_timeout };

    std::variant<size_t, std::monostate> result =
        co_await (socket.async_receive_from(asio::buffer(response), sender,
                                            asio::use_awaitable) ||
                  timer.async_wait(asio::use_awaitable));

    if (std::holds_alternative<std::monostate>(result))
      break;

    if (sender != local_target)
      continue;

    bio::stream_buffer<bio::array_source> buffer{ response.data(),
                                                  response.size() };
    boost::archive::binary_iarchive       archive{ buffer };

    RequestType type{};
    archive >> type;

    if (type != RequestType::SendLobby)
      continue;

    Lobby lobby{};
    archive >> lobby;

    if (auto active_lobby = ActiveLobby::get())
    {
      if (active_lobby->info().host_id == lobby.host_id)
        continue;
    }

    lobbies_list->m_contentLayer->addChild(
        LobbyCell::create(lobby, bg_width, bg_height, colored));
    colored = !colored;
    lobbies_list->m_contentLayer->updateLayout();
  }

  circle->removeFromParent();
}

void LobbiesPopup::onCreateLobby(CCObject*)
{
  if (auto lobby = ActiveLobby::get())
  {
    if (lobby->isHost())
    {
      FLAlertLayer::create("Already Created",
                           "There is already a lobby created by this "
                           "user\nDelete it and create a new one if needed",
                           "OK")
          ->show();
      return;
    }

    geode::createQuickPopup("Already in lobby",
                            "You are already in another lobby, do you want to "
                            "unjoin and create a new one?",
                            "No", "Yes",
                            [](auto*, bool btn2)
                            {
                              if (!btn2)
                                return;
                              GDMXManager::get().unjoinLobby();
                              CreateLobbyPopup::create()->show();
                            });
    return;
  }

  CreateLobbyPopup::create()->show();
}

void LobbiesPopup::onJoinLobby(cocos2d::CCObject*)
{
  FLAlertLayer::create("GDMX", "Joining Lobby", "OK")->show();
}
