// Copyright 2016 the V8 project authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "src/inspector/V8ConsoleAgentImpl.h"

#include "src/inspector/V8ConsoleMessage.h"
#include "src/inspector/V8InspectorImpl.h"
#include "src/inspector/V8InspectorSessionImpl.h"
#include "src/inspector/V8StackTraceImpl.h"
#include "src/inspector/protocol/Protocol.h"

namespace v8_inspector {

namespace ConsoleAgentState {
static const char consoleEnabled[] = "consoleEnabled";
}

V8ConsoleAgentImpl::V8ConsoleAgentImpl(
    V8InspectorSessionImpl* session, protocol::FrontendChannel* frontendChannel,
    protocol::DictionaryValue* state)
    : m_session(session),
      m_state(state),
      m_frontend(frontendChannel),
      m_enabled(false) {}

V8ConsoleAgentImpl::~V8ConsoleAgentImpl() {}

void V8ConsoleAgentImpl::enable(ErrorString* errorString) {
  if (m_enabled) return;
  m_state->setBoolean(ConsoleAgentState::consoleEnabled, true);
  m_enabled = true;
  m_session->inspector()->enableStackCapturingIfNeeded();
  reportAllMessages();
}

void V8ConsoleAgentImpl::disable(ErrorString* errorString) {
  if (!m_enabled) return;
  m_session->inspector()->disableStackCapturingIfNeeded();
  m_state->setBoolean(ConsoleAgentState::consoleEnabled, false);
  m_enabled = false;
}

void V8ConsoleAgentImpl::clearMessages(ErrorString* errorString) {}

void V8ConsoleAgentImpl::restore() {
  if (!m_state->booleanProperty(ConsoleAgentState::consoleEnabled, false))
    return;
  ErrorString ignored;
  enable(&ignored);
}

void V8ConsoleAgentImpl::messageAdded(V8ConsoleMessage* message) {
  if (m_enabled) reportMessage(message, true);
}

bool V8ConsoleAgentImpl::enabled() { return m_enabled; }

void V8ConsoleAgentImpl::reportAllMessages() {
  V8ConsoleMessageStorage* storage =
      m_session->inspector()->ensureConsoleMessageStorage(
          m_session->contextGroupId());
  for (const auto& message : storage->messages()) {
    if (message->origin() == V8MessageOrigin::kConsole)
      reportMessage(message.get(), false);
  }
}

void V8ConsoleAgentImpl::reportMessage(V8ConsoleMessage* message,
                                       bool generatePreview) {
  DCHECK(message->origin() == V8MessageOrigin::kConsole);
  message->reportToFrontend(&m_frontend);
  m_frontend.flush();
}

}  // namespace v8_inspector
