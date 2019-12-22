#include "discord.hpp"
#include "gateway.hpp"
#include "bot.hpp"
#include "utils.hpp"
#include <boost/tokenizer.hpp>

#define EVENT_HANDLER(t) void gateway::event_##t(const discord::payload &msg)
#define GATEWAY_HANDLE(op) void gateway::handle_##op(const discord::payload &msg)

using namespace boost;

namespace backend {
  /**
   * @brief: handle the READY event
   *
   * The ready event provides the following information:
   *    1) sharding
   *    2) user object
   *    3) guilding
   *    4) session id
   */
  EVENT_HANDLER(READY) {
    session_id = msg.d["session_id"].get<std::string>();
    bot->guild_info.id = std::stoul(msg.d["guilds"][0]["id"].get<std::string>()); // part of only 1 guild
    bot->status = ACTIVE;
  }

  EVENT_HANDLER(RESUMED) {
    bot->up_to_date = true;
  }

  EVENT_HANDLER(RECONNECT) {
  }

  EVENT_HANDLER(HELLO) {}
  EVENT_HANDLER(INVALID_SESSION){}
  EVENT_HANDLER(CHANNEL_CREATE){}
  EVENT_HANDLER(CHANNEL_UPDATE){}
  EVENT_HANDLER(CHANNEL_DELETE){}
  EVENT_HANDLER(CHANNEL_PINS_UPDATE){}

  /**
   * @brief: grab guild information
   *
   * This is a very important event because it contains information about the guild as well as
   * all of the users that exists insdie of it. Pearlbot only exists in one guild so only one, so it
   * holds the guild object instead of vector of guilds. Since the guild create event is only sent
   * once, send a REQUEST_GUILD_MEMBERS to populate the user list.
   */
  EVENT_HANDLER(GUILD_CREATE) {
    try {
      bot->guild_info.member_count = msg.d["member_count"].get<int>();
      auto roles = msg.d["roles"];
      auto channels = msg.d["channels"];
      size_t idx = 0;
      while(!roles[idx++].is_null()) {
        (bot->guild_info.roles).push_back(parse_role(roles[idx]));
      }
      idx = 0;
      while(!channels[idx++].is_null()) {
        (bot->guild_info.channels).push_back(parse_channel(roles[idx]));
      }
      send_payload({discord::REQUEST_GUILD_MEMBERS});
    } catch(const std::exception &e) {
      std::cout << __FILE__ << __LINE__ << ": " << e.what() << std::endl;
    }
  }

  EVENT_HANDLER(GUILD_UPDATE){}
  EVENT_HANDLER(GUILD_DELETE){}
  EVENT_HANDLER(GUILD_BAN_ADD){}
  EVENT_HANDLER(GUILD_BAN_REMOVE){}
  EVENT_HANDLER(GUILD_EMOJIS_UPDATE){}
  EVENT_HANDLER(GUILD_INTEGRATION_UPDATE){}
  EVENT_HANDLER(GUILD_MEMBER_ADD){}
  EVENT_HANDLER(GUILD_MEMBER_REMOVE){}

  EVENT_HANDLER(GUILD_MEMBER_UPDATE){}

  /**
   * @brief: handle guild member chunk event
   *
   * Chunk provides information on all of the members in the guild. Use this to populate the guild
   * member map. This event provides a list of member objects which will be ignored. The only
   * relevant information here is the user object.
   */
  EVENT_HANDLER(GUILD_MEMBER_CHUNK) {
    try {
      // make sure that we have the correct guild
      assert(std::stoul(msg.d["id"].get<std::string>()) == bot->guild_info.id);
      auto members = msg.d["members"];
      size_t idx = 0;
      while(!members[idx++].is_null()) {
        discord::user new_user = parse_user(members[idx]["user"]);
        (bot->guild_info.users)[new_user.id] = new_user;
      }
    } catch(const std::exception &e) {
      std::cout << __FILE__ << __LINE__ << ": " << e.what() << std::endl;
    }
  }

  EVENT_HANDLER(GUILD_ROLE_CREATE){}
  EVENT_HANDLER(GUILD_ROLE_UPDATE){}
  EVENT_HANDLER(GUILD_ROLE_DELETE){}

  /**
   * @brief: handle the message create event
   *
   * Checks if the messages is a command and then checks if the user has permissions to issue the
   * command.
   */
  EVENT_HANDLER(MESSAGE_CREATE) {
    std::string content = msg.d["content"].get<std::string>();
    if(content.at(0) == bot->ref) {
      if(bot->whitelist[0].id != msg.d["user"]["id"].get<uint64_t>()) { // check if the user has permission to issue a command
        bot->command_q.push("Permission Denied");
        return;
      }
      content.erase(0,1);
      std::string cmd = content.substr(0, content.find(" ")); // finds first word
      if(cmd == "makegang") {
        tokenizer<> tok(content);
        std::vector<std::string> args;
        for(tokenizer<>::iterator beg=tok.begin(); beg != tok.end(); ++beg) {
          args.push_back(*beg);
        }
      }
      bot->command_q.push(cmd);
    }
  }

  EVENT_HANDLER(MESSAGE_UPDATE){}
  EVENT_HANDLER(MESSAGE_DELETE){}
  EVENT_HANDLER(MESSAGE_DELETE_BULK){}
  EVENT_HANDLER(MESSAGE_REACTION_ADD){}
  EVENT_HANDLER(MESSAGE_REACTION_REMOVE){}
  EVENT_HANDLER(MESSAGE_REACTION_REMOVE_ALL){}

  EVENT_HANDLER(PRESENCE_UPDATE) {
  }

  EVENT_HANDLER(TYPING_START) {}
  EVENT_HANDLER(USER_UPDATE){}
  EVENT_HANDLER(VOICE_STATE_UPDATE){}
  EVENT_HANDLER(VOICE_SERVER_UPDATE){}
  EVENT_HANDLER(WEBHOOKS_UPDATE){}

  GATEWAY_HANDLE(DISPATCH) {
    last_sequence_data++;
    (this->*(this->events[msg.t]))(msg);
  }

  /**
   * @brief: handle a heartbeat request from wss
   *
   * Sends a heartbeat back.
   *
   * @bug: will send twice because other thread will send a heartbeat too
   */
  GATEWAY_HANDLE(HEARTBEAT) {
    send_payload(
        {
          { "op", discord::HEARTBEAT },
          { "d", this->last_sequence_data },
        });
  }

  GATEWAY_HANDLE(IDENTIFY) {}

  GATEWAY_HANDLE(STATUS_UPDATE) {}

  GATEWAY_HANDLE(VOICE_UPDATE) {}

  GATEWAY_HANDLE(RESUME) {}

  GATEWAY_HANDLE(RECONNECT) {}

  GATEWAY_HANDLE(REQUEST_GUILD_MEMBERS) {}

  GATEWAY_HANDLE(INVALID_SESS) {
    bot->status = DISCONNECTED;
    bot->up_to_date = false;
  }

  /**
   * @brief: responds to the HELLO payload
   *
   * The initial HELLO payload provides the heartbeat_interval as well as
   * the gateway server location.
   */
  GATEWAY_HANDLE(HELLO) {
    // this is the first time we've received a HELLO packet
    if(bot->status == NEW) {
      heartbeat_interval = msg.d["heartbeat_interval"].get<int>();
      // the first identify payload is unique
      send_payload(
          {
            {U("d"),{
              { U("token"), bot->token },
              { U("properties"), {
                { U("$os"), U("linux") },
                { U("$browser"), U("Discord") },
                { U("$device"), U("Discord") }}
              },
              { U("compress"), false }}
            },
            { U("op"), discord::IDENTIFY },
          });
    } else if(bot->status == DISCONNECTED) {
      send_payload({
          { "token", bot->token },
          { "session_id", session_id },
          { "seq", last_sequence_data }
        });
    }
  }

  GATEWAY_HANDLE(HEARTBEAT_ACK) {
    heartbeat_lock.lock();
    heartbeat_ticks--;
    heartbeat_lock.unlock();
  }
}
