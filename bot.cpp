#include "connection.hpp"
#include "endpoints.hpp"
#include "gateway.hpp"
#include "bot.hpp"

#include <string>
#include <iostream>

/**
 * @brief: bot constructor
 *
 * Constructor for bot.
 *
 * @param[in]: token, the bot's token
 * @param[in]: ref, a reference for the discord users to use to ping the bot
 */
Bot::Bot(std::string token, char ref)
  : status{NEW}, up_to_date{true}, wss_gateway(this, false, backend::JSON), c(this), token{token}, ref{ref} {}

int Bot::run() {
  try {
    c.identify(); // establish the connection
    std::thread wss = std::thread([this](){this->wss_gateway.run();}); // start the gateway handler
    while(status == NEW); // gateway to confirm we are ready
    while(status == ACTIVE) {
      // sleep until notified that there is command waiting
      if(!command_q.empty()) {
        auto task = command_q.front();
        command_q.pop(); // make the commit
        curr_chan = task.channel_id;
        task.todo();
      }
    }
    wss.join();
  } catch(const std::exception& e) {
    std::cout << __FILE__  << __LINE__ << e.what() << std::endl;
  }
  return 0;
}

Bot::~Bot() {
  status = TERMINATE;
  wss_gateway.close();
}

void Bot::create_message(std::string msg) {
  c.create_message(msg);
}
