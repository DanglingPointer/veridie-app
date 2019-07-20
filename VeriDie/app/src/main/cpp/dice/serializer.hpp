#ifndef SERIALIZER_HPP
#define SERIALIZER_HPP

#include <memory>
#include <string>
#include <optional>
#include "dice/cast.hpp"

namespace dice {

struct Request
{
   dice::Cast cast;
   std::optional<uint32_t> threshold;
};

struct Response
{
   dice::Cast cast;
   std::optional<size_t> successCount;
};

class ISerializer
{
public:
   virtual ~ISerializer() = default;
   virtual std::string CreateRequest(const dice::Request & request) = 0;
   virtual std::string CreateResponse(const dice::Response & response) = 0;
   virtual dice::Request ParseRequest(const std::string & request) = 0;
   virtual dice::Response ParseResponse(const std::string & response) = 0;
};

std::unique_ptr<dice::ISerializer> CreateXmlSerializer();

}

#endif // SERIALIZER_HPP