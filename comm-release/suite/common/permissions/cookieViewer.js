/* -*- Mode: C++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*-
 *
 * ***** BEGIN LICENSE BLOCK *****
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
 * The Original Code is Mozilla Communicator client code, released
 * March 31, 1998.
 *
 * The Initial Developer of the Original Code is
 * Netscape Communications Corporation.
 * Portions created by the Initial Developer are Copyright (C) 1998
 * the Initial Developer. All Rights Reserved.
 *
 * Contributor(s):
 *   Ben Goodger
 *   Jens Hatlak <jh@junetz.de>
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

var kObserverService;

// interface variables
var cookiemanager         = null;          // cookiemanager interface
var permissionmanager     = null;          // permissionmanager interface
var promptservice         = null;          // promptservice interface
var gDateService = null;

// cookies and permissions list
var cookies              = [];
var permissions          = [];
var allCookies           = [];
var deletedCookies       = [];
var deletedPermissions   = [];

const nsIPermissionManager = Components.interfaces.nsIPermissionManager;
const nsICookiePermission = Components.interfaces.nsICookiePermission;

var cookieBundle;
var gUpdatingBatch = "";

function Startup() {

  // arguments passed to this routine:
  //   cookieManager 
 
  // xpconnect to cookiemanager/permissionmanager/promptservice interfaces
  cookiemanager = Components.classes["@mozilla.org/cookiemanager;1"].getService();
  cookiemanager = cookiemanager.QueryInterface(Components.interfaces.nsICookieManager);
  permissionmanager = Components.classes["@mozilla.org/permissionmanager;1"].getService();
  permissionmanager = permissionmanager.QueryInterface(Components.interfaces.nsIPermissionManager);
  promptservice = Components.classes["@mozilla.org/embedcomp/prompt-service;1"].getService();
  promptservice = promptservice.QueryInterface(Components.interfaces.nsIPromptService);

  // intialize gDateService
  if (!gDateService) {
    const nsScriptableDateFormat_CONTRACTID = "@mozilla.org/intl/scriptabledateformat;1";
    const nsIScriptableDateFormat = Components.interfaces.nsIScriptableDateFormat;
    gDateService = Components.classes[nsScriptableDateFormat_CONTRACTID]
      .getService(nsIScriptableDateFormat);
  }

  // intialize string bundle
  cookieBundle = document.getElementById("cookieBundle");

  // load in the cookies and permissions
  cookiesTree = document.getElementById("cookiesTree");
  permissionsTree = document.getElementById("permissionsTree");
  loadCookies();
  loadPermissions();

  // be prepared to reload the display if anything changes
  kObserverService = Components.classes["@mozilla.org/observer-service;1"].getService(Components.interfaces.nsIObserverService);
  kObserverService.addObserver(cookieReloadDisplay, "cookie-changed", false);
  kObserverService.addObserver(cookieReloadDisplay, "perm-changed", false);

  // filter the table if requested by caller
  if (window.arguments &&
      window.arguments[0] &&
      window.arguments[0].filterString)
    setFilter(window.arguments[0].filterString);

  document.getElementById("filter").focus();
}

function Shutdown() {
  kObserverService.removeObserver(cookieReloadDisplay, "cookie-changed");
  kObserverService.removeObserver(cookieReloadDisplay, "perm-changed");
}

var cookieReloadDisplay = {
  observe: function(subject, topic, state) {
    if (topic == gUpdatingBatch)
      return;
    if (topic == "cookie-changed") {
      allCookies.length = 0;
      loadCookies();
    } else if (topic == "perm-changed") {
      permissions.length = 0;
      loadPermissions();
    }
  }
}

function doSelectAll() {
  var elem = document.commandDispatcher.focusedElement;
  if (elem && "treeBoxObject" in elem)
    elem.view.selection.selectAll();
}

/*** =================== COOKIES CODE =================== ***/

const nsICookie = Components.interfaces.nsICookie;

var cookiesTreeView = {
  rowCount : 0,
  setTree : function(tree){},
  getImageSrc : function(row,column) {},
  getProgressMode : function(row,column) {},
  getCellValue : function(row,column) {},
  getCellText : function(row,column){
    var rv="";
    switch (column.id) {
    case "domainCol":
      rv = cookies[row].rawHost;
      break;
    case "nameCol":
      rv = cookies[row].name;
      break;
    case "expiresCol":
      rv = cookies[row].expires;
      break;
    }
    return rv;
  },
  isSeparator : function(index) {return false;},
  isSorted: function() { return false; },
  isContainer : function(index) {return false;},
  cycleHeader : function(aCol) {},
  getRowProperties : function(row,prop) {},
  getColumnProperties : function(column,prop) {},
  getCellProperties : function(row,column,prop) {}
 };
var cookiesTree;

function Cookie(id,name,value,isDomain,host,rawHost,path,isSecure,expires) {
  this.id = id;
  this.name = name;
  this.value = value;
  this.isDomain = isDomain;
  this.host = host;
  this.rawHost = rawHost;
  this.path = path;
  this.isSecure = isSecure;
  this.expires = GetExpiresString(expires);
  this.expiresSortValue = expires;
}

function loadCookies() {
  // load cookies into a table
  var enumerator = cookiemanager.enumerator;
  var count = 0;
  while (enumerator.hasMoreElements()) {
    var nextCookie = enumerator.getNext();
    if (!nextCookie) break;
    nextCookie = nextCookie.QueryInterface(Components.interfaces.nsICookie);
    var host = nextCookie.host;
    allCookies[count] =
      new Cookie(count++, nextCookie.name, nextCookie.value, nextCookie.isDomain, host,
                 (host.charAt(0)==".") ? host.substring(1,host.length) : host,
                 nextCookie.path, nextCookie.isSecure, nextCookie.expires);
  }

  // filter, sort and display the table
  cookiesTree.treeBoxObject.view = cookiesTreeView;
  filter(document.getElementById("filter").value);
}

function GetExpiresString(expires) {
  if (expires) {
    var date = new Date(1000*expires);

    // if a server manages to set a really long-lived cookie, the dateservice
    // can't cope with it properly, so we'll just return a blank string
    // see bug 238045 for details
    var expiry = "";
    try {
      expiry = gDateService.FormatDateTime("", gDateService.dateFormatLong,
                                           gDateService.timeFormatSeconds, 
                                           date.getFullYear(), date.getMonth()+1, 
                                           date.getDate(), date.getHours(),
                                           date.getMinutes(), date.getSeconds());
    } catch(ex) {
      // do nothing
    }
    return expiry;
  }
  return cookieBundle.getString("expireAtEndOfSession");
}

function CookieSelected() {
  var selections = GetTreeSelections(cookiesTree);
  if (selections.length) {
    document.getElementById("removeCookie").removeAttribute("disabled");
  } else {
    document.getElementById("removeCookie").setAttribute("disabled", "true");
    ClearCookieProperties();
    return true;
  }
    
  var idx = selections[0];
  if (idx >= cookies.length) {
    // Something got out of synch.  See bug 119812 for details
    dump("Tree and viewer state are out of sync! " +
         "Help us figure out the problem in bug 119812");
    return false;
  }

  var props = [
    {id: "ifl_name", value: cookies[idx].name},
    {id: "ifl_value", value: cookies[idx].value}, 
    {id: "ifl_isDomain",
     value: cookies[idx].isDomain ?
            cookieBundle.getString("domainColon") : cookieBundle.getString("hostColon")},
    {id: "ifl_host", value: cookies[idx].host},
    {id: "ifl_path", value: cookies[idx].path},
    {id: "ifl_isSecure",
     value: cookies[idx].isSecure ?
            cookieBundle.getString("forSecureOnly") : 
            cookieBundle.getString("forAnyConnection")},
    {id: "ifl_expires", value: cookies[idx].expires}
  ];

  var value;
  var field;
  for (var i = 0; i < props.length; i++)
  {
    field = document.getElementById(props[i].id);
    if ((selections.length > 1) && (props[i].id != "ifl_isDomain")) {
      value = ""; // clear field if multiple selections
    } else {
      value = props[i].value;
    }
    field.value = value;
  }
  return true;
}

function ClearCookieProperties() {
  var properties = 
    ["ifl_name","ifl_value","ifl_host","ifl_path","ifl_isSecure","ifl_expires"];
  for (var prop=0; prop<properties.length; prop++) {
    document.getElementById(properties[prop]).value = "";
  }
}

function DeleteCookie() {
  if (cookiesTreeView.selection.count > 1) {
    var title = cookieBundle.getString("deleteSelectedCookiesTitle");
    var msg = cookieBundle.getString("deleteSelectedCookies");
    var flags = ((promptservice.BUTTON_TITLE_IS_STRING * promptservice.BUTTON_POS_0) +
                 (promptservice.BUTTON_TITLE_CANCEL * promptservice.BUTTON_POS_1) +
                 promptservice.BUTTON_POS_1_DEFAULT)
    var yes = cookieBundle.getString("deleteSelectedCookiesYes");
    if (promptservice.confirmEx(window, title, msg, flags, yes, null, null, null, {value:0}) == 1)
      return;
  }
  DeleteSelectedItemFromTree(cookiesTree, cookiesTreeView,
                                 cookies, deletedCookies,
                                 "removeCookie", "removeAllCookies");
  if (document.getElementById("filter").value) {
    // remove selected cookies from unfiltered set
    for (var i = 0; i < deletedCookies.length; ++i) {
      allCookies.splice(allCookies.indexOf(deletedCookies[i]), 1);
    }
  }
  if (!cookies.length) {
    ClearCookieProperties();
  }
  FinalizeCookieDeletions();
}

function DeleteAllCookies() {
  var title = cookieBundle.getString("deleteAllCookiesTitle");
  var msg = cookieBundle.getString("deleteAllCookies");
  var flags = ((promptservice.BUTTON_TITLE_IS_STRING * promptservice.BUTTON_POS_0) +
               (promptservice.BUTTON_TITLE_CANCEL * promptservice.BUTTON_POS_1) +
               promptservice.BUTTON_POS_1_DEFAULT)
  var yes = cookieBundle.getString("deleteAllCookiesYes");
  if (promptservice.confirmEx(window, title, msg, flags, yes, null, null, null, {value:0}) == 1)
    return;

  ClearCookieProperties();
  DeleteAllFromTree(cookiesTree, cookiesTreeView,
                        cookies, deletedCookies,
                        "removeCookie", "removeAllCookies");
  allCookies.length = 0;
  FinalizeCookieDeletions();
}

function FinalizeCookieDeletions() {
  gUpdatingBatch = "cookie-changed";
  for (var c=0; c<deletedCookies.length; c++) {
    cookiemanager.remove(deletedCookies[c].host,
                         deletedCookies[c].name,
                         deletedCookies[c].path,
                         document.getElementById("checkbox").checked);
  }
  deletedCookies.length = 0;
  gUpdatingBatch = "";
}

function HandleCookieKeyPress(e) {
  if (e.keyCode == KeyEvent.DOM_VK_DELETE) {
    DeleteCookie();
  }
}

var lastCookieSortColumn = "rawHost";
var lastCookieSortAscending = true;

function CookieColumnSort(column, updateSelection) {
  lastCookieSortAscending =
      SortTree(cookiesTree, cookiesTreeView, cookies,
               column, lastCookieSortColumn, lastCookieSortAscending,
               updateSelection);
  lastCookieSortColumn = column;
  // set the sortDirection attribute to get the styling going
  // first we need to get the right element
  var sortedCol;
  switch (column) {
    case "rawHost":
      sortedCol = document.getElementById("domainCol");
      break;
    case "name":
      sortedCol = document.getElementById("nameCol");
      break;
    case "expires":
      sortedCol = document.getElementById("expiresCol");
      break;
  }
  if (lastCookieSortAscending)
    sortedCol.setAttribute("sortDirection", "ascending");
  else
    sortedCol.setAttribute("sortDirection", "descending");

  // clear out the sortDirection attribute on the rest of the columns
  var currentCol = sortedCol.parentNode.firstChild;
  while (currentCol) {
    if (currentCol != sortedCol && currentCol.localName == "treecol")
      currentCol.removeAttribute("sortDirection");
    currentCol = currentCol.nextSibling;
  }
}

/*** =================== PERMISSIONS CODE =================== ***/

var permissionsTreeView = {
  rowCount : 0,
  setTree : function(tree){},
  getImageSrc : function(row,column) {},
  getProgressMode : function(row,column) {},
  getCellValue : function(row,column) {},
  getCellText : function(row,column){
    var rv="";
    if (column.id=="siteCol") {
      rv = permissions[row].rawHost;
    } else if (column.id=="capabilityCol") {
      rv = permissions[row].capability;
    }
    return rv;
  },
  isSeparator : function(index) {return false;},
  isSorted: function() { return false; },
  isContainer : function(index) {return false;},
  cycleHeader : function(aCol) {},
  getRowProperties : function(row,prop) {},
  getColumnProperties : function(column,prop) {},
  getCellProperties : function(row,column,prop) {}
 };
var permissionsTree;

function Permission(id, host, rawHost, type, capability) {
  this.id = id;
  this.host = host;
  this.rawHost = rawHost;
  this.type = type;
  this.capability = capability;
}

function loadPermissions() {
  // load permissions into a table
  var enumerator = permissionmanager.enumerator;
  var canStr = cookieBundle.getString("can");
  var canSessionStr = cookieBundle.getString("canSession");
  var cannotStr = cookieBundle.getString("cannot");
  while (enumerator.hasMoreElements()) {
    var nextPermission = enumerator.getNext();
    nextPermission = nextPermission.QueryInterface(Components.interfaces.nsIPermission);
    if (nextPermission.type == "cookie") {
      var host = nextPermission.host;
      var capability;
      switch (nextPermission.capability) {
        case nsIPermissionManager.ALLOW_ACTION:
          capability = canStr;
          break;
        case nsIPermissionManager.DENY_ACTION:
          capability = cannotStr;
          break;
        case nsICookiePermission.ACCESS_SESSION:
          capability = canSessionStr;
          break;
        default:
          continue;
      }
      permissions.push(new Permission(permissions.length, host,
                                      host.replace(/^\./, ""),
                                      nextPermission.type, capability));
    }
  }
  permissionsTreeView.rowCount = permissions.length;

  // sort and display the table
  permissionsTree.treeBoxObject.view = permissionsTreeView;
  permissionsTreeView.selection.select(-1);
  SortTree(permissionsTree, permissionsTreeView, permissions,
           lastPermissionSortColumn, lastPermissionSortColumn,
           !lastPermissionSortAscending);

  // disable "remove all" button if there are no cookies
  if (permissions.length == 0) {
    document.getElementById("removeAllPermissions").setAttribute("disabled", "true");
  } else {
    document.getElementById("removeAllPermissions").removeAttribute("disabled");
  }

}

function PermissionSelected() {
  var selections = GetTreeSelections(permissionsTree);
  if (selections.length)
    document.getElementById("removePermission").removeAttribute("disabled");
  else
    document.getElementById("removePermission").setAttribute("disabled", "true");
}

function DeletePermission() {
  if (permissionsTreeView.selection.count > 1) {
    var title = cookieBundle.getString("deleteSelectedSitesTitle");
    var msg = cookieBundle.getString("deleteSelectedCookiesSites");
    var flags = ((promptservice.BUTTON_TITLE_IS_STRING * promptservice.BUTTON_POS_0) +
                 (promptservice.BUTTON_TITLE_CANCEL * promptservice.BUTTON_POS_1) +
                 promptservice.BUTTON_POS_1_DEFAULT)
    var yes = cookieBundle.getString("deleteSelectedSitesYes");
    if (promptservice.confirmEx(window, title, msg, flags, yes, null, null, null, {value:0}) == 1)
      return;
  }
  DeleteSelectedItemFromTree(permissionsTree, permissionsTreeView,
                                 permissions, deletedPermissions,
                                 "removePermission", "removeAllPermissions");
  FinalizePermissionDeletions();
}

function setCookiePermissions(action) {
  var site = document.getElementById('cookie-site');
  var url = site.value.replace(/^\s*([-\w]*:\/+)?/, "http://");
  var ioService = Components.classes["@mozilla.org/network/io-service;1"]
                            .getService(Components.interfaces.nsIIOService);
  var uri = ioService.newURI(url, null, null);
  
  // only set the permission if the permission doesn't already exist
  if (permissionmanager.testPermission(uri, "cookie") != action)
    permissionmanager.add(uri, "cookie", action);

  site.focus();
  site.value = "";
}

function buttonEnabling(textfield) {
  // trim any leading space
  var site = textfield.value.replace(/^\s*([-\w]*:\/+)?/, "");
  var block = document.getElementById("btnBlock");
  var session = document.getElementById("btnSession");
  var allow = document.getElementById("btnAllow");
  block.disabled = !site;
  session.disabled = !site;
  allow.disabled = !site;
}

function DeleteAllPermissions() {
  var title = cookieBundle.getString("deleteAllSitesTitle");
  var msg = cookieBundle.getString("deleteAllCookiesSites");
  var flags = ((promptservice.BUTTON_TITLE_IS_STRING * promptservice.BUTTON_POS_0) +
               (promptservice.BUTTON_TITLE_CANCEL * promptservice.BUTTON_POS_1) +
               promptservice.BUTTON_POS_1_DEFAULT)
  var yes = cookieBundle.getString("deleteAllSitesYes");
  if (promptservice.confirmEx(window, title, msg, flags, yes, null, null, null, {value:0}) == 1)
    return;
    
  DeleteAllFromTree(permissionsTree, permissionsTreeView,
                        permissions, deletedPermissions,
                        "removePermission", "removeAllPermissions");
  FinalizePermissionDeletions();
}

function FinalizePermissionDeletions() {
  if (!deletedPermissions.length)
    return;

  gUpdatingBatch = "perm-changed";
  for (var p = 0; p < deletedPermissions.length; ++p)
    permissionmanager.remove(deletedPermissions[p].host, deletedPermissions[p].type);
  deletedPermissions.length = 0;
  gUpdatingBatch = "";
}

function HandlePermissionKeyPress(e) {
  if (e.keyCode == 46) {
    DeletePermission();
  }
}

var lastPermissionSortColumn = "rawHost";
var lastPermissionSortAscending = true;

function PermissionColumnSort(column, updateSelection) {
  lastPermissionSortAscending =
    SortTree(permissionsTree, permissionsTreeView, permissions,
                 column, lastPermissionSortColumn, lastPermissionSortAscending, 
                 updateSelection);
  lastPermissionSortColumn = column;
  
  // make sure sortDirection is set
  var sortedCol;
  switch (column) {
    case "rawHost":
      sortedCol = document.getElementById("siteCol");
      break;
    case "capability":
      sortedCol = document.getElementById("capabilityCol");
      break;
  }
  if (lastPermissionSortAscending)
    sortedCol.setAttribute("sortDirection", "ascending");
  else
    sortedCol.setAttribute("sortDirection", "descending");

  // clear out the sortDirection attribute on the rest of the columns
  var currentCol = sortedCol.parentNode.firstChild;
  while (currentCol) {
    if (currentCol != sortedCol && currentCol.localName == "treecol")
      currentCol.removeAttribute("sortDirection");
    currentCol = currentCol.nextSibling;
  }
}

/*** ============ CODE FOR HELP BUTTON =================== ***/

function doHelpButton()
{
  var selTab = document.getElementById('tabbox').selectedTab;
  var key = selTab.getAttribute('help');
  openHelp(key, 'chrome://communicator/locale/help/suitehelp.rdf');
}

/*** =================== FILTER CODE =================== ***/

function filterCookies(aFilterValue)
{
  var filterSet = [];
  for (var i = 0; i < allCookies.length; ++i) {
    var cookie = allCookies[i];
    if (cookie.rawHost.indexOf(aFilterValue) != -1 ||
        cookie.name.indexOf(aFilterValue) != -1 ||
        cookie.value.indexOf(aFilterValue) != -1)
      filterSet.push(cookie);
  }
  return filterSet;
}

function filter(filter)
{
  // clear the display
  var oldCount = cookiesTreeView.rowCount;
  cookiesTreeView.rowCount = 0;
  cookiesTree.treeBoxObject.rowCountChanged(0, -oldCount);

  // set up the display
  cookies = filter ? filterCookies(filter) : allCookies;
  cookiesTreeView.rowCount = cookies.length;
  cookiesTree.treeBoxObject.rowCountChanged(0, cookiesTreeView.rowCount);

  // sort the tree according to the last sort parameters
  SortTree(cookiesTree, cookiesTreeView, cookies, lastCookieSortColumn,
           lastCookieSortColumn, !lastCookieSortAscending);

  // disable Remove All Cookies button if the view is filtered or there are no cookies
  if (filter || !cookies.length)
    document.getElementById("removeAllCookies").setAttribute("disabled", "true");
  else
    document.getElementById("removeAllCookies").removeAttribute("disabled");

  // if the view is filtered and not empty then select the first item
  if (filter && cookies.length)
    cookiesTreeView.selection.select(0);
}

function setFilter(aFilterString)
{
  document.getElementById("filter").value = aFilterString;
  filter(aFilterString);
}

function focusFilterBox()
{
  var filterBox = document.getElementById("filter");
  filterBox.focus();
  filterBox.select();
}
