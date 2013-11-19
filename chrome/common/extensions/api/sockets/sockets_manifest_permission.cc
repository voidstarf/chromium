// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/common/extensions/api/sockets/sockets_manifest_permission.h"

#include "base/memory/scoped_ptr.h"
#include "base/strings/utf_string_conversions.h"
#include "base/values.h"
#include "chrome/common/extensions/api/manifest_types.h"
#include "chrome/common/extensions/api/sockets/sockets_manifest_data.h"
#include "chrome/common/extensions/extension_messages.h"
#include "extensions/common/error_utils.h"
#include "extensions/common/manifest_constants.h"
#include "grit/generated_resources.h"
#include "ipc/ipc_message.h"
#include "ui/base/l10n/l10n_util.h"

namespace extensions {

namespace sockets_errors {
const char kErrorInvalidHostPattern[] = "Invalid host:port pattern '*'";
}

namespace errors = sockets_errors;
using api::manifest_types::Sockets;
using api::manifest_types::SocketHostPatterns;
using content::SocketPermissionRequest;

namespace  {

static bool ParseHostPattern(
    SocketsManifestPermission* permission,
    content::SocketPermissionRequest::OperationType operation_type,
    const std::string& host_pattern,
    string16* error) {
  SocketPermissionEntry entry;
  if (!SocketPermissionEntry::ParseHostPattern(
        operation_type, host_pattern, &entry)) {
    *error = ErrorUtils::FormatErrorMessageUTF16(
        errors::kErrorInvalidHostPattern, host_pattern);
    return false;
  }
  permission->AddPermission(entry);
  return true;
}

static bool ParseHostPatterns(
    SocketsManifestPermission* permission,
    content::SocketPermissionRequest::OperationType operation_type,
    const scoped_ptr<SocketHostPatterns>& host_patterns,
    string16* error) {
  if (!host_patterns)
    return true;

  if (host_patterns->as_string) {
    return ParseHostPattern(permission, operation_type,
                            *host_patterns->as_string, error);
  }

  CHECK(host_patterns->as_strings);
  for (std::vector<std::string>::const_iterator it =
      host_patterns->as_strings->begin();
      it != host_patterns->as_strings->end(); ++it) {
    if (!ParseHostPattern(permission, operation_type, *it, error)) {
      return false;
    }
  }
  return true;
}

static void SetHostPatterns(
    scoped_ptr<SocketHostPatterns>& host_patterns,
    const SocketsManifestPermission* permission,
    content::SocketPermissionRequest::OperationType operation_type) {
  host_patterns.reset(new SocketHostPatterns());
  host_patterns->as_strings.reset(new std::vector<std::string>());
  for (SocketsManifestPermission::SocketPermissionEntrySet::const_iterator it =
      permission->entries().begin(); it != permission->entries().end() ; ++it) {
    if (it->pattern().type == operation_type) {
      host_patterns->as_strings->push_back(it->GetHostPatternAsString());
      break;
    }
  }
}

static SocketsManifestPermission::PermissionKind HasOperationType(
    const SocketsManifestPermission::SocketPermissionEntrySet& set,
    SocketPermissionRequest::OperationType operation_type,
    SocketsManifestPermission::PermissionKind kind) {
  for (SocketsManifestPermission::SocketPermissionEntrySet::const_iterator
      it = set.begin(); it != set.end() ; ++it) {
    if (it->pattern().type == operation_type)
      return kind;
  }
  return SocketsManifestPermission::kNone;
}

}  // namespace

SocketsManifestPermission::SocketsManifestPermission()
    : kinds_(kNone) {
}

SocketsManifestPermission::~SocketsManifestPermission() {}

// static
scoped_ptr<SocketsManifestPermission> SocketsManifestPermission::FromValue(
    const base::Value& value,
    string16* error) {
  scoped_ptr<Sockets> sockets = Sockets::FromValue(value, error);
  if (!sockets)
    return scoped_ptr<SocketsManifestPermission>();

  scoped_ptr<SocketsManifestPermission> result(new SocketsManifestPermission());
  if (sockets->udp) {
    result->kinds_ |= kUdpPermission;
    if (!ParseHostPatterns(result.get(),
                           SocketPermissionRequest::UDP_BIND,
                           sockets->udp->bind,
                           error)) {
      return scoped_ptr<SocketsManifestPermission>();
    }
    if (!ParseHostPatterns(result.get(),
                           SocketPermissionRequest::UDP_SEND_TO,
                           sockets->udp->send,
                           error)) {
      return scoped_ptr<SocketsManifestPermission>();
    }
    if (!ParseHostPatterns(result.get(),
                           SocketPermissionRequest::UDP_MULTICAST_MEMBERSHIP,
                           sockets->udp->multicast_membership,
                           error)) {
      return scoped_ptr<SocketsManifestPermission>();
    }
  }
  if (sockets->tcp) {
    result->kinds_ |= kTcpPermission;
    if (!ParseHostPatterns(result.get(),
                           SocketPermissionRequest::TCP_CONNECT,
                           sockets->tcp->connect,
                           error)) {
      return scoped_ptr<SocketsManifestPermission>();
    }
  }
  if (sockets->tcp_server) {
    result->kinds_ |= kTcpServerPermission;
    if (!ParseHostPatterns(result.get(),
                           SocketPermissionRequest::TCP_LISTEN,
                           sockets->tcp_server->listen,
                           error)) {
      return scoped_ptr<SocketsManifestPermission>();
    }
  }
  return result.Pass();
}

bool SocketsManifestPermission::CheckRequest(
    const Extension* extension,
    const SocketPermissionRequest& request) const {
  for (SocketPermissionEntrySet::const_iterator it = permissions_.begin();
       it != permissions_.end(); ++it) {
    if (it->Check(request))
      return true;
  }
  return false;
}

std::string SocketsManifestPermission::name() const {
  return manifest_keys::kSockets;
}

std::string SocketsManifestPermission::id() const {
  return name();
}

bool SocketsManifestPermission::HasMessages() const {
  bool is_empty = permissions_.empty() && (kinds_ == kNone);
  return !is_empty;
}

PermissionMessages SocketsManifestPermission::GetMessages() const {
  // TODO(rpaquay): This function and callees is (almost) a copy/paste
  // from extensions::SocketPermissiona.
  PermissionMessages result;
  if (!AddAnyHostMessage(result)) {
    AddSpecificHostMessage(result);
    AddSubdomainHostMessage(result);
  }
  AddNetworkListMessage(result);
  return result;
}

bool SocketsManifestPermission::FromValue(const base::Value* value) {
  if (!value)
    return false;
  string16 error;
  scoped_ptr<SocketsManifestPermission> data(
      SocketsManifestPermission::FromValue(*value, &error));

  if (!data)
    return false;

  permissions_ = data->permissions_;
  kinds_ = data->kinds_;
  return true;
}

scoped_ptr<base::Value> SocketsManifestPermission::ToValue() const {
  Sockets sockets;
  if (has_udp()) {
    sockets.udp.reset(new Sockets::Udp());
    SetHostPatterns(sockets.udp->bind, this,
                    SocketPermissionRequest::UDP_BIND);
    SetHostPatterns(sockets.udp->send, this,
                    SocketPermissionRequest::UDP_SEND_TO);
    SetHostPatterns(sockets.udp->multicast_membership, this,
                    SocketPermissionRequest::UDP_MULTICAST_MEMBERSHIP);
  }
  if (has_tcp()) {
    sockets.tcp.reset(new Sockets::Tcp());
    SetHostPatterns(sockets.tcp->connect, this,
                    SocketPermissionRequest::TCP_CONNECT);
  }
  if (has_tcp_server()) {
    sockets.tcp_server.reset(new Sockets::TcpServer());
    SetHostPatterns(sockets.tcp_server->listen, this,
                    SocketPermissionRequest::TCP_LISTEN);
  }

  return scoped_ptr<base::Value>(sockets.ToValue().release()).Pass();
}

ManifestPermission* SocketsManifestPermission::Clone() const {
  scoped_ptr<SocketsManifestPermission> result(new SocketsManifestPermission());
  result->permissions_ = permissions_;
  result->kinds_ = kinds_;
  return result.release();
}

ManifestPermission* SocketsManifestPermission::Diff(
    const ManifestPermission* rhs) const {
  const SocketsManifestPermission* other =
      static_cast<const SocketsManifestPermission*>(rhs);

  scoped_ptr<SocketsManifestPermission> data(new SocketsManifestPermission());
  std::set_difference(
      permissions_.begin(), permissions_.end(),
      other->permissions_.begin(), other->permissions_.end(),
      std::inserter<SocketPermissionEntrySet>(
          data->permissions_, data->permissions_.begin()));

  data->kinds_ = (kinds_ & (~other->kinds_));

  // Note: We may need to fix up |kinds_| because any permission entry
  // in a given group (udp, tcp, etc.) implies the corresponding kind bit set.
  data->kinds_ |= HasOperationType(data->permissions_,
      SocketPermissionRequest::UDP_BIND, kUdpPermission);
  data->kinds_ |= HasOperationType(data->permissions_,
      SocketPermissionRequest::UDP_SEND_TO, kUdpPermission);
  data->kinds_ |= HasOperationType(data->permissions_,
      SocketPermissionRequest::UDP_MULTICAST_MEMBERSHIP, kUdpPermission);
  data->kinds_ |= HasOperationType(data->permissions_,
      SocketPermissionRequest::TCP_CONNECT, kTcpPermission);
  data->kinds_ |= HasOperationType(data->permissions_,
      SocketPermissionRequest::TCP_LISTEN, kTcpServerPermission);
  return data.release();
}

ManifestPermission* SocketsManifestPermission::Union(
    const ManifestPermission* rhs) const {
  const SocketsManifestPermission* other =
      static_cast<const SocketsManifestPermission*>(rhs);

  scoped_ptr<SocketsManifestPermission> data(new SocketsManifestPermission());
  std::set_union(
      permissions_.begin(), permissions_.end(),
      other->permissions_.begin(), other->permissions_.end(),
      std::inserter<SocketPermissionEntrySet>(
          data->permissions_, data->permissions_.begin()));

  data->kinds_ = (kinds_ | other->kinds_);
  return data.release();
}

ManifestPermission* SocketsManifestPermission::Intersect(
    const ManifestPermission* rhs) const {
  const SocketsManifestPermission* other =
      static_cast<const SocketsManifestPermission*>(rhs);

  scoped_ptr<SocketsManifestPermission> data(new SocketsManifestPermission());
  std::set_intersection(
      permissions_.begin(), permissions_.end(),
      other->permissions_.begin(), other->permissions_.end(),
      std::inserter<SocketPermissionEntrySet>(
          data->permissions_, data->permissions_.begin()));

  data->kinds_ = (kinds_ & other->kinds_);
  return data.release();
}

bool SocketsManifestPermission::Contains(const ManifestPermission* rhs) const {
  const SocketsManifestPermission* other =
      static_cast<const SocketsManifestPermission*>(rhs);

  return std::includes(
      permissions_.begin(), permissions_.end(),
      other->permissions_.begin(), other->permissions_.end()) &&
      ((kinds_ | other->kinds_) == kinds_);
}

bool SocketsManifestPermission::Equal(const ManifestPermission* rhs) const {
  const SocketsManifestPermission* other =
      static_cast<const SocketsManifestPermission*>(rhs);

  return (permissions_ == other->permissions_) &&
      (kinds_ == other->kinds_);
}

void SocketsManifestPermission::Write(IPC::Message* m) const {
  IPC::WriteParam(m, permissions_);
  IPC::WriteParam(m, kinds_);
}

bool SocketsManifestPermission::Read(const IPC::Message* m,
                                     PickleIterator* iter) {
  return IPC::ReadParam(m, iter, &permissions_) &&
      IPC::ReadParam(m, iter, &kinds_);
}

void SocketsManifestPermission::Log(std::string* log) const {
  IPC::LogParam(permissions_, log);
  IPC::LogParam(kinds_, log);
}

void SocketsManifestPermission::AddPermission(
    const SocketPermissionEntry& entry) {
  permissions_.insert(entry);
}

bool SocketsManifestPermission::AddAnyHostMessage(
    PermissionMessages& messages) const {
  for (SocketPermissionEntrySet::const_iterator it = permissions_.begin();
      it != permissions_.end(); ++it) {
    if (it->IsAddressBoundType() &&
        it->GetHostType() == SocketPermissionEntry::ANY_HOST) {
      messages.push_back(PermissionMessage(
          PermissionMessage::kSocketAnyHost,
          l10n_util::GetStringUTF16(
              IDS_EXTENSION_PROMPT_WARNING_SOCKET_ANY_HOST)));
      return true;
    }
  }
  return false;
}

void SocketsManifestPermission::AddSubdomainHostMessage(
    PermissionMessages& messages) const {
  std::set<string16> domains;
  for (SocketPermissionEntrySet::const_iterator it = permissions_.begin();
      it != permissions_.end(); ++it) {
    if (it->GetHostType() == SocketPermissionEntry::HOSTS_IN_DOMAINS)
      domains.insert(UTF8ToUTF16(it->pattern().host));
  }
  if (!domains.empty()) {
    int id = (domains.size() == 1) ?
             IDS_EXTENSION_PROMPT_WARNING_SOCKET_HOSTS_IN_DOMAIN :
             IDS_EXTENSION_PROMPT_WARNING_SOCKET_HOSTS_IN_DOMAINS;
    messages.push_back(PermissionMessage(
        PermissionMessage::kSocketDomainHosts,
        l10n_util::GetStringFUTF16(
            id,
            JoinString(
                std::vector<string16>(
                    domains.begin(), domains.end()), ' '))));
  }
}

void SocketsManifestPermission::AddSpecificHostMessage(
    PermissionMessages& messages) const {
  std::set<string16> hostnames;
  for (SocketPermissionEntrySet::const_iterator it = permissions_.begin();
    it != permissions_.end(); ++it) {
    if (it->GetHostType() == SocketPermissionEntry::SPECIFIC_HOSTS)
      hostnames.insert(UTF8ToUTF16(it->pattern().host));
  }
  if (!hostnames.empty()) {
    int id = (hostnames.size() == 1) ?
             IDS_EXTENSION_PROMPT_WARNING_SOCKET_SPECIFIC_HOST :
             IDS_EXTENSION_PROMPT_WARNING_SOCKET_SPECIFIC_HOSTS;
    messages.push_back(PermissionMessage(
        PermissionMessage::kSocketSpecificHosts,
        l10n_util::GetStringFUTF16(
            id,
            JoinString(
                std::vector<string16>(
                    hostnames.begin(), hostnames.end()), ' '))));
  }
}

void SocketsManifestPermission::AddNetworkListMessage(
    PermissionMessages& messages) const {
  for (SocketPermissionEntrySet::const_iterator it = permissions_.begin();
      it != permissions_.end(); ++it) {
    if (it->pattern().type == SocketPermissionRequest::NETWORK_STATE) {
      messages.push_back(PermissionMessage(
          PermissionMessage::kNetworkState,
          l10n_util::GetStringUTF16(
              IDS_EXTENSION_PROMPT_WARNING_NETWORK_STATE)));
    }
  }
}

}  // namespace extensions
