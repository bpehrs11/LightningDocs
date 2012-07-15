/* ***** BEGIN LICENSE BLOCK *****
 * Version: MPL 1.1/GPL 2.0/LGPL 2.1
 *
 * The contents of this file are subject to the Mozilla Public License Version
 * 1.1 (the "License"); you may not use this file except in compliance with
 * the License. You may obtain a copy of the License at
 * http://www.mozilla.org/MPL/
 *
 * Software distributed under the License is distributed on an "AS IS" basis,
 * WITHOUT WARRANTY OF ANY KIND, either express or implied. See the License
 * for the specific language governing rights and limitations under the
 * License.
 *
 * The Original Code is the Instantbird messenging client, released
 * 2007.
 *
 * The Initial Developer of the Original Code is
 * Florian QUEZE <florian@instantbird.org>.
 * Portions created by the Initial Developer are Copyright (C) 2007
 * the Initial Developer. All Rights Reserved.
 *
 * Contributor(s):
 *
 * Alternatively, the contents of this file may be used under the terms of
 * either the GNU General Public License Version 2 or later (the "GPL"), or
 * the GNU Lesser General Public License Version 2.1 or later (the "LGPL"),
 * in which case the provisions of the GPL or the LGPL are applicable instead
 * of those above. If you wish to allow use of your version of this file only
 * under the terms of either the GPL or the LGPL, and not to allow others to
 * use your version of this file under the terms of the MPL, indicate your
 * decision by deleting the provisions above and replace them with the notice
 * and other provisions required by the GPL or the LGPL. If you do not delete
 * the provisions above, a recipient may use your version of this file under
 * the terms of any one of the MPL, the GPL or the LGPL.
 *
 * ***** END LICENSE BLOCK ***** */

Cu.import("resource:///modules/imStatusUtils.jsm");

var statusSelector = {
  observe: function ss_observe(aSubject, aTopic, aMsg) {
    if (aTopic == "status-changed")
      this.displayCurrentStatus();
    else if (aTopic == "user-icon-changed")
      this.displayUserIcon();
    else if (aTopic == "user-display-name-changed")
      this.displayUserDisplayName();
  },

  displayUserIcon: function ss_displayUserIcon() {
    let icon = Services.core.globalUserStatus.getUserIcon();
    document.getElementById("userIcon").src = icon ? icon.spec : "";
  },

  displayUserDisplayName: function ss_displayUserDisplayName() {
    let displayName = Services.core.globalUserStatus.displayName;
    let elt = document.getElementById("displayName");
    if (displayName)
      elt.removeAttribute("usingDefault");
    else {
      let bundle = document.getElementById("chatBundle");
      displayName = bundle.getString("displayNameEmptyText");
      elt.setAttribute("usingDefault", displayName);
    }
    elt.setAttribute("value", displayName);
  },

  displayStatusType: function ss_displayStatusType(aStatusType) {
    document.getElementById("statusMessage")
            .setAttribute("statusType", aStatusType);
    let statusString = Status.toLabel(aStatusType);
    let statusTypeIcon = document.getElementById("statusTypeIcon");
    statusTypeIcon.setAttribute("status", aStatusType);
    statusTypeIcon.setAttribute("tooltiptext", statusString);
    return statusString;
  },

  displayCurrentStatus: function ss_displayCurrentStatus() {
    let us = Services.core.globalUserStatus;
    let status = Status.toAttribute(us.statusType);
    let message = status == "offline" ? "" : us.statusText;
    let statusString = this.displayStatusType(status);
    let statusMessage = document.getElementById("statusMessage");
    if (message)
      statusMessage.removeAttribute("usingDefault");
    else {
      statusMessage.setAttribute("usingDefault", statusString);
      message = statusString;
    }
    statusMessage.setAttribute("value", message);
    statusMessage.setAttribute("tooltiptext", message);
  },

  editStatus: function ss_editStatus(aEvent) {
    let status = aEvent.originalTarget.getAttribute("status");
    if (status == "offline")
      Services.core.globalUserStatus.setStatus(Ci.imIStatusInfo.STATUS_OFFLINE, "");
    else if (status)
      this.startEditStatus(status);
  },

  startEditStatus: function ss_startEditStatus(aStatusType) {
    let currentStatusType =
      document.getElementById("statusTypeIcon").getAttribute("status");
    if (aStatusType != currentStatusType) {
      this._statusTypeBeforeEditing = currentStatusType;
      this._statusTypeEditing = aStatusType;
      this.displayStatusType(aStatusType);
    }
    this.statusMessageClick();
  },

  statusMessageClick: function ss_statusMessageClick() {
    let statusType =
      document.getElementById("statusTypeIcon").getAttribute("status");
    if (statusType == "offline")
      return;

    let elt = document.getElementById("statusMessage");
    if (!elt.hasAttribute("editing")) {
      elt.setAttribute("editing", "true");
      elt.addEventListener("keypress", this.statusMessageKeyPress);
      elt.addEventListener("blur", this.statusMessageBlur);
      if (elt.hasAttribute("usingDefault")) {
        if ("_statusTypeBeforeEditing" in this &&
            this._statusTypeBeforeEditing == "offline")
          elt.setAttribute("value", Services.core.globalUserStatus.statusText);
        else
          elt.removeAttribute("value");
      }
      if (!("TextboxSpellChecker" in window))
        Components.utils.import("resource:///modules/imTextboxUtils.jsm");
      TextboxSpellChecker.registerTextbox(elt);
      // force binding attachment by forcing layout
      elt.getBoundingClientRect();
      elt.select();
    }

    this.statusMessageRefreshTimer();
  },

  statusMessageRefreshTimer: function ss_statusMessageRefreshTimer() {
    const timeBeforeAutoValidate = 20 * 1000;
    if ("_stopEditStatusTimeout" in this)
      clearTimeout(this._stopEditStatusTimeout);
    this._stopEditStatusTimeout = setTimeout(this.finishEditStatusMessage,
                                             timeBeforeAutoValidate, true);
  },

  statusMessageBlur: function ss_statusMessageBlur(aEvent) {
    if (aEvent.originalTarget == document.getElementById("statusMessage").inputField)
      statusSelector.finishEditStatusMessage(true);
  },

  statusMessageKeyPress: function ss_statusMessageKeyPress(aEvent) {
    switch (aEvent.keyCode) {
      case aEvent.DOM_VK_RETURN:
      case aEvent.DOM_VK_ENTER:
        statusSelector.finishEditStatusMessage(true);
        break;

      case aEvent.DOM_VK_ESCAPE:
        statusSelector.finishEditStatusMessage(false);
        break;

      default:
        statusSelector.statusMessageRefreshTimer();
    }
  },

  finishEditStatusMessage: function ss_finishEditStatusMessage(aSave) {
    clearTimeout(this._stopEditStatusTimeout);
    delete this._stopEditStatusTimeout;
    let elt = document.getElementById("statusMessage");
    if (aSave) {
      let newStatus = Ci.imIStatusInfo.STATUS_UNKNOWN;
      if ("_statusTypeEditing" in this) {
        let statusType = this._statusTypeEditing;
        if (statusType == "available")
          newStatus = Ci.imIStatusInfo.STATUS_AVAILABLE;
        else if (statusType == "unavailable")
          newStatus = Ci.imIStatusInfo.STATUS_UNAVAILABLE;
        else if (statusType == "offline")
          newStatus = Ci.imIStatusInfo.STATUS_OFFLINE;
        delete this._statusTypeBeforeEditing;
        delete this._statusTypeEditing;
      }
      // apply the new status only if it is different from the current one
      if (newStatus != Ci.imIStatusInfo.STATUS_UNKNOWN ||
          elt.value != elt.getAttribute("value"))
        Services.core.globalUserStatus.setStatus(newStatus, elt.value);
    }
    else if ("_statusTypeBeforeEditing" in this) {
      this.displayStatusType(this._statusTypeBeforeEditing);
      delete this._statusTypeBeforeEditing;
      delete this._statusTypeEditing;
    }

    if (elt.hasAttribute("usingDefault"))
      elt.setAttribute("value", elt.getAttribute("usingDefault"));
    TextboxSpellChecker.unregisterTextbox(elt);
    elt.removeAttribute("editing");
    elt.removeEventListener("keypress", this.statusMessageKeyPress, false);
    elt.removeEventListener("blur", this.statusMessageBlur, false);
  },

  userIconClick: function ss_userIconClick() {
    const nsIFilePicker = Components.interfaces.nsIFilePicker;
    let fp = Components.classes["@mozilla.org/filepicker;1"]
                       .createInstance(nsIFilePicker);
    let bundle = document.getElementById("chatBundle");
    fp.init(window, bundle.getString("userIconFilePickerTitle"),
            nsIFilePicker.modeOpen);
    fp.appendFilters(nsIFilePicker.filterImages);
    if (fp.show() == nsIFilePicker.returnOK)
      Services.core.globalUserStatus.setUserIcon(fp.file);
  },

  displayNameClick: function ss_displayNameClick() {
    let elt = document.getElementById("displayName");
    if (!elt.hasAttribute("editing")) {
      elt.setAttribute("editing", "true");
      if (elt.hasAttribute("usingDefault"))
        elt.removeAttribute("value");
      elt.addEventListener("keypress", this.displayNameKeyPress);
      elt.addEventListener("blur", this.displayNameBlur);
      // force binding attachmant by forcing layout
      elt.getBoundingClientRect();
      elt.select();
    }

    this.displayNameRefreshTimer();
  },

  _stopEditDisplayNameTimeout: 0,
  displayNameRefreshTimer: function ss_displayNameRefreshTimer() {
    const timeBeforeAutoValidate = 20 * 1000;
    clearTimeout(this._stopEditDisplayNameTimeout);
    this._stopEditDisplayNameTimeout =
      setTimeout(this.finishEditDisplayName, timeBeforeAutoValidate, true);
  },

  displayNameBlur: function ss_displayNameBlur(aEvent) {
    if (aEvent.originalTarget == document.getElementById("displayName").inputField)
      statusSelector.finishEditDisplayName(true);
  },

  displayNameKeyPress: function ss_displayNameKeyPress(aEvent) {
    switch (aEvent.keyCode) {
      case aEvent.DOM_VK_RETURN:
      case aEvent.DOM_VK_ENTER:
        statusSelector.finishEditDisplayName(true);
        break;

      case aEvent.DOM_VK_ESCAPE:
        statusSelector.finishEditDisplayName(false);
        break;

      default:
        statusSelector.displayNameRefreshTimer();
    }
  },

  finishEditDisplayName: function ss_finishEditDisplayName(aSave) {
    clearTimeout(this._stopEditDisplayNameTimeout);
    let elt = document.getElementById("displayName");
    // Apply the new display name only if it is different from the current one.
    if (aSave && elt.value != elt.getAttribute("value"))
      Services.core.globalUserStatus.displayName = elt.value;
    else if (elt.hasAttribute("usingDefault"))
      elt.setAttribute("value", elt.getAttribute("usingDefault"));

    elt.removeAttribute("editing");
    elt.removeEventListener("keypress", this.displayNameKeyPress, false);
    elt.removeEventListener("blur", this.displayNameBlur, false);
  },

  init: function ss_load() {
    let events = ["status-changed"];
    statusSelector.displayCurrentStatus();

    if (document.getElementById("displayName")) {
      events.push("user-display-name-changed");
      statusSelector.displayUserDisplayName();
    }

    if (document.getElementById("userIcon")) {
      events.push("user-icon-changed");
      statusSelector.displayUserIcon();
    }

    for each (let event in events)
      Services.obs.addObserver(statusSelector, event, false);
    statusSelector._events = events;

    window.addEventListener("unload", statusSelector.unload);
  },

  unload: function ss_unload() {
    for each (let event in statusSelector._events)
      Services.obs.removeObserver(statusSelector, event);
   }
};
