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
 * The Original Code is mozilla.org code.
 *
 * The Initial Developer of the Original Code is
 * Netscape Communications Corporation.
 * Portions created by the Initial Developer are Copyright (C) 1998
 * the Initial Developer. All Rights Reserved.
 *
 * Contributor(s):
 *   Blake Ross <blakeross@telocity.com>
 *   Peter Annema <disttsc@bart.nl>
 *   Samir Gehani <sgehani@netscape.com>
 *
 * Alternatively, the contents of this file may be used under the terms of
 * either of the GNU General Public License Version 2 or later (the "GPL"),
 * or the GNU Lesser General Public License Version 2.1 or later (the "LGPL"),
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

const nsIWebNavigation = Components.interfaces.nsIWebNavigation;
var gPrintSettingsAreGlobal = true;
var gSavePrintSettings = true;
var gChromeState = null; // chrome state before we went into print preview
var gInPrintPreviewMode = false;
var gNavToolbox = null;

function getWebNavigation()
{
  try {
    return getBrowser().webNavigation;
  } catch (e) {
    return null;
  }
}

function BrowserReloadWithFlags(reloadFlags)
{
  /* First, we'll try to use the session history object to reload so 
   * that framesets are handled properly. If we're in a special 
   * window (such as view-source) that has no session history, fall 
   * back on using the web navigation's reload method.
   */

  var webNav = getWebNavigation();
  try {
    var sh = webNav.sessionHistory;
    if (sh)
      webNav = sh.QueryInterface(Components.interfaces.nsIWebNavigation);
  } catch (e) {
  }

  try {
    webNav.reload(reloadFlags);
  } catch (e) {
  }
}

function toggleAffectedChrome(aHide)
{
  // chrome to toggle includes:
  //   (*) menubar
  //   (*) navigation bar
  //   (*) personal toolbar
  //   (*) tab browser ``strip''
  //   (*) sidebar
  //   (*) statusbar
  //   (*) findbar

  if (!gChromeState)
    gChromeState = new Object;

  var statusbar = document.getElementById("status-bar");
  getNavToolbox().hidden = aHide; 
  var notificationBox = gBrowser.getNotificationBox();
  var findbar = document.getElementById("FindToolbar")

  // sidebar states map as follows:
  //   hidden    => hide/show nothing
  //   collapsed => hide/show only the splitter
  //   shown     => hide/show the splitter and the box
  if (aHide)
  {
    // going into print preview mode
    gChromeState.sidebar = SidebarGetState();
    SidebarSetState("hidden");

    // deal with tab browser
    gBrowser.mStrip.setAttribute("moz-collapsed", "true");

    // deal with the Status Bar
    gChromeState.statusbarWasHidden = statusbar.hidden;
    statusbar.hidden = true;

    // deal with the notification box
    gChromeState.notificationsWereHidden = notificationBox.notificationsHidden;
    notificationBox.notificationsHidden = true;

    if (findbar)
    {
      gChromeState.findbarWasHidden = findbar.hidden;
      findbar.close();
    }
    else
    {
      gChromeState.findbarWasHidden = true;
    }

    gChromeState.syncNotificationsOpen = false;
    var syncNotifications = document.getElementById("sync-notifications");
    if (syncNotifications)
    {
      gChromeState.syncNotificationsOpen = !syncNotifications.notificationsHidden;
      syncNotifications.notificationsHidden = true;
    }
  }
  else
  {
    // restoring normal mode (i.e., leaving print preview mode)
    SidebarSetState(gChromeState.sidebar);

    // restore tab browser
    gBrowser.mStrip.removeAttribute("moz-collapsed");

    // restore the Status Bar
    statusbar.hidden = gChromeState.statusbarWasHidden;

    // restore the notification box
    notificationBox.notificationsHidden = gChromeState.notificationsWereHidden;

    if (!gChromeState.findbarWasHidden)
      findbar.open();

    if (gChromeState.syncNotificationsOpen)
      document.getElementById("sync-notifications").notificationsHidden = false;
  }

  // if we are unhiding and sidebar used to be there rebuild it
  if (!aHide && gChromeState.sidebar == "visible")
    SidebarRebuild();
}

var PrintPreviewListener = {
  _printPreviewTab: null,
  _tabBeforePrintPreview: null,

  getPrintPreviewBrowser: function () {
    if (!this._printPreviewTab) {
      this._tabBeforePrintPreview = getBrowser().selectedTab;
      this._printPreviewTab = getBrowser().addTab("about:blank");
      getBrowser().selectedTab = this._printPreviewTab;
    }
    return getBrowser().getBrowserForTab(this._printPreviewTab);
  },
  getSourceBrowser: function () {
    return this._tabBeforePrintPreview ?
      getBrowser().getBrowserForTab(this._tabBeforePrintPreview) :
      getBrowser().selectedBrowser;
  },
  getNavToolbox: function () {
    return window.getNavToolbox();
  },
  onEnter: function () {
    gInPrintPreviewMode = true;
    toggleAffectedChrome(true);
  },
  onExit: function () {
    getBrowser().selectedTab = this._tabBeforePrintPreview;
    this._tabBeforePrintPreview = null;
    gInPrintPreviewMode = false;
    toggleAffectedChrome(false);
    getBrowser().removeTab(this._printPreviewTab, { disableUndo: true });
    this._printPreviewTab = null;
  }
};

function getNavToolbox()
{
  if (!gNavToolbox)
    gNavToolbox = document.getElementById("navigator-toolbox");
  return gNavToolbox;
}

function BrowserPrintPreview()
{
  PrintUtils.printPreview(PrintPreviewListener);
}

function BrowserSetDefaultCharacterSet(aCharset)
{
  // no longer needed; set when setting Force; see bug 79608
}

function BrowserSetForcedCharacterSet(aCharset)
{
  getBrowser().docShell.charset = aCharset;
  BrowserCharsetReload();
}

function BrowserCharsetReload()
{
  BrowserReloadWithFlags(nsIWebNavigation.LOAD_FLAGS_CHARSET_CHANGE);
}

var gFindInstData;
function getFindInstData()
{
  if (!gFindInstData) {
    gFindInstData = new nsFindInstData();
    gFindInstData.browser = getBrowser();
    // defaults for rootSearchWindow and currentSearchWindow are fine here
  }
  return gFindInstData;
}

function BrowserFind()
{
  findInPage(getFindInstData());
}

function BrowserFindAgain(reverse)
{
  findAgainInPage(getFindInstData(), reverse);
}

function BrowserCanFindAgain()
{
  return canFindAgainInPage();
}

function getMarkupDocumentViewer()
{
  return getBrowser().markupDocumentViewer;
}
