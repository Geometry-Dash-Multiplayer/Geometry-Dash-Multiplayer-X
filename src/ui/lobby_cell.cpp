#include "lobby_cell.hpp"
#include "gdmx_manager.hpp"
#include "requests.hpp"
#include "tasks.hpp"
#include <boost/iostreams/device/array.hpp>
#include <boost/iostreams/stream.hpp>
#include <boost/archive/binary_oarchive.hpp>
#include <boost/archive/binary_iarchive.hpp>
#include <boost/asio/experimental/awaitable_operators.hpp>
#include <Geode/Geode.hpp>
namespace asio = boost::asio;
namespace bio  = boost::iostreams;
using namespace asio::experimental::awaitable_operators;
using namespace geode::prelude;
using asio::ip::udp;

LobbyCell::~LobbyCell()
{
  if (circle)
    circle->removeFromParent();
}

LobbyCell* LobbyCell::create(Lobby&& lobby, float width, float height,
                             bool colored)
{
  auto obj = new LobbyCell(std::move(lobby), colored);
  if (obj->init(width, height, colored))
  {
    obj->autorelease();
    return obj;
  }

  delete obj;
  return nullptr;
}

LobbyCell* LobbyCell::create(const Lobby& lobby, float width, float height,
                             bool colored)
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
  if (!CCNode::init())
    return false;
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
  cell_items->setContentWidth(width / 2);
  cell_items->setLayout(
      RowLayout::create()->setAxisAlignment(AxisAlignment::Start)->setGap(5));
  cell_items->setID("cell-items");
  cell_main_layer->addChild(cell_items);

  auto icon = CCSprite::create("earth.png"_spr);
  icon->setLayoutOptions(
      AxisLayoutOptions::create()->setScaleLimits(0.1f, 0.1f));
  icon->setID("lobby-icon");
  cell_items->addChild(icon);

  auto lobby_name = CCLabelBMFont::create(lobby.name.c_str(), "bigFont.fnt");
  lobby_name->setScale(0.5f);
  lobby_name->setLayoutOptions(
      AxisLayoutOptions::create()->setAutoScale(true)->setScaleLimits(0.35f,
                                                                      0.5f));
  lobby_name->setID("lobby-name");
  cell_items->addChild(lobby_name);

  int         total_user_count = rand(), user_count = rand();
  std::string counter_text =
      fmt::format("{}/{}", total_user_count % user_count, total_user_count);

  auto user_count_label =
      CCLabelBMFont::create(counter_text.c_str(), "bigFont.fnt");
  user_count_label->setScale(0.2f);
  user_count_label->setPosition(user_count_label->getScaledContentSize() / 2);

  auto user_count_container = CCNode::create();
  user_count_container->setContentSize(
      { user_count_label->getScaledContentWidth(),
        lobby_name->getScaledContentHeight() / 2 });
  user_count_container->addChild(user_count_label);
  cell_items->addChild(user_count_container);

  auto info_button = CCMenuItemSpriteExtra::create(
      CCSprite::createWithSpriteFrameName("GJ_infoIcon_001.png"), this,
      menu_selector(LobbyCell::onInfo));
  info_button->setScale(info_button->m_baseScale = 0.6f);
  info_button->setPosition(info_button->getScaledContentSize() / 2);

  auto info_menu = CCMenu::create();
  info_menu->setContentSize({ info_button->getScaledContentWidth(),
                              lobby_name->getScaledContentHeight() });
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
                              lobby_name->getScaledContentHeight() });
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
  static_cast<ButtonSprite*>(join_button->getNormalImage())
      ->setString(new_text);
  if (colored_layer)
    colored_layer->setColor({ .b = 0xFF });
  else
  {
    auto [width, height] = cell_main_layer->getContentSize();
    colored_layer = CCLayerColor::create({ .b = 0xFF, .a = 75 }, width, height);
    cell_main_layer->addChild(colored_layer, -1);
  }
  join_button->setTag(false);
}

void LobbyCell::markUnjoined()
{
  static_cast<ButtonSprite*>(join_button->getNormalImage())->setString("Join");
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
    const auto  other_host_id = lobby->info().host_id;
    const auto  cells = CCArrayExt<LobbyCell*>(getParent()->getChildren());
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

  asio::streambuf                 buffer;
  std::ostream                    stream{ &buffer };
  boost::archive::binary_oarchive archive{ stream };
  archive << RequestType::UnjoinLobby
          << GDMXManager::get().getID(lobby.type == LobbyType::Local);

  co_await socket.async_send_to(buffer.data(), target);
}

asio::awaitable<void> LobbyCell::joinUnchecked()
{
  udp::endpoint target{ asio::ip::address_v4(lobby.host_id),
                        lookup_socket_port };
  udp::socket   socket{ ctx, udp::v4() };

  {
    asio::streambuf                 buffer;
    std::ostream                    stream{ &buffer };
    boost::archive::binary_oarchive archive{ stream };
    archive << RequestType::JoinLobby
            << GDMXPlayer::self(lobby.type == LobbyType::Local);

    co_await socket.async_send_to(buffer.data(), target);
  }

  while (true)
  {
    udp::endpoint      sender;
    std::vector<char>  response;
    asio::steady_timer timer{ ctx, response_timeout };

    std::variant<size_t, std::monostate> result =
        co_await (socket.async_receive_from(asio::buffer(response), sender,
                                            asio::use_awaitable) ||
                  timer.async_wait(asio::use_awaitable));

    if (std::holds_alternative<std::monostate>(result))
    {
      FLAlertLayer::create(
          "Unreachable Host",
          "The host could not be reached, maybe try again later", "OK")
          ->show();
      break;
    }

    if (sender != target)
      continue;

    RequestType                     type{};
    bio::stream<bio::array_source>  stream{ response.data(), response.size() };
    boost::archive::binary_iarchive archive{ stream };
    archive >> type;

    if (type != RequestType::JoinSuccessful)
    {
      FLAlertLayer::create(
          "Unknown Error",
          "An unknown error has occurred while joining the lobby", "OK")
          ->show();
      break;
    }

    GDMXManager::get().joinLobby(sender, lobby, socket.release());
    markJoined(false);
    break;
  }

  circle->removeFromParent();
  circle = nullptr;
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
  asio::co_spawn(ctx, join_button->getTag() ? join() : unjoin(),
                 asio::detached);
  scheduleUpdate();
}
