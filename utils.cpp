#include "discord.hpp"
#include "utils.hpp"
#include "connection.hpp"
#include "gateway.hpp"
#include "bot.hpp"

#include <nlohmann/json.hpp>

namespace backend {
  /**
   * @brief: packages the payload into a readable input for the web request
   */
  nlohmann::json gateway::package(const discord::payload payload) {
    nlohmann::json data = {
      {"op", payload.op}
    };
    // nlohmann::json expects nullptr to insert null
    if(payload.s != 0) {
      data.update( { {"s", payload.s} } );
    }
    if(!payload.t.empty()) {
      data.update( { {"t", payload.t} } );
    }
    switch(payload.op) {
      case(discord::RESUME):
        data.update(
          {
            {"d", {
              { "token", bot->token },
              { "session_id", session_id },
              { "seq", last_sequence_data }}
            }
          });
        break;
      case(discord::REQUEST_GUILD_MEMBERS):
        data.update(
          {
            {"d", {
              {"guild_id", bot->guild_info.id},
              {"limit", 250},
              {"query", ""}
            }}
          });
        break;
      default:
        data.update(payload.d);
    }
    return data;
  }

  /**
   * @brief: turns raw json to payload object
   *
   * The json is directly taken from the incoming message of the wss and is turned
   * into a payload struct.
   *
   * @param[in]: nlohmann json object
   * @return: payload object with the respective fields
   * @bug: d can also be an integer
   */
  discord::payload gateway::unpack(const nlohmann::json msg) {
    return {
        static_cast<discord::opcodes>(msg["op"].get<int>()),
        msg["d"].is_null() ? nlohmann::json{} : msg["d"],
        msg["s"].is_null() ? 0 : msg["s"].get<int>(),
        msg["t"].is_null() ? "" : msg["t"].get<std::string>()};
  }

  discord::user parse_user(const nlohmann::json &user_obj) {
    discord::user user_info;
    user_info.avatar = user_obj["avatar"].is_null() ? 0 : std::stoul(user_obj["avatar"].get<std::string>());
    user_info.discriminator = user_obj["discriminator"].is_null() ? 0 : std::stoul(user_obj["discriminator"].get<std::string>());
    user_info.id = user_obj["id"].is_null() ? 0 : std::stoul(user_obj["id"].get<std::string>());
    user_info.username = user_obj["username"].is_null() ? "" : user_obj["username"].get<std::string>();
    return user_info;
  }

  discord::member parse_member(const nlohmann::json &member_obj) {
    discord::member member_info;
    member_info.deaf = member_obj["deaf"].is_null() ? false : member_obj["deaf"].get<bool>();
    member_info.mute = member_obj["mute"].is_null() ? false : member_obj["mute"].get<bool>();
    if(member_obj.contains("nick"))
      member_info.nick = member_obj["nick"].is_null() ? "" : member_obj["nick"].get<std::string>();
    member_info.usr_info = parse_user(member_obj["user"]);
    return member_info;
  }

  discord::role parse_role(const nlohmann::json &role_obj) {
    discord::role temp;
    temp.name = role_obj["name"].is_null() ? "" : role_obj["name"].get<std::string>();
    temp.id = role_obj["id"].is_null() ? 0 : std::stoul(role_obj["id"].get<std::string>());
    temp.managed = role_obj["managed"].is_null() ? false : role_obj["managed"].get<bool>();
    temp.mentionable = role_obj["mentionable"].is_null() ? false : role_obj["mentionable"].get<bool>();
    temp.permissions = role_obj["permissions"].is_null() ? 0 : role_obj["permissions"].get<uint64_t>();
    temp.position = role_obj["position"].is_null() ? 0 : role_obj["position"].get<short>();
    temp.hoist = role_obj["hoist"].is_null() ? false : role_obj["hoist"].get<bool>();
    return temp;
  }

  discord::channel parse_channel(const nlohmann::json &channel_obj) {
    discord::channel temp;
    temp.type = channel_obj["type"].is_null() ? 0 : channel_obj["type"].get<short>();
    temp.id = channel_obj["id"].is_null() ? 0 : std::stoul(channel_obj["id"].get<std::string>());

    if(temp.type != 1) {
      temp.name = channel_obj["name"].is_null() ? "" : channel_obj["name"].get<std::string>();
      temp.position = channel_obj["position"].is_null() ? 0 :channel_obj["position"].get<short>();
    }
    switch(temp.type) {
      case(1): {
        break;
      }
      case(0): {
        break;
      }
      case(2): {
        temp.bitrate = channel_obj["bitrate"].is_null() ? 0 : channel_obj["bitrate"].get<int>();
        temp.user_limit = channel_obj["user_limit"].is_null() ? 0 : channel_obj["user_limit"].get<short>();
        break;
      }
      case(4): {
        temp.user_limit = channel_obj["user_limit"].is_null() ? 0 : channel_obj["user_limit"].get<short>();
        break;
      }
    }
    return temp;
  }
}
