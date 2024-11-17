#include "lobby_cell.hpp"
#include "gdmx_manager.hpp"
#include <eos/portable_iarchive.hpp>
#include <eos/portable_oarchive.hpp>
#include <net/requests.hpp>
#include <utils/logging.hpp>
#include <boost/asio/experimental/awaitable_operators.hpp>
#include <Geode/Geode.hpp>
namespace asio = boost::asio;
using namespace asio::experimental::awaitable_operators;
using namespace cocos2d;
using asio::ip::udp;

LobbyCell::~LobbyCell()
{
  if (circle)
    circle->removeFromParent();
}

bool LobbyCell::init(CCSize size, bool colored)
{
  if (!CCNode::init())
    return false;
  auto [width, height] = size;

  setContentSize({ width, height + 2.2f });
  setAnchorPoint({});
  setLayout(ColumnLayout::create()
                ->setAxisReverse(true)
                ->setAxisAlignment(AxisAlignment::End)
                ->setGap(0));

  cell_main_layer = CCNode::create();
  cell_main_layer->setContentSize({ width, height });
  cell_main_layer->setID("cell-main-layer");
  addChild(cell_main_layer);

  if (colored)
  {
    colored_layer = CCLayerColor::create({ .a = 75 }, width, height);
    cell_main_layer->addChild(colored_layer);
  }

  auto cell_items = CCNode::create();
  cell_items->setAnchorPoint({ 0, 0.5f });
  cell_items->setPosition({ width / 32, height / 2 });
  cell_items->setContentWidth(width * 0.6f);
  cell_items->setLayout(RowLayout::create()
                            ->setAxisAlignment(AxisAlignment::Start)
                            ->setGap(5)
                            ->setAutoScale(false));
  cell_items->setID("cell-items");
  cell_main_layer->addChild(cell_items);

  auto icon = CCSprite::create("earth.png"_spr);
  icon->setScale(0.1f);
  icon->setAnchorPoint({});
  icon->setID("icon");

  auto icon_wrapper = CCNode::create();
  icon_wrapper->setContentSize(icon->getScaledContentSize());
  icon_wrapper->setID("icon-wrapper");
  icon_wrapper->addChild(icon);
  cell_items->addChild(icon_wrapper);

  auto name = CCLabelBMFont::create(lobby.name.c_str(), "bigFont.fnt");
  name->setAnchorPoint({});
  name->setID("name");

  auto name_wrapper = CCNode::create();
  name_wrapper->setContentSize(
      { name->getContentWidth(), name->getContentHeight() * 0.85f });
  name_wrapper->setScale(0.4f);
  name_wrapper->setLayoutOptions(
      AxisLayoutOptions::create()->setAutoScale(true)->setScaleLimits(0.3f,
                                                                      0.5f));
  name_wrapper->setID("name-wrapper");
  name_wrapper->addChild(name);
  cell_items->addChild(name_wrapper);

  std::string counter_text = std::to_string(lobby.player_count);
  auto user_count = CCLabelBMFont::create(counter_text.c_str(), "bigFont.fnt");
  user_count->setScale(0.3f);
  user_count->setAnchorPoint({});
  user_count->setID("user-count");

  auto user_count_wrapper = CCNode::create();
  user_count_wrapper->setContentSize(
      { user_count->getScaledContentWidth(),
        user_count->getScaledContentHeight() * 0.9f });
  user_count_wrapper->setID("user-count-wrapper");
  user_count_wrapper->addChild(user_count);
  cell_items->addChild(user_count_wrapper);

  auto info_button = CCMenuItemSpriteExtra::create(
      CCSprite::createWithSpriteFrameName("GJ_infoIcon_001.png"), this,
      menu_selector(LobbyCell::onInfo));
  info_button->setScale(info_button->m_baseScale = 0.6f);
  info_button->setPosition(info_button->getScaledContentSize() / 2);

  auto info_menu = CCMenu::create();
  info_menu->setContentSize(info_button->getScaledContentSize());
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
  lock_menu->setContentSize(lock_button->getScaledContentSize());
  lock_menu->setID("lock-menu");
  lock_menu->addChild(lock_button);
  cell_items->addChild(lock_menu);

  join_button = CCMenuItemSpriteExtra::create(
      ButtonSprite::create("Join"), this, menu_selector(LobbyCell::onJoin));
  join_button->setTag(true);
  join_button->setScale(join_button->m_baseScale = 0.6f);
  join_button->setPosition(
      { width - (width / 32) - (join_button->getScaledContentWidth() / 2),
        height / 2 });
  auto join_menu = CCMenu::create();
  join_menu->ignoreAnchorPointForPosition(false);
  join_menu->setID("join-menu");
  join_menu->addChild(join_button);
  cell_main_layer->addChild(join_menu);

  auto separator = CCLayerColor::create({ .a = 150 }, width, 2.2f);
  separator->setID("cell-separator");
  addChild(separator);

  cell_items->updateLayout();
  updateLayout();
  return true;
}

void LobbyCell::update(float delta)
{
  ctx.poll();
  if (ctx.stopped())
  {
    unscheduleUpdate();
    ctx.restart();
  }
}

void LobbyCell::markJoined(bool is_host)
{
  const char* new_text = is_host ? "Delete" : "Unjoin";
  auto [width, height] = cell_main_layer->getContentSize();

  static_cast<ButtonSprite*>(join_button->getNormalImage())
      ->setString(new_text);
  join_button->setPositionX(width - (width / 32) -
                            (join_button->getScaledContentWidth() / 2));

  if (colored_layer)
  {
    colored_layer->setColor({ .g = 0xFF });
  }
  else
  {
    colored_layer = CCLayerColor::create({ .g = 0xFF, .a = 75 }, width, height);
    cell_main_layer->addChild(colored_layer, -1);
  }
  join_button->setTag(false);
}

void LobbyCell::markUnjoined()
{
  auto [width, height] = cell_main_layer->getContentSize();
  static_cast<ButtonSprite*>(join_button->getNormalImage())->setString("Join");
  join_button->setPositionX(width - (width / 32) -
                            (join_button->getScaledContentWidth() / 2));

  if (colored)
    colored_layer->setColor({});
  else
    removeChild(std::exchange(colored_layer, nullptr));
  join_button->setTag(true);
}

asio::awaitable<void> LobbyCell::join()
{
  circle = LoadingCircle::create();
  circle->setPosition(join_button->convertToWorldSpace(
      join_button->getScaledContentSize() / 2));
  join_button->setVisible(false);
  circle->show();

  if (auto lobby = ActiveLobby::get())
  {
    const auto other_host_id = lobby->hostID();
    const auto cells =
        geode::cocos::CCArrayExt<LobbyCell*>(getParent()->getChildren());
    auto* const result =
        std::ranges::find_if(cells, [other_host_id](LobbyCell* cell)
                             { return cell->lobby.host_id == other_host_id; });

    if (result != cells.end())
      co_await ((*result)->unjoin() && joinUnchecked());
    co_return;
  }

  co_await joinUnchecked();
}

asio::awaitable<void> LobbyCell::unjoin()
{
  markUnjoined();
  GDMXManager::get().unjoinLobby();
  join_button->setTag(true);

  udp::endpoint target{ asio::ip::address_v4(lobby.host_id), main_socket_port };
  udp::socket   socket{ ctx, udp::v4() };

  asio::streambuf        buffer;
  eos::portable_oarchive archive{ buffer };

  archive << RequestType::UnjoinLobby
          << GDMXManager::get().getID(lobby.type == LobbyType::Local);

  co_await socket.async_send_to(buffer.data(), target);
}

asio::awaitable<void> LobbyCell::erase()
{
  auto* parent = getParent();
  parent->removeChild(this, false);
  parent->updateLayout();

  udp::socket socket{ ctx, udp::v4() };
  co_await HostedLobby::get()->reportShutdown(socket);

  GDMXManager::get().unjoinLobby();
}

#if defined(__GNUC__) || defined(__clang__)
  #pragma GCC diagnostic push
  #pragma GCC diagnostic ignored "-Wdeprecated-declarations"
#elif defined(_MSC_VER)
  #pragma warning(push)
  #pragma warning(disable : 4996)
#endif

asio::awaitable<void> LobbyCell::joinUnchecked()
{
  assert(lobby.type == LobbyType::Local);
  udp::endpoint target{ asio::ip::address_v4(lobby.host_id),
                        lookup_socket_port };
  udp::socket   socket{ ctx, udp::v4() };

  {
    asio::streambuf        buffer;
    eos::portable_oarchive archive{ buffer };

    archive << RequestType::JoinLobby
            << GDMXPlayer::self(lobby.type == LobbyType::Local);

    co_await socket.async_send_to(buffer.data(), target);
  }

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
      FLAlertLayer::create(
          "Unreachable Host",
          "The host could not be reached, maybe try again later", "OK")
          ->show();
      break;
    }

    if (sender != target)
      continue;

    buffer.commit(*plen);
    auto type = RequestType::Empty;

    try
    {
      eos::portable_iarchive archive{ buffer };
      archive >> type;

      if (type != RequestType::JoinSuccessful)
      {
        FLAlertLayer::create("Invalid Response",
                             "Received a response with an improper flag (maybe "
                             "the host runs an outdated version of gdmx?)",
                             "OK")
            ->show();
        break;
      }

      GDMXManager::get().joinLobby(
          lobby,
          udp::endpoint(asio::ip::address_v4(lobby.host_id), main_socket_port),
          socket.release());
      markJoined(false);
    }
    catch (const boost::archive::archive_exception& exception)
    {
      FLAlertLayer::create("Corrupted Response",
                           "The response body contains invalid binary data, "
                           "maybe try again later",
                           "OK")
          ->show();
      output::error("Received Corrupted Response: {}", exception.what());
    }
    break;
  }

  circle->removeFromParent();
  circle = nullptr;
  join_button->setVisible(true);
}

#if defined(__GNUC__) || defined(__clang__)
  #pragma GCC diagnostic pop
#elif defined(_MSC_VER)
  #pragma warning(pop)
#endif

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
  asio::co_spawn(ctx,
                 join_button->getTag()          ? join()
                 : ActiveLobby::get()->isHost() ? erase()
                                                : unjoin(),
                 asio::detached);
  scheduleUpdate();
}
