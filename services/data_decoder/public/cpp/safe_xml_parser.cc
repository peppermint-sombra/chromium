// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "services/data_decoder/public/cpp/safe_xml_parser.h"

#include "base/callback.h"
#include "base/macros.h"
#include "base/threading/thread_checker.h"
#include "base/unguessable_token.h"
#include "base/values.h"
#include "services/data_decoder/public/interfaces/constants.mojom.h"
#include "services/data_decoder/public/interfaces/xml_parser.mojom.h"
#include "services/service_manager/public/cpp/connector.h"
#include "services/service_manager/public/mojom/constants.mojom.h"

namespace data_decoder {

namespace {

// Class that does the actual parsing. Deletes itself when parsing is done.
class SafeXmlParser {
 public:
  SafeXmlParser(service_manager::Connector* connector,
                const std::string& unsafe_xml,
                XmlParserCallback callback,
                const std::string& batch_id);
  ~SafeXmlParser();

 private:
  void ReportResults(std::unique_ptr<base::Value> parsed_json,
                     const base::Optional<std::string>& error);

  XmlParserCallback callback_;
  mojom::XmlParserPtr xml_parser_ptr_;
  SEQUENCE_CHECKER(sequence_checker_);

  DISALLOW_COPY_AND_ASSIGN(SafeXmlParser);
};

SafeXmlParser::SafeXmlParser(service_manager::Connector* connector,
                             const std::string& unsafe_xml,
                             XmlParserCallback callback,
                             const std::string& batch_id)
    : callback_(std::move(callback)) {
  DCHECK(callback_);  // Parsing without a callback is useless.

  // If no batch ID has been provided, use a random instance ID to guarantee the
  // connection is to a new service running in its own process.
  service_manager::Identity identity(
      mojom::kServiceName, service_manager::mojom::kInheritUserID,
      batch_id.empty() ? base::UnguessableToken::Create().ToString()
                       : batch_id);
  connector->BindInterface(identity, &xml_parser_ptr_);

  // Unretained(this) is safe as the xml_parser_ptr_ is owned by this class.
  xml_parser_ptr_.set_connection_error_handler(base::BindOnce(
      &SafeXmlParser::ReportResults, base::Unretained(this),
      /*parsed_xml=*/nullptr,
      base::make_optional(
          std::string("Connection error with the XML parser process."))));
  xml_parser_ptr_->Parse(
      unsafe_xml,
      base::BindOnce(&SafeXmlParser::ReportResults, base::Unretained(this)));
}

SafeXmlParser::~SafeXmlParser() = default;

void SafeXmlParser::ReportResults(std::unique_ptr<base::Value> parsed_xml,
                                  const base::Optional<std::string>& error) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);

  std::move(callback_).Run(std::move(parsed_xml), error);

  // This should be the last interaction with this instance, safely delete.
  delete this;
}

}  // namespace

const base::Value* GetXmlElementChildren(const base::Value& element) {
  if (!element.is_dict())
    return nullptr;
  return element.FindKeyOfType(mojom::XmlParser::kChildrenKey,
                               base::Value::Type::LIST);
}

std::string GetXmlQualifiedName(const std::string& name_space,
                                const std::string& name) {
  return name_space.empty() ? name : name_space + ":" + name;
}

void ParseXml(service_manager::Connector* connector,
              const std::string& unsafe_xml,
              XmlParserCallback callback,
              const std::string& batch_id) {
  new SafeXmlParser(connector, unsafe_xml, std::move(callback), batch_id);
}

bool IsXmlElementNamed(const base::Value& element, const std::string& name) {
  if (!element.is_dict())
    return false;
  const base::Value* tag_text = element.FindKeyOfType(
      mojom::XmlParser::kTagKey, base::Value::Type::STRING);
  return tag_text ? tag_text->GetString() == name : false;
}

bool IsXmlElementOfType(const base::Value& element, const std::string& type) {
  if (!element.is_dict())
    return false;
  const base::Value* type_text = element.FindKeyOfType(
      mojom::XmlParser::kTypeKey, base::Value::Type::STRING);
  return type_text ? type_text->GetString() == type : false;
}

bool GetXmlElementTagName(const base::Value& element, std::string* tag_name) {
  DCHECK(tag_name);
  if (!element.is_dict())
    return false;
  const base::Value* tag_text = element.FindKeyOfType(
      mojom::XmlParser::kTagKey, base::Value::Type::STRING);
  if (!tag_text)
    return false;
  *tag_name = tag_text->GetString();
  return true;
}

bool GetXmlElementText(const base::Value& element, std::string* text) {
  DCHECK(text);
  const base::Value* children = GetXmlElementChildren(element);
  if (!children)
    return false;

  const base::Value* text_node = nullptr;
  for (const base::Value& value : children->GetList()) {
    if (IsXmlElementOfType(value, mojom::XmlParser::kTextNodeType) ||
        IsXmlElementOfType(value, mojom::XmlParser::kCDataNodeType)) {
      text_node = &value;
      break;
    }
  }
  if (!text_node)
    return false;

  const base::Value* text_value = text_node->FindKeyOfType(
      mojom::XmlParser::kTextKey, base::Value::Type::STRING);
  ;
  *text = text_value ? text_value->GetString() : "";
  return true;
}

bool GetXmlElementNamespacePrefix(const base::Value& element,
                                  const std::string& namespace_uri,
                                  std::string* prefix) {
  prefix->clear();
  const base::Value* namespaces = element.FindKeyOfType(
      mojom::XmlParser::kNamespacesKey, base::Value::Type::DICTIONARY);
  if (!namespaces)
    return false;

  // The namespaces dictionary is prefix -> URI, so we have to do a reverse
  // lookup.
  for (const auto& item : namespaces->DictItems()) {
    if (item.second.GetString() == namespace_uri) {
      *prefix = item.first;
      return true;
    }
  }
  return false;
}

int GetXmlElementChildrenCount(const base::Value& element,
                               const std::string& name) {
  const base::Value* children = GetXmlElementChildren(element);
  if (!children)
    return 0;
  int child_count = 0;
  for (const base::Value& value : children->GetList()) {
    DCHECK(value.is_dict());
    std::string tag_name;
    bool success = GetXmlElementTagName(value, &tag_name);
    if (success && tag_name == name)
      child_count++;
  }
  return child_count;
}

const base::Value* GetXmlElementChildWithType(const base::Value& element,
                                              const std::string& type) {
  const base::Value* children = GetXmlElementChildren(element);
  if (!children)
    return nullptr;
  for (const base::Value& value : children->GetList()) {
    DCHECK(value.is_dict());
    if (IsXmlElementOfType(value, type)) {
      return &value;
    }
  }
  return nullptr;
}

const base::Value* GetXmlElementChildWithTag(const base::Value& element,
                                             const std::string& tag) {
  const base::Value* children = GetXmlElementChildren(element);
  if (!children)
    return nullptr;
  for (const base::Value& value : children->GetList()) {
    DCHECK(value.is_dict());
    if (IsXmlElementNamed(value, tag))
      return &value;
  }
  return nullptr;
}

bool GetAllXmlElementChildrenWithTag(
    const base::Value& element,
    const std::string& tag,
    std::vector<const base::Value*>* children_out) {
  const base::Value* children = GetXmlElementChildren(element);
  if (!children)
    return false;
  bool found = false;
  for (const base::Value& child : children->GetList()) {
    DCHECK(child.is_dict());
    if (IsXmlElementNamed(child, tag)) {
      found = true;
      children_out->push_back(&child);
    }
  }
  return found;
}

const base::Value* FindXmlElementPath(
    const base::Value& element,
    std::initializer_list<base::StringPiece> path,
    bool* unique_path) {
  const base::Value* cur = nullptr;
  if (unique_path)
    *unique_path = true;

  for (const base::StringPiece component_piece : path) {
    std::string component(component_piece);
    if (!cur) {
      // First element has to match the current node.
      if (!IsXmlElementNamed(element, component))
        return nullptr;
      cur = &element;
      continue;
    }

    const base::Value* new_cur = GetXmlElementChildWithTag(*cur, component);
    if (!new_cur)
      return nullptr;

    if (unique_path && *unique_path &&
        GetXmlElementChildrenCount(*cur, component) > 1)
      *unique_path = false;

    cur = new_cur;
  }
  return cur;
}

std::string GetXmlElementAttribute(const base::Value& element,
                                   const std::string& element_name) {
  if (!element.is_dict())
    return "";

  const base::Value* attributes = element.FindKeyOfType(
      mojom::XmlParser::kAttributesKey, base::Value::Type::DICTIONARY);
  if (!attributes)
    return "";

  const base::Value* value =
      attributes->FindKeyOfType(element_name, base::Value::Type::STRING);
  return value ? value->GetString() : "";
}

}  // namespace data_decoder
