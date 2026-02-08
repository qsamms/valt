#pragma once

#include <utils/constants.h>

#include <exception>
#include <string>

class ValtException : public std::exception {
   private:
    std::string message;

   public:
    ValtException(const std::string& s) : message(s) {}
    ValtException() : message(ERR_UNKNOWN) {}

    const char* what() const noexcept override { return message.c_str(); }

    const int get_message_size() { return message.size(); }
};

class InvalidCommandException : public ValtException {
   public:
    InvalidCommandException() : ValtException(ERR_INVALID_COMMAND) {}
};

class UnknownCommandException : public ValtException {
   public:
    UnknownCommandException() : ValtException(ERR_UNKNOWN_COMMAND) {}
};

class KeyNotFoundException : public ValtException {
   public:
    KeyNotFoundException() : ValtException(ERR_KEY_NOT_FOUND) {}
};

class QueueExistsException : public ValtException {
   public:
    QueueExistsException() : ValtException(ERR_QUEUE_EXISTS) {}
};

class QueueNotFoundException : public ValtException {
   public:
    QueueNotFoundException() : ValtException(ERR_QUEUE_NOT_FOUND) {}
};
