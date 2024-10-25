#include "lobbies_popup.hpp"
#include "lobby_cell.hpp"
#include "create_lobby_popup.hpp"
#include "gdmx_manager.hpp"
#include <utils/logging.hpp>
#include <eos/portable_iarchive.hpp>
#include <eos/portable_oarchive.hpp>
#include <boost/asio/experimental/awaitable_operators.hpp>
#include <variant>
#include <Geode/Geode.hpp>
namespace asio = boost::asio;
using namespace asio::experimental::awaitable_operators;
using namespace std::chrono_literals;
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
  bool circle_active = true;

  std::vector<geode::Ref<LobbyCell>> cells;

  if (auto lobby = ActiveLobby::get())
  {
    auto& cell = cells.emplace_back(
        LobbyCell::create(lobby->data(), { bg_width, bg_height / 5 }, colored));
    cell->markJoined(lobby->isHost());
    colored = false;
  }

  udp::socket   socket{ ctx, udp::v4() };
  udp::endpoint local_target{ asio::ip::address_v4::broadcast(),
                              lookup_socket_port };
  socket.set_option(udp::socket::reuse_address(true));
  socket.set_option(asio::socket_base::broadcast(true));

  {
    asio::streambuf        buffer;
    eos::portable_oarchive archive{ buffer };
    archive << RequestType::FetchLobbies;

    co_await socket.async_send_to(buffer.data(), local_target);
  }

  log::debug("listening for lobbies...");

  const auto wait_start = std::chrono::steady_clock::now();

  while (true)
  {
    asio::streambuf           buffer;
    boost::system::error_code errc;
    udp::endpoint             sender;
    asio::steady_timer        timer{ ctx, response_timeout };

    auto result = co_await (
        socket.async_receive_from(
            buffer.prepare(max_response_size), sender,
            asio::redirect_error(asio::use_awaitable, errc)) ||
        timer.async_wait(asio::redirect_error(asio::use_awaitable, errc)));

    size_t* plen = std::get_if<size_t>(&result);
    if (!plen)
    {
      if (!cells.empty())
      {
        for (auto& cell : cells)
          lobbies_list->m_contentLayer->addChild(cell);
        lobbies_list->m_contentLayer->updateLayout();
        cells.clear();
      }

      if (circle_active)
      {
        circle->removeFromParent();
        circle_active = false;
      }

      if ((std::chrono::steady_clock::now() - wait_start) > 5s)
        break;
      continue;
    }

    buffer.commit(*plen);

    auto  type = RequestType::Empty;
    Lobby lobby;

    try
    {
      eos::portable_iarchive archive{ buffer };
      archive >> type;
      if (type != RequestType::SendLobby)
        continue;
      archive >> lobby;
    }
    catch (const boost::archive::archive_exception& exception)
    {
      log::error("Archive Exception - {}", exception.what());
      if (type != RequestType::Empty)
        log::error("Request was {}: {}", fmt::underlying(type), type);
      continue;
    }

    if (auto active_lobby = ActiveLobby::get())
    {
      if (active_lobby->hostID() == lobby.host_id)
        continue;
    }

    cells.emplace_back(LobbyCell::create(std::move(lobby),
                                         { bg_width, bg_height / 5 }, colored));
    colored = !colored;
  }

  log::debug("done listening for lobbies");
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
