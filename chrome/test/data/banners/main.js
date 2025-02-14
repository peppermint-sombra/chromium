// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

const Action = {
  VERIFY_APPINSTALLED: "verify_appinstalled",
  VERIFY_PROMPT_APPINSTALLED: "verify_prompt_appinstalled",
  VERIFY_BEFOREINSTALLPROMPT: "verify_beforeinstallprompt",
  CALL_PROMPT_DELAYED: "call_prompt_delayed",
  CALL_PROMPT_IN_HANDLER: "call_prompt_in_handler",
  CALL_PROMPT_NO_USERCHOICE: "call_prompt_no_userchoice",
  CANCEL_PROMPT: "cancel_prompt",
  STASH_EVENT: "stash_event",
};

const LISTENER = "listener";
const ATTR = "attr";

// These blanks will get filled in when each event comes through.
let gotEventsFrom = ['_'.repeat(LISTENER.length), '_'.repeat(ATTR.length)];

let stashedEvent = null;

function startWorker(worker) {
  navigator.serviceWorker.register(worker);
}

function verifyEvents(eventName) {
  function setTitle() {
    window.document.title = 'Got ' + eventName + ': ' +
      gotEventsFrom.join(', ');
  }

  window.addEventListener(eventName, () => {
    gotEventsFrom[0] = LISTENER;
    setTitle();
  });
  window['on' + eventName] = () => {
    gotEventsFrom[1] = ATTR;
    setTitle();
  };
}

function callPrompt(event) {
  event.prompt();
  event.userChoice.then(function(choiceResult) {
    window.document.title = 'Got userChoice: ' + choiceResult.outcome;
  });
}

function callStashedPrompt() {
  if (stashedEvent === null) {
      throw new Error('No event was previously stashed');
  }
  stashedEvent.prompt();
}

function addPromptListener(action) {
  window.addEventListener('beforeinstallprompt', function(e) {
    e.preventDefault();

    switch (action) {
      case Action.CALL_PROMPT_DELAYED:
        setTimeout(callPrompt, 0, e);
        break;
      case Action.CALL_PROMPT_IN_HANDLER:
        callPrompt(e);
        break;
      case Action.CALL_PROMPT_NO_USERCHOICE:
        setTimeout(() => e.prompt(), 0);
        break;
      case Action.CANCEL_PROMPT:
        // Navigate the window to trigger the banner cancellation.
        window.location.href = "/";
        break;
      case Action.STASH_EVENT:
        stashedEvent = e;
        break;
    }
  });
}

function addManifestLinkTag() {
  const url = new URL(window.location.href);
  let manifestUrl = url.searchParams.get('manifest');
  if (!manifestUrl) {
    manifestUrl = 'manifest.json';
  }

  var linkTag = document.createElement("link");
  linkTag.id = "manifest";
  linkTag.rel = "manifest";
  linkTag.href = manifestUrl;
  document.head.append(linkTag);
}

function initialize() {
  const url = new URL(window.location.href);
  const action = url.searchParams.get('action');
  if (!action) {
    return;
  }

  switch (action) {
    case Action.VERIFY_APPINSTALLED:
      verifyEvents('appinstalled');
      break;
    case Action.VERIFY_PROMPT_APPINSTALLED:
      addPromptListener(Action.CALL_PROMPT_NO_USERCHOICE);
      verifyEvents('appinstalled');
      break;
    case Action.VERIFY_BEFOREINSTALLPROMPT:
      verifyEvents('beforeinstallprompt');
      break;
    case Action.CALL_PROMPT_DELAYED:
    case Action.CALL_PROMPT_IN_HANDLER:
    case Action.CALL_PROMPT_NO_USERCHOICE:
    case Action.CANCEL_PROMPT:
    case Action.STASH_EVENT:
      addPromptListener(action);
      break;
    default:
      throw new Error("Unrecognised action: " + action);
  }
}

function initializeWithWorker(worker) {
  startWorker(worker);
  initialize();
}

function changeManifestUrl(newManifestUrl) {
  var linkTag = document.getElementById("manifest");
  linkTag.href = newManifestUrl;
}
