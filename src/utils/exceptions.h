#pragma once

#include <exception>
#include <string>

#include "response_codes.h"

class InvalidCommandException : public std::exception {
   private:
    std::string message;

   public:
    InvalidCommandException(const std::string& s) { message = "ERR: " + s; }
    InvalidCommandException() : message(ERR_INVALID_COMMAND) {}

    const char* what() const noexcept override { return message.c_str(); }

    const int get_message_size() { return message.size(); }
};
