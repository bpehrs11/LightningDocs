/* -*- Mode: Java; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
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
 * The Original Code is Thunderbird App Idle Manager.
 *
 * The Initial Developer of the Original Code is
 * the Mozilla Foundation.
 * Portions created by the Initial Developer are Copyright (C) 2009
 * the Initial Developer. All Rights Reserved.
 *
 * Contributor(s):
 *   David Bienvenu <bienvenu@nventure.com>
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

const EXPORTED_SYMBOLS = ['appIdleManager'];

const Cc = Components.classes;
const Ci = Components.interfaces;

// This module provides a mechanism to turn window focus and blur events
// into app idle notifications. If we get a blur notification that is not
// followed by a focus notification in less than some small number of seconds,
// then we send a begin app idle notification.
// If we get a focus event, and we're app idle, then we send an end app idle 
// notification.
// The notification topic is "mail:appIdle", the values are "idle", and "back"

var appIdleManager =
{
  _appIdle: false,
  _timerInterval: 5000, // 5 seconds ought to be plenty
  get _timer()
  {
    delete this._timer;
    return this._timer = Cc["@mozilla.org/timer;1"].createInstance(Ci.nsITimer);
  },

  _timerCallback: function()
  {
    appIdleManager._appIdle = true;
    Cc["@mozilla.org/observer-service;1"]
      .getService(Components.interfaces.nsIObserverService).notifyObservers(null, "mail:appIdle", "idle");
    
  },
  
  onBlur: function()
  {
    appIdleManager._timer.initWithCallback(appIdleManager._timerCallback,
                                 appIdleManager._timerInterval,
                                 Ci.nsITimer.TYPE_ONE_SHOT);
   },
  
  onFocus: function()
  {
    appIdleManager._timer.cancel();
    if (appIdleManager._appIdle)
    {
      appIdleManager._appIdle = false;
      Cc["@mozilla.org/observer-service;1"]
        .getService(Components.interfaces.nsIObserverService).notifyObservers(null, "mail:appIdle", "back");
    }
  }
};

