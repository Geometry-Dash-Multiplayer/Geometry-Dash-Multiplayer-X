#pragma once
#include <cstdint>
#include <chrono>
#include <variant>
#include <vector>

class GDMXPlayer;
class SyncData;

enum class RequestType : uint32_t
{
  Empty,
  FetchLobbies,
  SendLobby,
  JoinLobby,
  JoinSuccessful,
  UnjoinLobby,
  RetainJoin,
  PlayerEnteredLevel,
  PlayerExitedLevel,
  PlayerEnterSuccessful,
  PlayerExitSuccessful,
  PlayerSync,
  ServerShutdown,
  PlayerAdd    = JoinLobby,
  PlayerRemove = UnjoinLobby
};

inline const char* to_string(RequestType type)
{
  switch (type)
  {
  case RequestType::Empty: return "Empty";
  case RequestType::FetchLobbies: return "Fetch Lobbies";
  case RequestType::SendLobby: return "Send Lobby";
  case RequestType::JoinLobby: return "Join Lobby";
  case RequestType::JoinSuccessful: return "Join Successful";
  case RequestType::UnjoinLobby: return "Unjoin Lobby";
  case RequestType::RetainJoin: return "Retain Join";
  case RequestType::PlayerEnteredLevel: return "Player Entered Level";
  case RequestType::PlayerExitedLevel: return "Player Exited Level";
  case RequestType::PlayerEnterSuccessful: return "Player Enter Successful";
  case RequestType::PlayerExitSuccessful: return "Player Exit Successful";
  case RequestType::PlayerSync: return "Player Sync";
  case RequestType::ServerShutdown: return "Server Shutdown";
  default: return "Unknown";
  }
}

inline auto format_as(RequestType type) { return to_string(type); }

enum class EventType : uint32_t
{
  PlayerEnteredLevel,
  PlayerExitedLevel,
  PlayerSync,
  LevelPlayersList
};

inline const char* to_string(EventType type)
{
  switch (type)
  {
  case EventType::PlayerEnteredLevel: return "Player Entered Level";
  case EventType::PlayerExitedLevel: return "Player Exited Level";
  case EventType::PlayerSync: return "Player Sync";
  case EventType::LevelPlayersList: return "Level Players List";
  default: return "Unknown";
  }
}

inline auto format_as(EventType type) { return to_string(type); }

using PlayerEnteredLevelValue = std::pair<GDMXPlayer, uint32_t>;
using PlayerExitedLevelValue  = std::pair<uint64_t, uint32_t>;
using PlayerSyncValue         = std::pair<uint64_t, SyncData>;
using LevelPlayersListValue   = std::vector<GDMXPlayer>;
using EventValue = std::variant<PlayerEnteredLevelValue, PlayerExitedLevelValue,
                                PlayerSyncValue, LevelPlayersListValue>;

inline constexpr size_t lookup_socket_port = 55989;
inline constexpr size_t main_socket_port   = 55898;
inline constexpr auto   response_timeout   = std::chrono::milliseconds(500);
inline constexpr size_t max_response_size  = 480; // in bytes
