/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*-
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
 * Portions created by the Initial Developer are Copyright (C) 1998-1999
 * the Initial Developer. All Rights Reserved.
 *
 * Contributor(s):
 *   Mark Banner <mark@standard8.demon.co.uk>
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

Components.utils.import("resource://gre/modules/Services.jsm");

var gFilterListMsgWindow = null;
var gCurrentFilterList;
var gCurrentFolder;

var gStatusFeedback = {
  progressMeterVisible : false,

  showStatusString: function(status)
  {
    document.getElementById("statusText").setAttribute("value", status);
  },
  startMeteors: function()
  {
    // change run button to be a stop button
    var runButton = document.getElementById("runFiltersButton");
    runButton.setAttribute("label", runButton.getAttribute("stoplabel"));
    runButton.setAttribute("accesskey", runButton.getAttribute("stopaccesskey"));

    if (!this.progressMeterVisible)
    {
      document.getElementById('statusbar-progresspanel').removeAttribute('collapsed');
      this.progressMeterVisible = true;
    }

    document.getElementById("statusbar-icon").setAttribute("mode", "undetermined");
  },
  stopMeteors: function()
  {
    try {
      // change run button to be a stop button
      var runButton = document.getElementById("runFiltersButton");
      runButton.setAttribute("label", runButton.getAttribute("runlabel"));
      runButton.setAttribute("accesskey", runButton.getAttribute("runaccesskey"));

      if (this.progressMeterVisible)
      {
        document.getElementById('statusbar-progresspanel').collapsed = true;
        this.progressMeterVisible = true;
      }
    }
    catch (ex) {
      // can get here if closing window when running filters
    }
  },
  showProgress: function(percentage)
  {
  },
  closeWindow: function()
  {
  }
};

var filterEditorQuitObserver = {
  observe: function(aSubject, aTopic, aData)
  {
    // Check whether or not we want to veto the quit request (unless another
    // observer already did.
    if (aTopic == "quit-application-requested" &&
        (aSubject instanceof Components.interfaces.nsISupportsPRBool) &&
        !aSubject.data)
      aSubject.data = !onFilterClose();
  }
}

function onLoad()
{
    gFilterListMsgWindow = Components.classes["@mozilla.org/messenger/msgwindow;1"].createInstance(Components.interfaces.nsIMsgWindow);
    gFilterListMsgWindow.domWindow = window;
    gFilterListMsgWindow.rootDocShell.appType = Components.interfaces.nsIDocShell.APP_TYPE_MAIL;
    gFilterListMsgWindow.statusFeedback = gStatusFeedback;

    updateButtons();

    // Get the folder where filters should be defined, if that server
    // can accept filters.
    var firstItem = getFilterFolderForSelection();

    // if the selected server cannot have filters, get the default server
    // if the default server cannot have filters, check all accounts
    // and get a server that can have filters.
    if (!firstItem)
        firstItem = getServerThatCanHaveFilters().rootFolder;

    if (firstItem) {
        selectFolder(firstItem);
    }

    Services.obs.addObserver(filterEditorQuitObserver,
                             "quit-application-requested", false);
}

/**
 * Called when a user selects a folder in the list, so we can update the filters
 * that are displayed
 *
 * @param aFolder  the nsIMsgFolder that was selected
 */
function onFilterFolderClick(aFolder)
{
    if (!aFolder || aFolder == gCurrentFolder)
      return;

    // Save the current filters to disk before switching because
    // the dialog may be closed and we'll lose current filters.
    gCurrentFilterList.saveToDefaultFile();

    selectFolder(aFolder);
}

function CanRunFiltersAfterTheFact(aServer)
{
  // filter after the fact is implement using search
  // so if you can't search, you can't filter after the fact
  return aServer.canSearchMessages;
}

// roots the tree at the specified folder
function setFolder(msgFolder)
{
   if (msgFolder == gCurrentFolder)
     return;
   gCurrentFolder = msgFolder;

   //Calling getFilterList will detect any errors in rules.dat, backup the file, and alert the user
   var filterList = msgFolder.getEditableFilterList(gFilterListMsgWindow);
   rebuildFilterList(filterList);

   // Select the first item in the list, if there is one.
   var list = document.getElementById("filterList");
   if (list.getRowCount())
     list.selectItem(list.getItemAtIndex(0));

   // This will get the deferred to account root folder, if server is deferred.
   // We intentionally do this after setting gCurrentFolder, as we want
   // that to refer to the rootFolder for the actual server, not the
   // deferred-to server, as gCurrentFolder is really a proxy for the
   // server whose filters we are editing. But below here we are managing
   // where the filters will get applied, which is on the deferred-to server.
   msgFolder = msgFolder.server.rootMsgFolder;

   // root the folder picker to this server
   var runMenu = document.getElementById("runFiltersPopup");
   runMenu._teardown();
   runMenu._parentFolder = msgFolder;
   runMenu._ensureInitialized();

   // run filters after the fact not supported by news
   if (CanRunFiltersAfterTheFact(msgFolder.server)) {
     document.getElementById("runFiltersFolder").removeAttribute("hidden");
     document.getElementById("runFiltersButton").removeAttribute("hidden");
     document.getElementById("folderPickerPrefix").removeAttribute("hidden");

     // for POP3 and IMAP, select the first folder, which is the INBOX
     document.getElementById("runFiltersFolder").selectedIndex = 0;
     runMenu.selectFolder(getFirstFolder(msgFolder));
   }
   else {
     document.getElementById("runFiltersFolder").setAttribute("hidden", "true");
     document.getElementById("runFiltersButton").setAttribute("hidden", "true");
     document.getElementById("folderPickerPrefix").setAttribute("hidden", "true");
   }

   // Get the first folder for this server. INBOX for
   // imap and pop accts and 1st news group for news.
   updateButtons();
}

function toggleFilter(aFilter, aIndex)
{
    if (aFilter.unparseable)
    {
      var bundle = document.getElementById("bundle_filter");
      var promptSvc = Components.classes["@mozilla.org/embedcomp/prompt-service;1"]
                                .getService(Components.interfaces.nsIPromptService);
      promptSvc.alert(window, null, bundle.getString("cannotEnableFilter"));
      return;
    }
    aFilter.enabled = !aFilter.enabled;

    // Now update the appropriate row
    var row = document.getElementById("filterList").childNodes[aIndex + 1];
    row.childNodes[1].setAttribute("enabled", aFilter.enabled);
}

// sets up the menulist and the filter list
function selectFolder(aFolder)
{
    // update the server menu
    var serverMenu = document.getElementById("serverMenuPopup");
    serverMenu.selectFolder(aFolder);

    setFolder(aFolder);
}

function currentFilter()
{
    var currentItem = document.getElementById("filterList").selectedItem;
    return currentItem ? currentItem._filter : null;
}

function onEditFilter()
{
  var selectedFilter = currentFilter();
  var args = {filter: selectedFilter, filterList: gCurrentFilterList};

  window.openDialog("chrome://messenger/content/FilterEditor.xul", "FilterEditor", "chrome,modal,titlebar,resizable,centerscreen", args);

  if ("refresh" in args && args.refresh)
    rebuildFilterList(gCurrentFilterList);
}

function onNewFilter(emailAddress)
{
  var args = {filterList: gCurrentFilterList};

  window.openDialog("chrome://messenger/content/FilterEditor.xul", "FilterEditor", "chrome,modal,titlebar,resizable,centerscreen", args);

  if ("refresh" in args && args.refresh) {
    rebuildFilterList(gCurrentFilterList);

    // the new filter is always added as first item on top. Select it.
    let list = document.getElementById("filterList");
    list.clearSelection();
    list.addItemToSelection(list.childNodes[1]);
    updateViewPosition(1);
  }
}

function onDeleteFilter()
{
  let list = document.getElementById("filterList");
  let items = list.selectedItems;
  if (!items.length)
    return;

  let checkValue = {value:false};
  let prefBranch = Components.classes["@mozilla.org/preferences-service;1"]
                             .getService(Components.interfaces.nsIPrefService)
                             .getBranch(null);
  let bundle = document.getElementById("bundle_filter");
  let promptSvc = Components.classes["@mozilla.org/embedcomp/prompt-service;1"]
                            .getService(Components.interfaces.nsIPromptService);
  if ((prefBranch.getBoolPref("mailnews.filters.confirm_delete")) &&
      (promptSvc.confirmEx(window, null,
                           bundle.getString("deleteFilterConfirmation"),
                           promptSvc.STD_YES_NO_BUTTONS,
                           '', '', '',
                           bundle.getString('dontWarnAboutDeleteCheckbox'),
                           checkValue)))
    return;

  if (checkValue.value)
     prefBranch.setBoolPref("mailnews.filters.confirm_delete", false);

  // save filter position before the first selected one
  let newSelection = items[0].previousElementSibling;
  if (newSelection == list.childNodes[0])
    newSelection = null;

  // must reverse the loop, as the items list shrinks when we delete
  for (let index = items.length - 1; index >= 0; --index) {
    let item = items[index];
    gCurrentFilterList.removeFilter(item._filter);
    document.getElementById("filterList").removeChild(item);
  }

  // select filter above previously selected if one existed, otherwise the first one
  if (!newSelection && list.itemCount)
    newSelection = list.childNodes[1];
  if (newSelection) {
    list.addItemToSelection(newSelection);
    updateViewPosition(-1);
  }
}

function onUp(event)
{
    moveCurrentFilter(Components.interfaces.nsMsgFilterMotion.up);
}

function onDown(event)
{
    moveCurrentFilter(Components.interfaces.nsMsgFilterMotion.down);
}

function viewLog()
{
  var args = {filterList: gCurrentFilterList};

  window.openDialog("chrome://messenger/content/viewLog.xul", "FilterLog", "chrome,modal,titlebar,resizable,centerscreen", args);
}

function onFilterUnload()
{
  gCurrentFilterList.saveToDefaultFile();
  Services.obs.removeObserver(filterEditorQuitObserver,
                              "quit-application-requested");
}

function onFilterClose()
{
  var runButton = document.getElementById("runFiltersButton");
  if (runButton.getAttribute("label") == runButton.getAttribute("stoplabel")) {
    var bundle = document.getElementById("bundle_filter");
    var promptTitle = bundle.getString("promptTitle");
    var promptMsg = bundle.getString("promptMsg");
    var stopButtonLabel = bundle.getString("stopButtonLabel");
    var continueButtonLabel = bundle.getString("continueButtonLabel");

    var promptSvc = Components.classes["@mozilla.org/embedcomp/prompt-service;1"]
                              .getService(Components.interfaces.nsIPromptService);
    var result = promptSvc.confirmEx(window, promptTitle, promptMsg,
               (promptSvc.BUTTON_TITLE_IS_STRING * promptSvc.BUTTON_POS_0) +
               (promptSvc.BUTTON_TITLE_IS_STRING * promptSvc.BUTTON_POS_1),
               continueButtonLabel, stopButtonLabel, null, null, {value:0});

    if (result)
      gFilterListMsgWindow.StopUrls();
    else
      return false;
  }

  return true;
}

function runSelectedFilters()
{
  // if run button has "stop" label, do stop.
  var runButton = document.getElementById("runFiltersButton");
  if (runButton.getAttribute("label") == runButton.getAttribute("stoplabel")) {
    gFilterListMsgWindow.StopUrls();
    return;
  }

  var menu = document.getElementById("runFiltersFolder");
  var folder = menu._folder || menu.selectedItem._folder;

  var filterService = Components.classes["@mozilla.org/messenger/services/filters;1"].getService(Components.interfaces.nsIMsgFilterService);
  var filterList = filterService.getTempFilterList(folder);
  var folders = Components.classes["@mozilla.org/supports-array;1"].createInstance(Components.interfaces.nsISupportsArray);
  folders.AppendElement(folder);

  // make sure the tmp filter list uses the real filter list log stream
  filterList.logStream = gCurrentFilterList.logStream;
  filterList.loggingEnabled = gCurrentFilterList.loggingEnabled;

  var list = document.getElementById("filterList");
  var index = 0;
  for each (var item in list.selectedItems) {
    filterList.insertFilterAt(index++, item._filter);
  }

  filterService.applyFiltersToFolders(filterList, folders, gFilterListMsgWindow);
}

function moveCurrentFilter(motion)
{
    var filter = currentFilter();
    if (!filter)
      return;

    gCurrentFilterList.moveFilter(filter, motion);
    rebuildFilterList(gCurrentFilterList);
}

function rebuildFilterList(aFilterList)
{
  gCurrentFilterList = aFilterList;
  var list = document.getElementById("filterList");

  // Make a note of which filters were previously selected
  var selectedNames = [];
  for (var i = 0; i < list.selectedItems.length; i++)
    selectedNames.push(list.selectedItems[i]._filter.filterName);

  // Save scroll position so we can try to restore it later.
  let firstVisibleRowIndex = list.getIndexOfFirstVisibleRow();

  // Remove any existing child nodes, but not our headers
  for (var i = list.childNodes.length - 1; i > 0; i--) {
    list.removeChild(list.childNodes[i]);
  }

  // listbox.xml seems to cache the value of the first selected item in a
  // range at _selectionStart. The old value though is now obsolete,
  // since we will recreate all of the elements. We need to clear this,
  // and one way to do this is with a call to clearSelection. This might be
  // ugly from an accessibility perspective, since it fires an onSelect event.
  list.clearSelection();

  for (i = 0; i < aFilterList.filterCount; i++) {
    var filter = aFilterList.getFilterAt(i);
    var listitem = document.createElement("listitem");
    var nameCell = document.createElement("listcell");
    nameCell.setAttribute("label", filter.filterName);
    var enabledCell = document.createElement("listcell");
    enabledCell.setAttribute("enabled", filter.enabled);
    enabledCell.setAttribute("class", "listcell-iconic");
    listitem.appendChild(nameCell);
    listitem.appendChild(enabledCell);

    // We have to attach this listener to the listitem, even though we only care
    // about clicks on the enabledCell.  However, attaching to that item doesn't
    // result in any events actually getting received
    listitem.addEventListener("click", onFilterClick, true);

    listitem.addEventListener("dblclick", onFilterDoubleClick, true);
    listitem._filter = filter;
    list.appendChild(listitem);

    if (selectedNames.indexOf(filter.filterName) != -1)
      list.addItemToSelection(listitem);
  }
  updateViewPosition(firstVisibleRowIndex);
}

function updateViewPosition(firstVisibleRowIndex)
{
  let list = document.getElementById("filterList");
  if (firstVisibleRowIndex == -1)
    firstVisibleRowIndex = list.getIndexOfFirstVisibleRow();
  // Restore to the extent possible the scroll position.
  if (firstVisibleRowIndex && list.itemCount)
    list.scrollToIndex(Math.min(firstVisibleRowIndex, list.itemCount - 1));

  if (list.selectedCount) {
    // Make sure that at least the first selected item is visible.
    list.ensureElementIsVisible(list.selectedItems[0]);

    // The current item should be the first selected item, so that keyboard
    // selection extension can work.
    list.currentItem = list.selectedItems[0];
  }

  updateButtons();
  list.focus();
}

function updateButtons()
{
    var list = document.getElementById("filterList");
    var numFiltersSelected = list.selectedItems.length;
    var oneFilterSelected = (numFiltersSelected == 1);

    var filter = currentFilter();
    // "edit" only enabled when one filter selected or if we couldn't parse the filter
    var disabled = !oneFilterSelected || filter.unparseable
    document.getElementById("editButton").disabled = disabled;
    
    // "delete" only disabled when no filters are selected
    document.getElementById("deleteButton").disabled = !numFiltersSelected;

    // we can run multiple filters on a folder
    // so only disable this UI if no filters are selected
    document.getElementById("folderPickerPrefix").disabled = !numFiltersSelected;
    document.getElementById("runFiltersFolder").disabled = !numFiltersSelected;
    document.getElementById("runFiltersButton").disabled = !numFiltersSelected;

    // "up" enabled only if one filter selected, and it's not the first
    // don't use list.currentIndex here, it's buggy when we've just changed the
    // children in the list (via rebuildFilterList)
    var upDisabled = !(oneFilterSelected && 
                       list.selectedItems[0] != list.childNodes[1]);
    document.getElementById("reorderUpButton").disabled = upDisabled
    // "down" enabled only if one filter selected, and it's not the last
    var downDisabled = !(oneFilterSelected && list.currentIndex < list.getRowCount()-1);
    document.getElementById("reorderDownButton").disabled = downDisabled;
}

/**
  * Given a selected folder, returns the folder where filters should
  *  be defined (the root folder except for news) if the server can
  *  accept filters.
  *
  * @returns an nsIMsgFolder where the filter is defined
  */
function getFilterFolderForSelection()
{
    var args = window.arguments;

    if (args && args[0] && args[0].folder)
    {
        var selectedFolder = args[0].folder;
        var msgFolder = selectedFolder.QueryInterface(Components.interfaces.nsIMsgFolder);
        try
        {
            var rootFolder = msgFolder.server.rootFolder;
            if (rootFolder.isServer)
            {
                var server = rootFolder.server;
                if (server.canHaveFilters)
                    return (server.type == "nntp") ? msgFolder : rootFolder;
            }
        }
        catch (ex)
        {
        }
    }

    return null;
}

/** if the selected server cannot have filters, get the default server
  * if the default server cannot have filters, check all accounts
  * and get a server that can have filters.
  *
  * @returns an nsIMsgIncomingServer
  */
function getServerThatCanHaveFilters()
{
    var firstItem = null;

    var accountManager
        = Components.classes["@mozilla.org/messenger/account-manager;1"].
            getService(Components.interfaces.nsIMsgAccountManager);

    var defaultAccount = accountManager.defaultAccount;
    var defaultIncomingServer = defaultAccount.incomingServer;

    // check to see if default server can have filters
    if (defaultIncomingServer.canHaveFilters) {
        firstItem = defaultIncomingServer;
    }
    // if it cannot, check all accounts to find a server
    // that can have filters
    else
    {
        var allServers = accountManager.allServers;
        var numServers = allServers.Count();
        var index = 0;
        for (index = 0; index < numServers; index++)
        {
            var currentServer
            = allServers.GetElementAt(index).QueryInterface(Components.interfaces.nsIMsgIncomingServer);

            if (currentServer.canHaveFilters)
            {
                firstItem = currentServer;
                break;
            }
        }
    }

    return firstItem;
}

function onFilterClick(event)
{
    // we only care about button 0 (left click) events
    if (event.button != 0)
      return;

    // Remember, we had to attach the click-listener to the whole listitem, so
    // now we need to see if the clicked the enable-column
    var toggle = event.target.childNodes[1];
    if ((event.clientX < toggle.boxObject.x + toggle.boxObject.width) &&
        (event.clientX > toggle.boxObject.x)) {
      var list = document.getElementById("filterList");
      toggleFilter(event.target._filter, list.getIndexOfItem(event.target));
      event.stopPropagation();
    }
}

function onFilterDoubleClick(event)
{
    // we only care about button 0 (left click) events
    if (event.button != 0)
      return;

    onEditFilter();
}

function onFilterListKeyPress(event)
{
  if (event.charCode == KeyEvent.DOM_VK_SPACE)
  {
    let list = document.getElementById("filterList");
    for each (var item in list.selectedItems)
      toggleFilter(item._filter, list.getIndexOfItem(item));
  }
  else switch (event.keyCode)
  {
    case KeyEvent.DOM_VK_DELETE:
      if (!document.getElementById("deleteButton").disabled)
        onDeleteFilter();
      break;
    case KeyEvent.DOM_VK_ENTER:
    case KeyEvent.DOM_VK_RETURN:
      if (!document.getElementById("editButton").disabled)
        onEditFilter();
      break;
  }
}

function onTargetSelect(event) {
  var menu = document.getElementById("runFiltersFolder");
  menu._folder = event.target._folder;
  menu.setAttribute("label", event.target._folder.prettyName);
}

/**
  * For a given server folder, get the first folder. For imap and pop it's INBOX
  * and it's the very first group for news accounts.
  */
function getFirstFolder(msgFolder)
{
  // Sanity check.
  if (! msgFolder.isServer)
    return msgFolder;

  try {
    // Find Inbox for imap and pop
    if (msgFolder.server.type != "nntp")
    {
      const nsMsgFolderFlags = Components.interfaces.nsMsgFolderFlags;
      var inboxFolder = msgFolder.getFolderWithFlags(nsMsgFolderFlags.Inbox);
      if (inboxFolder)
        return inboxFolder;
      else
        // If inbox does not exist then use the server as default.
        return msgFolder;
    }
    else
      // XXX TODO: For news, we should find the 1st group/folder off the news groups. For now use server.
      return msgFolder;
  }
  catch (ex) {
    dump(ex + "\n");
  }
  return msgFolder;
}
