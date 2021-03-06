// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/ui/webui/invalidations_message_handler.h"

#include "base/bind.h"
#include "chrome/browser/invalidation/invalidation_logger.h"
#include "chrome/browser/invalidation/invalidation_service.h"
#include "chrome/browser/invalidation/invalidation_service_factory.h"
#include "chrome/browser/profiles/profile.h"
#include "content/public/browser/web_ui.h"
#include "sync/notifier/invalidation_handler.h"

namespace invalidation {
class InvalidationLogger;
}  // namespace invalidation

namespace syncer {
class ObjectIdInvalidationMap;
}  // namespace syncer

InvalidationsMessageHandler::InvalidationsMessageHandler()
    : logger_(NULL) {}

InvalidationsMessageHandler::~InvalidationsMessageHandler() {
  if (logger_)
    logger_->UnregisterObserver(this);
}

void InvalidationsMessageHandler::RegisterMessages() {
  web_ui()->RegisterMessageCallback(
      "doneLoading",
      base::Bind(&InvalidationsMessageHandler::UIReady,
                 base::Unretained(this)));
}

void InvalidationsMessageHandler::UIReady(const base::ListValue* args) {
  Profile* profile = Profile::FromWebUI(web_ui());
  if (profile) {
    invalidation::InvalidationService* invalidation_service =
        invalidation::InvalidationServiceFactory::GetForProfile(profile);
    if (invalidation_service)
      logger_ = invalidation_service->GetInvalidationLogger();
    if (logger_) {
      if (!logger_->IsObserverRegistered(this))
        logger_->RegisterObserver(this);
    }
  }
  UpdateContent(args);
}

void InvalidationsMessageHandler::UpdateContent(const base::ListValue* args) {
  if (logger_)
    logger_->EmitContent();
}

void InvalidationsMessageHandler::OnUnregistration(
    const base::DictionaryValue& details) {}

void InvalidationsMessageHandler::OnRegistration(
    const base::DictionaryValue& details) {}

void InvalidationsMessageHandler::OnStateChange(
    const syncer::InvalidatorState& new_state) {
  std::string state(syncer::InvalidatorStateToString(new_state));
  web_ui()->CallJavascriptFunction("chrome.invalidations.updateState",
                                   base::StringValue(state));
}

void InvalidationsMessageHandler::OnUpdateIds(
    const std::string& handler_name,
    const syncer::ObjectIdSet& ids_set) {
  scoped_ptr<base::ListValue> list_of_objects = ObjectIdSetToList(ids_set);
  web_ui()->CallJavascriptFunction("chrome.invalidations.updateIds",
                                   base::StringValue(handler_name),
                                   *list_of_objects);
}
void InvalidationsMessageHandler::OnDebugMessage(
    const base::DictionaryValue& details) {}

void InvalidationsMessageHandler::OnInvalidation(
    const syncer::ObjectIdInvalidationMap& new_invalidations) {
  scoped_ptr<base::ListValue> invalidations_list = new_invalidations.ToValue();
  web_ui()->CallJavascriptFunction("chrome.invalidations.logInvalidations",
                                   *invalidations_list);
}

scoped_ptr<base::ListValue> InvalidationsMessageHandler::ObjectIdSetToList(
    const syncer::ObjectIdSet& ids_set) {
  scoped_ptr<base::ListValue> list(new base::ListValue());

  for (syncer::ObjectIdSet::const_iterator it = ids_set.begin();
       it != ids_set.end(); ++it) {
    scoped_ptr<base::DictionaryValue> dic(new base::DictionaryValue);
    dic->SetString("name", it->name());
    dic->SetInteger("source", it->source());
    list->Append(dic.release());
  }
  return list.Pass();
}
