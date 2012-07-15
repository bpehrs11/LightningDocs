/* -*- Mode: Javascript; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/* Version: MPL 1.1/GPL 2.0/LGPL 2.1
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
 * The Original Code is the permission tab for Page Info.
 *
 * The Initial Developer of the Original Code is
 *   Florian QUEZE <f.qu@queze.net>
 * Portions created by the Initial Developer are Copyright (C) 2006-2007
 * the Initial Developer. All Rights Reserved.
 *
 * Contributor(s):
 *   Daniel Brooks <db48x@yahoo.com>
 *
 * Alternatively, the contents of this file may be used under the terms of
 * either the GNU General Public License Version 2 or later (the "GPL"), or
 * the GNU Lesser General Public License Version 2.1 or later (the "LGPL"),
 * in which case the provisions of the GPL or the LGPL are applicable instead
 * of those above. If you wish to allow use of your version of this file only
 * under the terms of either the GPL or the LGPL, and not to allow others to
 * use your version of this file under the terms of the MPL, indicate your
 * decision by deleting the provisions above and replace them with the notice
 * and other provisions required by the LGPL or the GPL. If you do not delete
 * the provisions above, a recipient may use your version of this file under
 * the terms of any one of the MPL, the GPL or the LGPL.
 *
 * ***** END LICENSE BLOCK ***** */

const ALLOW = Services.perms.ALLOW_ACTION;         // 1
const BLOCK = Services.perms.DENY_ACTION;          // 2
const SESSION = nsICookiePermission.ACCESS_SESSION;// 8
var gPermURI;

var gPermObj = {
  image: function getImageDefaultPermission()
  {
    if (Services.prefs.getIntPref("permissions.default.image") == 2)
      return BLOCK;
    return ALLOW;
  },
  cookie: function getCookieDefaultPermission()
  {
    if (Services.prefs.getIntPref("network.cookie.cookieBehavior") == 2)
      return BLOCK;

    if (Services.prefs.getIntPref("network.cookie.lifetimePolicy") == 2)
      return SESSION;
    return ALLOW;
  },
  popup: function getPopupDefaultPermission()
  {
    if (Services.prefs.getBoolPref("dom.disable_open_during_load"))
      return BLOCK;
    return ALLOW;
  },
  install: function getInstallDefaultPermission()
  {
    try {
      if (!Services.prefs.getBoolPref("xpinstall.whitelist.required"))
        return ALLOW;
    }
    catch (e) {
    }
    return BLOCK;
  },
  geo: function getGeoDefaultPermission()
  {
    return BLOCK;
  }
};

var permissionObserver = {
  observe: function (aSubject, aTopic, aData)
  {
    if (aTopic == "perm-changed") {
      var permission = aSubject.QueryInterface(Components.interfaces.nsIPermission);
      if (/^https?/.test(gPermURI.scheme) &&
          permission.host == gPermURI.host && permission.type in gPermObj)
        initRow(permission.type);
    }
  }
};

function initPermission()
{
  onUnloadRegistry.push(onUnloadPermission);
  onResetRegistry.push(onUnloadPermission);
}

function onLoadPermission()
{
  gPermURI = gDocument.documentURIObject;
  if (/^https?/.test(gPermURI.scheme)) {
    var hostText = document.getElementById("hostText");
    hostText.value = gPermURI.host;
    Services.obs.addObserver(permissionObserver, "perm-changed", false);
  }
  for (var i in gPermObj)
    initRow(i);
}

function onUnloadPermission()
{
  if (/^https?/.test(gPermURI.scheme)) {
    Services.obs.removeObserver(permissionObserver, "perm-changed");
  }
}

function initRow(aPartId)
{
  var checkbox = document.getElementById(aPartId + "Def");
  var command  = document.getElementById("cmd_" + aPartId + "Toggle");
  if (!/^https?/.test(gPermURI.scheme)) {
    checkbox.checked = false;
    checkbox.setAttribute("disabled", "true");
    command.setAttribute("disabled", "true");
    document.getElementById(aPartId + "RadioGroup").selectedItem = null;
    return;
  }
  checkbox.removeAttribute("disabled");
  var pm = Services.perms;
  var perm = aPartId == "geo" ? pm.testExactPermission(gPermURI, aPartId) :
                                pm.testPermission(gPermURI, aPartId);
 
  if (perm) {
    checkbox.checked = false;
    command.removeAttribute("disabled");
  }
  else {
    checkbox.checked = true;
    command.setAttribute("disabled", "true");
    perm = gPermObj[aPartId]();
  }
  setRadioState(aPartId, perm);
}

function onCheckboxClick(aPartId)
{
  var command  = document.getElementById("cmd_" + aPartId + "Toggle");
  var checkbox = document.getElementById(aPartId + "Def");
  if (checkbox.checked) {
    Services.perms.remove(gPermURI.host, aPartId);
    command.setAttribute("disabled", "true");
    var perm = gPermObj[aPartId]();
    setRadioState(aPartId, perm);
  }
  else {
    onRadioClick(aPartId);
    command.removeAttribute("disabled");
  }
}

function onRadioClick(aPartId)
{
  var radioGroup = document.getElementById(aPartId + "RadioGroup");
  var id = radioGroup.selectedItem.id;
  var permission = id.split('-')[1];
  Services.perms.add(gPermURI, aPartId, permission);
}

function setRadioState(aPartId, aValue)
{
  var radio = document.getElementById(aPartId + "-" + aValue);
  radio.radioGroup.selectedItem = radio;
}
