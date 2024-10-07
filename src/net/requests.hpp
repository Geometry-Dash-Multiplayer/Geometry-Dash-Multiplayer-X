#pragma once
#include <cstdint>
#include <chrono>

enum class RequestType : uint32_t
{
  FetchLobbies,
  SendLobby,
  JoinLobby,
  JoinSuccessful,
  UnjoinLobby,
  RetainJoin,
  PlayerAdd    = JoinLobby,
  PlayerRemove = UnjoinLobby
};

inline const char* to_string(RequestType type)
{
  switch (type)
  {
  case RequestType::FetchLobbies: return "Fetch Lobbies";
  case RequestType::SendLobby: return "Send Lobby";
  case RequestType::JoinLobby: return "Join Lobby";
  case RequestType::JoinSuccessful: return "Join Successful";
  case RequestType::UnjoinLobby: return "Unjoin Lobby";
  case RequestType::RetainJoin: return "Retain Join";
  default: return "Unknown";
  }
}

enum class EventType : uint32_t
{
  ErasePlayer
};

inline constexpr size_t lookup_socket_port = 55989;
inline constexpr size_t main_socket_port   = 55898;
inline constexpr auto   response_timeout   = std::chrono::milliseconds(250);
