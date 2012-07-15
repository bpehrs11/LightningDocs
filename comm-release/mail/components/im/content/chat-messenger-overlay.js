/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

var imServices = {};
Components.utils.import("resource:///modules/imServices.jsm", imServices);
imServices = imServices.Services;

Components.utils.import("resource://gre/modules/FileUtils.jsm");

var gBuddyListContextMenu = null;

function buddyListContextMenu(aXulMenu) {
  this.target = aXulMenu.triggerNode;
  this.menu = aXulMenu;
  let localName = this.target.localName;
  this.onContact = localName == "imcontact";
  this.onConv = localName == "imconv";
  this.shouldDisplay = this.onContact || this.onConv;

  let hide = !this.onContact;
  ["context-openconversation", "context-edit-buddy-separator",
    "context-alias", "context-delete"].forEach(function(aId) {
    document.getElementById(aId).hidden = hide;
  });

  document.getElementById("context-close-conversation").hidden = !this.onConv;
  document.getElementById("context-openconversation").disabled =
    !hide && !this.target.canOpenConversation();
}
buddyListContextMenu.prototype = {
  openConversation: function blcm_openConversation() {
    if (this.onContact || this.onConv)
      this.target.openConversation();
  },
  closeConversation: function blcm_closeConversation() {
    if (this.onConv)
      this.target.closeConversation();
  },
  alias: function blcm_alias() {
    if (this.onContact)
      this.target.startAliasing();
  },
  delete: function blcm_delete() {
    if (!this.onContact)
      return;

    let buddy = this.target.contact.preferredBuddy;
    let bundle = document.getElementById("chatBundle");
    let displayName = this.target.displayName;
    let promptTitle = bundle.getFormattedString("buddy.deletePrompt.title",
                                                [displayName]);
    let userName = buddy.userName;
    if (displayName != userName) {
      displayName = bundle.getFormattedString("buddy.deletePrompt.displayName",
                                              [displayName, userName]);
    }
    let proto = buddy.protocol.name; // FIXME build a list
    let promptMessage = bundle.getFormattedString("buddy.deletePrompt.message",
                                                  [displayName, proto]);
    let deleteButton = bundle.getString("buddy.deletePrompt.button");
    let prompts = Services.prompt;
    let flags = prompts.BUTTON_TITLE_IS_STRING * prompts.BUTTON_POS_0 +
                prompts.BUTTON_TITLE_CANCEL * prompts.BUTTON_POS_1 +
                prompts.BUTTON_POS_1_DEFAULT;
    if (prompts.confirmEx(window, promptTitle, promptMessage, flags,
                          deleteButton, null, null, null, {}))
      return;

    this.target.remove();
  }
};

var gChatTab = null;

var chatTabType = {
  name: "chat",
  panelId: "chatTabPanel",
  modes: {
    chat: {
      type: "chat"
    }
  },

  _handleArgs: function(aArgs) {
    if (!aArgs || !("convType" in aArgs) || aArgs.convType != "log")
      return;

    let item = document.getElementById("searchResultConv");
    item.log = aArgs.conv;
    item.hidden = false;
    document.getElementById("contactlistbox").selectedItem = item;
  },
  openTab: function(aTab, aArgs) {
    if (!document.getElementById("conversationsGroup").nextSibling.localName == "imconv") {
      let convs = imServices.conversations.getUIConversations();
      if (convs.length != 0) {
        convs.sort(function(a, b)
                   a.title.toLowerCase().localeCompare(b.title.toLowerCase()));
        for each (let conv in convs)
          chatHandler._addConversation(conv);
      }
    }

    gChatTab = aTab;
    aTab.tabNode.setAttribute("image", "chrome://chat/skin/chat-16.png");
    this._handleArgs(aArgs);
    chatHandler.updateTitle();
  },
  shouldSwitchTo: function(aArgs) {
    if (!gChatTab)
      return -1;
    this._handleArgs(aArgs);
    return document.getElementById("tabmail").tabInfo.indexOf(gChatTab);
  },
  showTab: function(aTab) {
    gChatTab = aTab;
    let list = document.getElementById("contactlistbox");
    let convs = document.getElementById("conversationsGroup");
    if (!list.selectedItem || list.selectedItem == convs) {
      list.selectedItem =
        convs.nextSibling.localName == "imconv" ? convs.nextSibling : convs;
    }
    else
      chatHandler.onListItemSelected();
  },
  closeTab: function(aTab) {
    gChatTab = null;
  },

  supportsCommand: function (aCommand, aTab) false,
  isCommandEnabled: function (aCommand, aTab) false,
  doCommand: function(aCommand, aTab) { },
  onEvent: function(aEvent, aTab) { },

  saveTabState: function(aTab) { }
};

var chatHandler = {
  _addConversation: function(aConv) {
    let list = document.getElementById("contactlistbox");
    let convs = document.getElementById("conversationsGroup");
    let selectedItem = list.selectedItem;
    let shouldSelect =
      gChatTab && gChatTab.tabNode.selected &&
      (!selectedItem || (selectedItem == convs &&
                        convs.nextSibling.localName != "imconv"));
    let elt = convs.addContact(aConv, "imconv");
    if (shouldSelect)
      list.selectedItem = elt;
    return elt;
  },

  updateTitle: function() {
    let list = document.getElementById("conversationsGroup");
    let unreadCount = 0;
    for (let node = list.nextSibling; node.localName == "imconv"; node = node.nextSibling) {
      let conv = node.conv;
      if (!conv)
        continue;
      if (conv.isChat)
        unreadCount += conv.unreadTargetedMessageCount;
      else
        unreadCount += conv.unreadIncomingMessageCount;
    }
    
    let title =
      document.getElementById("chatBundle").getString("chatTabTitle");
    if (unreadCount)
      title += " (" + unreadCount + ")";
    let selectedItem = document.getElementById("contactlistbox").selectedItem;
    if (selectedItem && selectedItem.localName == "imconv" &&
        !selectedItem.hidden)
      title += " - " + selectedItem.getAttribute("displayname");
    gChatTab.title = title;
    document.getElementById("tabmail").setTabTitle(gChatTab);
  },

  onConvResize: function() {
    let convDeck = document.getElementById("conversationsDeck");
    let panel = convDeck.selectedPanel;
    if (panel && panel.localName == "imconversation")
      panel.onConvResize();
  },

  setStatusMenupopupCommand: function(aEvent) {
    let target = aEvent.originalTarget;
    if (target.getAttribute("id") == "imStatusShowAccounts") {
      openIMAccountMgr();
      return;
    }

    let status = target.getAttribute("status");
    if (!status)
      return; // Can status really be null? Maybe because of an add-on...

    let us = imServices.core.globalUserStatus;
    us.setStatus(Status.toFlag(status), us.statusText);
  },

  _pendingLogBrowserLoad: false,
  _showLog: function(aLog) {
    document.getElementById("conversationsDeck").selectedPanel =
      document.getElementById("logDisplay");
    document.getElementById("logDisplayDeck").selectedPanel =
      document.getElementById("logDisplayBrowserBox");
    let conv = aLog.getConversation();
    let browser = document.getElementById("conv-log-browser");
    if (this._pendingLogBrowserLoad) {
      browser._conv = conv;
      return conv;
    }
    browser.init(conv);
    this._pendingLogBrowserLoad = true;
    Services.obs.addObserver(this, "conversation-loaded", false);
    return conv;
  },
  _makeFriendlyDate: function(aDate) {
    let dts = Components.classes["@mozilla.org/intl/scriptabledateformat;1"]
                        .getService(Components.interfaces.nsIScriptableDateFormat);

    // Figure out when today begins
    let now = new Date();
    let today = new Date(now.getFullYear(), now.getMonth(),
                         now.getDate());

    // Figure out if the end time is from today, yesterday,
    // this week, etc.
    let kDayInMsecs = 24 * 60 * 60 * 1000;
    let time = dts.FormatTime("", dts.timeFormatNoSeconds,
                              aDate.getHours(), aDate.getMinutes(),0);
    let bundle = document.getElementById("chatBundle");
    if (aDate >= today)
      return bundle.getFormattedString("today", [time]);
    if (today - aDate < kDayInMsecs)
      return bundle.getFormattedString("yesterday", [time]);

    let date = dts.FormatDate("", dts.dateFormatShort, aDate.getFullYear(),
                              aDate.getMonth() + 1, aDate.getDate());
    return bundle.getFormattedString("dateTime", [date, time]);
  },

  _showLogList: function(aLogs) {
    let listbox = document.getElementById("logList");
    while (listbox.firstChild)
      listbox.removeChild(listbox.firstChild);
    let logs = [];
    for (let log in fixIterator(aLogs))
      logs.push(log);
    logs.sort(function(log1, log2) log2.time - log1.time);
    for each (let log in logs) {
      let elt = document.createElement("listitem");
      elt.setAttribute("label",
                       this._makeFriendlyDate(new Date(log.time * 1000)));
      elt.log = log;
      listbox.appendChild(elt);
    }
    return logs;
  },
  onLogSelect: function() {
    let selectedItem = document.getElementById("logList").selectedItem;
    if (!selectedItem)
      return;
    let log = selectedItem.log;
    if (!log) {
      let item = document.getElementById("contactlistbox").selectedItem;
      if (item) {
        document.getElementById("conversationsDeck").selectedPanel =
          item.convView;
      }
      return;
    }

    let list = document.getElementById("contactlistbox");
    if (list.selectedItem.getAttribute("id") != "searchResultConv")
      document.getElementById("goToConversation").hidden = false;
    this._showLog(log);
  },

  _contactObserver: {
    observe: function(aSubject, aTopic, aData) {
      if (aTopic == "contact-status-changed" ||
          aTopic == "contact-display-name-changed" ||
          aTopic == "contact-icon-changed")
        chatHandler.showContactInfo(aSubject);
    }
  },
  _observedContact: null,
  get observedContact() this._observedContact,
  set observedContact(aContact) {
    if (aContact == this._observedContact)
      return;
    if (this._observedContact) {
      this._observedContact.removeObserver(this._contactObserver);
      delete this._observedContact;
    }
    this._observedContact = aContact;
    if (aContact)
      aContact.addObserver(this._contactObserver);
    return aContact;
  },
  showCurrentConversation: function() {
    let item = document.getElementById("contactlistbox").selectedItem;
    if (!item)
      return;
    if (item.localName == "imconv")
      document.getElementById("conversationsDeck").selectedPanel = item.convView;
    else if (item.localName == "imcontact")
      item.openConversation();
  },
  showContactInfo: function(aContact) {
    let cti = document.getElementById("conv-top-info");
    cti.setAttribute("userIcon", aContact.buddyIconFilename);
    cti.setAttribute("displayName", aContact.displayName);
    let statusText = aContact.statusText;
    let statusType = aContact.statusType;
    if (statusText)
      statusText = " - " + statusText;
    cti.setAttribute("statusMessageWithDash", statusText);
    let statusString = Status.toLabel(statusType);
    cti.setAttribute("statusMessage", statusString + statusText);
    cti.setAttribute("status", Status.toAttribute(statusType));
    cti.setAttribute("statusTypeTooltiptext", statusString);
    cti.removeAttribute("typing");
    cti.removeAttribute("typed");

    let bundle = document.getElementById("chatBundle");
    let button = document.getElementById("goToConversation");
    button.label = bundle.getFormattedString("startAConversationWith.button",
                                             [aContact.displayName]);
    button.disabled = !aContact.canSendMessage;
  },
  _hideContextPane: function(aHide) {
    document.getElementById("contextSplitter").hidden = aHide;
    document.getElementById("contextPane").hidden = aHide;
  },
  onListItemSelected: function() {
    let item = document.getElementById("contactlistbox").selectedItem;
    if (!item || item.hidden || item.localName == "imgroup") {
      this._hideContextPane(true);
      document.getElementById("conversationsDeck").selectedPanel =
        document.getElementById("noConvScreen");
      this.updateTitle();
      this.observedContact = null;
      return;
    }
    if (item.getAttribute("id") == "searchResultConv") {
      let path = "logs/" + item.log.path;
      let file = FileUtils.getFile("ProfD", path.split("/"));
      let log = imServices.logs.getLogFromFile(file);
      let button = document.getElementById("goToConversation");
      button.hidden = true;
      let conv = this._showLog(log);

      let cti = document.getElementById("conv-top-info");
      cti.setAttribute("displayName", conv.title);
      cti.removeAttribute("userIcon");
      cti.removeAttribute("statusMessageWithDash");
      cti.removeAttribute("statusMessage");
      cti.removeAttribute("status");
      cti.removeAttribute("statusTypeTooltiptext");

      let logs = this._showLogList(imServices.logs.getSimilarLogs(log));
      let time = log.time;
      let list = document.getElementById("logList");
      let logItem = list.firstChild;
      while (logItem) {
        if (logItem.log.time == time) {
          list.selectedItem = logItem;
          break;
        }
        logItem = logItem.nextSibling;
      }
      this.observedContact = null;
    }
    else if (item.localName == "imconv") {
      let convDeck = document.getElementById("conversationsDeck");
      if (!item.convView) {
        let conv = document.createElement("imconversation");
        convDeck.appendChild(conv);
        conv.conv = item.conv;
        conv.tab = item;
        conv.setAttribute("contentcontextmenu", "chatConversationContextMenu");
        item.convView = conv;
        document.getElementById("contextSplitter").hidden = false;
        document.getElementById("contextPane").hidden = false;
      }
      else
        item.convView.onConvResize();
      convDeck.selectedPanel = item.convView;
      item.convView.updateConvStatus();
      item.update();

      this._showLogList(imServices.logs.getLogsForConversation(item.conv));
      let contextPane = document.getElementById("contextPane");
      if (item.conv.isChat) {
        contextPane.setAttribute("chat", "true");
        item.convView.showParticipants();        
      }
      else
        contextPane.removeAttribute("chat");

      let button = document.getElementById("goToConversation");
      let bundle = document.getElementById("chatBundle");
      button.label = bundle.getString("goBackToCurrentConversation.button");
      button.disabled = false;
      this.observedContact = null;
    }
    else if (item.localName == "imcontact") {
      let contact = item.contact;
      if (this.observedContact && contact &&
          this.observedContact.id == contact.id)
        return; // onselect has just been fired again because a status
                // change caused the imcontact to move.
                // Return early to avoid flickering and changing the selected log.

      this.showContactInfo(contact);
      this.observedContact = contact;

      document.getElementById("contextPane").removeAttribute("chat");

      let logs = this._showLogList(imServices.logs.getLogsForContact(contact));
      let listbox = document.getElementById("logList");
      if (logs.length)
        listbox.selectedItem = listbox.firstChild;
      else {
        document.getElementById("conversationsDeck").selectedPanel =
          document.getElementById("logDisplay");
        document.getElementById("logDisplayDeck").selectedPanel =
          document.getElementById("noPreviousConvScreen");
      }
    }
    this._hideContextPane(false);
    this.updateTitle();
  },

  onNickClick: function(aEvent) {
    // Open a private conversation only for a middle or double click.
    if (aEvent.button != 1 && (aEvent.button != 0 || aEvent.detail != 2))
      return;

    let conv = document.getElementById("contactlistbox").selectedItem.conv;
    let nick = aEvent.originalTarget.chatBuddy.name;
    let name = conv.target.getNormalizedChatBuddyName(nick);
    try {
      conv.account.createConversation(name);
    } catch (e) {}
  },

  onNicklistKeyPress: function(aEvent) {
    if (aEvent.keyCode != aEvent.DOM_VK_RETURN &&
        aEvent.keyCode != aEvent.DOM_VK_ENTER)
      return;

    let listbox = aEvent.originalTarget;
    if (listbox.selectedCount == 0)
      return;

    let conv = document.getElementById("contactlistbox").selectedItem.conv;
    for (let i = 0; i < listbox.selectedCount; ++i) {
      let nick = listbox.getSelectedItem(i).chatBuddy.name;
      let name = conv.target.getNormalizedChatBuddyName(nick);
      try {
        conv.account.createConversation(name);
      } catch (e) {}
    }
  },

  _openDialog: function(aType) {
    let features = "chrome,modal,titlebar,centerscreen";
    window.openDialog("chrome://messenger/content/chat/" + aType + ".xul", "",
                      features);
  },
  addBuddy: function() {
     this._openDialog("addbuddy");
  },
  joinChat: function() {
    this._openDialog("joinchat");
  },

  _colorCache: {},
  // Duplicated code from imconversation.xml :-(
  _computeColor: function(aName) {
    if (Object.prototype.hasOwnProperty.call(this._colorCache, aName))
      return this._colorCache[aName];

    // Compute the color based on the nick
    var nick = aName.match(/[a-zA-Z0-9]+/);
    nick = nick ? nick[0].toLowerCase() : nick = aName;
    // We compute a hue value (between 0 and 359) based on the
    // characters of the nick.
    // The first character weights kInitialWeight, each following
    // character weights kWeightReductionPerChar * the weight of the
    // previous character.
    const kInitialWeight = 10; // 10 = 360 hue values / 36 possible characters.
    const kWeightReductionPerChar = 0.52; // arbitrary value
    var weight = kInitialWeight;
    var res = 0;
    for (var i = 0; i < nick.length; ++i) {
      var char = nick.charCodeAt(i) - 47;
      if (char > 10)
        char -= 39;
      // now char contains a value between 1 and 36
      res += char * weight;
      weight *= kWeightReductionPerChar;
    }
    return (this._colorCache[aName] = Math.round(res) % 360);
  },

  _updateNoConvPlaceHolder: function() {
    let connected = false;
    let hasAccount = false;
    let canJoinChat = false;
    for (let account in fixIterator(imServices.accounts.getAccounts())) {
      hasAccount = true;
      if (account.connected) {
        connected = true;
        if (account.canJoinChat) {
          canJoinChat = true;
          break;
        }
      }
    }
    document.getElementById("noConvInnerBox").hidden = !connected;
    document.getElementById("noAccountInnerBox").hidden = hasAccount;
    document.getElementById("noConnectedAccountInnerBox").hidden =
      connected || !hasAccount;
    document.getElementById("button-add-buddy").disabled = !connected;
    document.getElementById("button-join-chat").disabled = !canJoinChat;
  },
  observe: function(aSubject, aTopic, aData) {
    if (aTopic == "browser-request") {
      imServices.ww.openWindow(null,
                               "chrome://chat/content/browserRequest.xul",
                               null, "chrome", aSubject);
      return;
    }

    if (aTopic == "conversation-loaded") {
      let browser = document.getElementById("conv-log-browser");
      if (aSubject != browser)
        return;

      for each (let msg in browser._conv.getMessages()) {
        if (!msg.system)
          msg.color = "color: hsl(" + this._computeColor(msg.who) + ", 100%, 40%);";
        browser.appendMessage(msg);
      }

      delete this._pendingLogBrowserLoad;
      Services.obs.removeObserver(this, "conversation-loaded");
      return;      
    }

    if (aTopic == "account-connected" || aTopic == "account-disconnected" ||
        aTopic == "account-added" || aTopic == "account-removed") {
      this._updateNoConvPlaceHolder();
      return;
    }

    if (aTopic == "contact-signed-on") {
      document.getElementById("onlinecontactsGroup").addContact(aSubject);
      document.getElementById("offlinecontactsGroup").removeContact(aSubject);
      return;
    }
    if (aTopic == "contact-signed-off") {
      document.getElementById("offlinecontactsGroup").addContact(aSubject);
      document.getElementById("onlinecontactsGroup").removeContact(aSubject);
      return;
    }
    if (aTopic == "contact-added") {
      let groupName = (aSubject.online ? "on" : "off") + "linecontactsGroup";
      document.getElementById(groupName).addContact(aSubject);
      return;
    }
    if (aTopic == "contact-removed") {
      let groupName = (aSubject.online ? "on" : "off") + "linecontactsGroup";
      document.getElementById(groupName).removeContact(aSubject);
      return;
    }
    if (aTopic == "contact-no-longer-dummy") {
      let oldId = parseInt(aData);
      let groupName = (aSubject.online ? "on" : "off") + "linecontactsGroup";
      let group = document.getElementById(groupName);
      if (group.contactsById.hasOwnProperty(oldId)) {
        let contact = group.contactsById[oldId];
        delete group.contactsById[oldId];
        group.contactsById[contact.contact.id] = contact;
      }
      return;
    }
    if (aTopic == "new-ui-conversation") {
      if (!gChatTab) {
        let tabmail = document.getElementById("tabmail");
        tabmail.openTab("chat", {background: true, convType: "new",
                                 conv: aSubject});
      }
      let conv = chatHandler._addConversation(aSubject);
      if (!aSubject.isChat && aSubject.buddy) {
        let contact = aSubject.buddy.buddy.contact;
        conv.imContact = contact;
        let groupName = (contact.online ? "on" : "off") + "linecontactsGroup";
        let item = document.getElementById(groupName).removeContact(contact);
        let list = document.getElementById("contactlistbox");
        if (list.selectedItem == item)
          list.selectedItem = conv;
      }
      return;
    }
    if (aTopic == "ui-conversation-closed") {
      let conv =
        document.getElementById("conversationsGroup").removeContact(aSubject);
      if (conv.imContact) {
        let contact = conv.imContact;
        let groupName = (contact.online ? "on" : "off") + "linecontactsGroup";
        document.getElementById(groupName).addContact(contact);
      }
      return;
    }

    if (aTopic == "buddy-authorization-request") {
      aSubject.QueryInterface(Ci.prplIBuddyRequest);
      let bundle = document.getElementById("chatBundle");
      let label = bundle.getFormattedString("buddy.authRequest.label",
                                            [aSubject.userName]);
      let value =
        "buddy-auth-request-" + aSubject.account.id + aSubject.userName;
      let acceptButton = {
        accessKey: bundle.getString("buddy.authRequest.allow.accesskey"),
        label: bundle.getString("buddy.authRequest.allow.label"),
        callback: function() { aSubject.grant(); }
      };
      let denyButton = {
        accessKey: bundle.getString("buddy.authRequest.deny.accesskey"),
        label: bundle.getString("buddy.authRequest.deny.label"),
        callback: function() { aSubject.deny(); }
      };
      let box = document.getElementById("chatTabPanel");
      box.appendNotification(label, value, null, box.PRIORITY_INFO_HIGH,
                            [acceptButton, denyButton]);
      if (!gChatTab) {
        let tabmail = document.getElementById("tabmail");
        tabmail.openTab("chat", {background: true});
      }
      return;
    }
    if (aTopic == "buddy-authorization-request-canceled") {
      aSubject.QueryInterface(Ci.prplIBuddyRequest);
      let value =
        "buddy-auth-request-" + aSubject.account.id + aSubject.userName;
      let notification =
        document.getElementById("chatTabPanel")
                .getNotificationWithValue(value);
      if (notification)
        notification.close();
      return;
    }
  },
  initContactList: function() {
    let group = document.getElementById("offlinecontactsGroup");
    imServices.tags.getTags().forEach(function (aTag) {
      aTag.getContacts().forEach(function(aContact) {
        group.addContact(aContact);
      });
    });
    group._updateGroupLabel();
    imServices.obs.addObserver(this, "new-ui-conversation", false);
    imServices.obs.addObserver(this, "ui-conversation-closed", false);
    imServices.obs.addObserver(this, "contact-signed-on", false);
    imServices.obs.addObserver(this, "contact-signed-off", false);
    imServices.obs.addObserver(this, "contact-added", false);
    imServices.obs.addObserver(this, "contact-removed", false);
    imServices.obs.addObserver(this, "contact-no-longer-dummy", false);
    imServices.obs.addObserver(this, "account-connected", false);
    imServices.obs.addObserver(this, "account-disconnected", false);
    imServices.obs.addObserver(this, "account-added", false);
    imServices.obs.addObserver(this, "account-removed", false);
  },
  init: function() {
    if (!Services.prefs.getBoolPref("mail.chat.enabled")) {
      ["button-chat", "menu_goChat", "goChatSeparator",
       "imAccountsStatus", "newIMAccountMenuItem"].forEach(function(aId) {
         let elt = document.getElementById(aId);
         if (elt)
           elt.hidden = true;
       });
      document.getElementById("key_goChat").disabled = true;
      return;
    }

    Components.utils.import("resource:///modules/index_im.js");

    // initialize the customizeDone method on the customizeable toolbar
    var toolbox = document.getElementById("chat-view-toolbox");
    toolbox.customizeDone = function(aEvent) {
      MailToolboxCustomizeDone(aEvent, "CustomizeChatToolbar");
    };

    let tabmail = document.getElementById("tabmail");
    tabmail.registerTabType(chatTabType);
    imServices.obs.addObserver(this, "browser-request", false);
    imServices.obs.addObserver(this, "buddy-authorization-request", false);
    imServices.obs.addObserver(this, "buddy-authorization-request-canceled", false);
    let listbox = document.getElementById("contactlistbox");
    listbox.addEventListener("keypress", function(aEvent) {
      let item = listbox.selectedItem;
      if (!item || !item.parentNode) // empty list or item no longer in the list
        return;
      item.keyPress(aEvent);
    });
    listbox.addEventListener("select", this.onListItemSelected.bind(this));
    window.addEventListener("resize", this.onConvResize.bind(this));
    document.getElementById("conversationsGroup").sortComparator =
      function(a, b) a.title.toLowerCase().localeCompare(b.title.toLowerCase());

    // The initialization of the im core may trigger a master password prompt,
    // so wrap it with the async prompter service.
    Components.classes["@mozilla.org/messenger/msgAsyncPrompter;1"]
              .getService(Components.interfaces.nsIMsgAsyncPrompter)
              .queueAsyncAuthPrompt("im", false, {
      onPromptStart: function() {
        imServices.core.init();

        // Find the accounts that exist in the im account service but
        // not in nsMsgAccountManager. They have probably been lost if
        // the user has used an older version of Thunderbird on a
        // profile with IM accounts. See bug 736035.
        let accountsById = {};
        for each (let account in fixIterator(imServices.accounts.getAccounts()))
          accountsById[account.numericId] = account;
        let mgr = Components.classes["@mozilla.org/messenger/account-manager;1"]
                            .getService(Ci.nsIMsgAccountManager);
        for each (let account in fixIterator(mgr.accounts, Ci.nsIMsgAccount)) {
          let incomingServer = account.incomingServer;
          if (!incomingServer || incomingServer.type != "im")
            continue;
          delete accountsById[incomingServer.wrappedJSObject.imAccount.numericId];
        }
        // Let's recreate each of them...
        for each (let account in accountsById) {
          let inServer = mgr.createIncomingServer(account.name,
                                                  account.protocol.id, // hostname
                                                  "im");
          inServer.wrappedJSObject.imAccount = account;
          let acc = mgr.createAccount();
          // Avoid new folder notifications.
          inServer.valid = false;
          acc.incomingServer = inServer;
          inServer.valid = true;
          mgr.notifyServerLoaded(inServer);
        }
        chatHandler.initContactList();
        chatHandler._updateNoConvPlaceHolder();
        statusSelector.init();
        return true;
      },
      onPromptAuthAvailable : function() { },
      onPromptCanceled : function() { }
    });
  }
};

window.addEventListener("load", chatHandler.init.bind(chatHandler));
