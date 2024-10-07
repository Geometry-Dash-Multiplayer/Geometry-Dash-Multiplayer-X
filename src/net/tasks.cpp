#include "tasks.hpp"
#include "requests.hpp"
#include <boost/archive/binary_iarchive.hpp>
#include <boost/archive/binary_oarchive.hpp>
#include <boost/iostreams/device/array.hpp>
#include <boost/iostreams/stream.hpp>
namespace bio  = boost::iostreams;
namespace asio = boost::asio;
using namespace std::chrono_literals;
using asio::ip::udp;

BackgroundTasks::BackgroundTasks(report_callback&&                 report,
                                 const std::optional<handle_type>& handle)
    : socket(ctx), lookup_socket(ctx), report(std::move(report))
{
  if (handle)
    socket.assign(udp::v4(), *handle);
  else
    socket.open(udp::v4());

  if (auto* lobby = HostedLobby::get())
  {
    lookup_socket.open(udp::v4());

    if (lobby->info().type == LobbyType::Local)
    {
      if (!handle)
        socket.bind(udp::endpoint(udp::v4(), main_socket_port));
      lookup_socket.bind(udp::endpoint(udp::v4(), lookup_socket_port));

      asio::co_spawn(ctx, listenForNewClients(), asio::detached);
    }
  }

  asio::co_spawn(ctx, listen(), asio::detached);
}

asio::awaitable<void> BackgroundTasks::listenForNewClients()
{
  while (true)
  {
    std::vector<char> response;
    udp::endpoint     sender;

    co_await lookup_socket.async_receive_from(asio::buffer(response), sender);
    RequestType type{};

    {
      bio::stream_buffer<bio::array_source> buffer{ response.data(),
                                                    response.size() };
      boost::archive::binary_iarchive       archive{ buffer };
      archive >> type;

      auto* lobby = HostedLobby::get();
      if (type == RequestType::JoinLobby)
      {
        GDMXPlayer player{};
        archive >> player;
        lobby->registerPlayer(player, sender);
      }
    }

    if (type != RequestType::FetchLobbies && type != RequestType::JoinLobby)
    {
      geode::log::debug("unhandled request type: {}", to_string(type));
      continue;
    }

    asio::streambuf                 buffer;
    std::ostream                    stream{ &buffer };
    boost::archive::binary_oarchive archive{ stream };

    if (type == RequestType::FetchLobbies)
      archive << RequestType::SendLobby << ActiveLobby::get()->info();
    else
      archive << RequestType::JoinSuccessful << HostedLobby::get()->players;

    co_await lookup_socket.async_send_to(buffer.data(), sender,
                                         asio::use_awaitable);
  }
}

asio::awaitable<void> BackgroundTasks::listen()
{
  while (true)
  {
    std::vector<char> response;
    udp::endpoint     sender;

    co_await socket.async_receive_from(asio::buffer(response), sender);

    RequestType type{};

    {
      bio::stream_buffer<bio::array_source> buffer{ response.data(),
                                                    response.size() };
      boost::archive::binary_iarchive       archive{ buffer };
      archive >> type;

      if (auto* lobby = HostedLobby::get())
      {
        if (type == RequestType::RetainJoin)
        {
          uint64_t id = 0;
          archive >> id;
          lobby->players[id].last_update = std::chrono::steady_clock::now();
          continue;
        }
      }
      else
      {
        if (type == RequestType::PlayerRemove)
        {
          uint64_t id = 0;
          archive >> id;
          ActiveLobby::get()->erasePlayer(id);
          continue;
        }
      }
    }

    geode::log::debug("unhandled request type: {}", to_string(type));
  }
}
