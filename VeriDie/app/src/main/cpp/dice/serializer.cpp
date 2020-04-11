#include "dice/serializer.hpp"
#include "utils/xmlparser.hpp"

namespace {

struct DiceTypeToString
{
   std::string operator()(const dice::D4 &) { return "D4"; }
   std::string operator()(const dice::D6 &) { return "D6"; }
   std::string operator()(const dice::D8 &) { return "D8"; }
   std::string operator()(const dice::D10 &) { return "D10"; }
   std::string operator()(const dice::D12 &) { return "D12"; }
   std::string operator()(const dice::D16 &) { return "D16"; }
   std::string operator()(const dice::D20 &) { return "D20"; }
   std::string operator()(const dice::D100 &) { return "D100"; }
};

struct RequestToXml
{
   std::string type;
   RequestToXml(std::string type)
      : type(std::move(type))
   {}
   template <typename D>
   std::unique_ptr<xml::Document<char>> operator()(const std::vector<D> & request)
   {
      auto doc = xml::NewDocument("Request");
      doc->GetRoot().AddAttribute("type", std::move(type));
      doc->GetRoot().AddAttribute("size", std::to_string(request.size()));
      return doc;
   }
};

struct ResponseToXml
{
   std::string type;
   ResponseToXml(std::string type)
      : type(std::move(type))
   {}
   template <typename D>
   std::unique_ptr<xml::Document<char>> operator()(const std::vector<D> & response)
   {
      auto doc = xml::NewDocument("Response");
      doc->GetRoot().AddAttribute("type", std::move(type));
      doc->GetRoot().AddAttribute("size", std::to_string(response.size()));
      for (const auto & val : response) {
         doc->GetRoot().AddChild("Val").SetContent(std::to_string(val));
      }
      return doc;
   }
};

struct FillValues
{
   const std::vector<uint32_t> & values;
   FillValues(const std::vector<uint32_t> & values)
      : values(values)
   {}
   template <typename D>
   void operator()(std::vector<D> & cast)
   {
      for (size_t i = 0; i < values.size(); ++i) {
         cast[i](values[i]);
      }
   }
};

class XmlSerializer : public dice::ISerializer
{
public:
   std::string WriteRequest(const dice::Request & request) override;
   std::string WriteResponse(const dice::Response & response) override;
   dice::Request ParseRequest(const std::string & request) override;
   dice::Response ParseResponse(const std::string & response) override;
};

std::string XmlSerializer::WriteRequest(const dice::Request & request)
{
   std::string type = std::visit(DiceTypeToString{}, request.cast);
   auto doc = std::visit(RequestToXml(std::move(type)), request.cast);
   if (request.threshold) {
      doc->GetRoot().AddAttribute("successFrom", std::to_string(*request.threshold));
   }
   return doc->ToString();
}

std::string XmlSerializer::WriteResponse(const dice::Response & response)
{
   std::string type = std::visit(DiceTypeToString{}, response.cast);
   auto doc = std::visit(ResponseToXml(std::move(type)), response.cast);
   if (response.successCount) {
      doc->GetRoot().AddAttribute("successCount", std::to_string(*response.successCount));
   }
   return doc->ToString();
}

dice::Request XmlSerializer::ParseRequest(const std::string & request)
{
   auto doc = xml::ParseString(request);
   const auto & name = doc->GetRoot().GetName();
   if (name != "Request") {
      throw xml::Exception("Expected Request, received: " + name);
   }
   std::string type = doc->GetRoot().GetAttributeValue("type");
   size_t size = std::stoul(doc->GetRoot().GetAttributeValue("size"));
   std::optional<uint32_t> successFrom;
   try {
      successFrom = std::stoul(doc->GetRoot().GetAttributeValue("successFrom"));
   }
   catch (const xml::Exception &) {
   }
   return dice::Request{dice::MakeCast(type, size), successFrom};
}

dice::Response XmlSerializer::ParseResponse(const std::string & response)
{
   auto doc = xml::ParseString(response);
   const auto & name = doc->GetRoot().GetName();
   if (name != "Response") {
      throw xml::Exception("Expected Response, received: " + name);
   }
   std::string type = doc->GetRoot().GetAttributeValue("type");
   size_t size = std::stoul(doc->GetRoot().GetAttributeValue("size"));
   dice::Cast cast = dice::MakeCast(type, size);

   std::vector<uint32_t> values(size);
   for (size_t i = 0; i < size; ++i) {
      values[i] = std::stoul(doc->GetRoot().GetChild(i).GetContent());
   }
   std::visit(FillValues(values), cast);

   std::optional<size_t> successCount;
   try {
      successCount = std::stoul(doc->GetRoot().GetAttributeValue("successCount"));
   }
   catch (const xml::Exception &) {
   }
   return dice::Response{std::move(cast), successCount};
}
} // namespace

namespace dice {

dice::Cast MakeCast(const std::string & type, size_t size)
{
   dice::Cast result;
   if (type == "D4") {
      result = dice::D4(size);
   } else if (type == "D6") {
      result = dice::D6(size);
   } else if (type == "D8") {
      result = dice::D8(size);
   } else if (type == "D10") {
      result = dice::D10(size);
   } else if (type == "D12") {
      result = dice::D12(size);
   } else if (type == "D16") {
      result = dice::D16(size);
   } else if (type == "D20") {
      result = dice::D20(size);
   } else if (type == "D100") {
      result = dice::D100(size);
   }
   return result;
}

std::unique_ptr<dice::ISerializer> CreateXmlSerializer()
{
   return std::make_unique<XmlSerializer>();
}
} // namespace dice