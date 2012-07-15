/* -*- Mode: C++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*-
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

var gRDF = Components.classes["@mozilla.org/rdf/rdf-service;1"].getService(Components.interfaces.nsIRDFService);

var gEditButton;
var gDeleteButton;
var gReorderUpButton;
var gReorderDownButton;
var gRunFiltersFolderPickerLabel;
var gRunFiltersFolderPicker;
var gRunFiltersButton;
var gFilterBundle;
var gFilterListMsgWindow = null;
var gFilterTree;
var gStatusBar;
var gStatusText;
var gCurrentServerURI = null;

var gStatusFeedback = {
	showStatusString: function(status)
  {
    gStatusText.setAttribute("value", status);
  },
	startMeteors: function()
  {
    // change run button to be a stop button
    gRunFiltersButton.setAttribute("label", gRunFiltersButton.getAttribute("stoplabel"));
    gRunFiltersButton.setAttribute("accesskey", gRunFiltersButton.getAttribute("stopaccesskey"));
    gStatusBar.setAttribute("mode", "undetermined");
  },
	stopMeteors: function() 
  {
    try {
      // change run button to be a stop button
      gRunFiltersButton.setAttribute("label", gRunFiltersButton.getAttribute("runlabel"));
      gRunFiltersButton.setAttribute("accesskey", gRunFiltersButton.getAttribute("runaccesskey"));
      gStatusBar.setAttribute("mode", "normal");
    }
    catch (ex) {
      // can get here if closing window when running filters
    }
  },
  showProgress: function(percentage)
  {
      //dump("XXX progress" + percentage + "\n");
  },
  closeWindow: function()
  {
  }
};

var gFilterTreeView = {
  mEnabledAtom: Components.classes['@mozilla.org/atom-service;1']
                          .getService(Components.interfaces.nsIAtomService)
                          .getAtom('Enabled-true'),
  mTree: null,
  get tree() {
    return this.mTree;
  },
  mFilterList: null,
  get filterList() {
    return this.mFilterList;
  },
  set filterList(val) {
    if (this.mTree)
      this.mTree.beginUpdateBatch();
    if (this.selection) {
      this.selection.clearSelection();
      this.selection.currentIndex = -1;
    }
    this.mFilterList = val;
    if (this.mTree) {
      this.mTree.scrollToRow(0);
      this.mTree.endUpdateBatch();
    }
  },
  /* nsITreeView methods */
  get rowCount() {
    return this.mFilterList.filterCount;
  },
  selection: null,
  getRowProperties: function getRowProperties(index, properties) {
    if (this.mFilterList.getFilterAt(index).enabled)
      properties.AppendElement(this.mEnabledAtom);
  },
  getCellProperties: function getCellProperties(row, col, properties) {
    if (this.mFilterList.getFilterAt(row).enabled)
      properties.AppendElement(this.mEnabledAtom);
  },
  getColumnProperties: function getColumnProperties(col, properties) {},
  isContainer: function isContainer(index) { return false; },
  isContainerOpen: function isContainerOpen(index) { return false; },
  isContainerEmpty: function isContainerEmpty(index) { return false; },
  isSeparator: function isSeparator(index) { return false; },
  isSorted: function isSorted() { return false; },
  canDrop: function canDrop(index, orientation) { return false; },
  drop: function drop(index, orientation) {},
  getParentIndex: function getParentIndex(index) { return -1; },
  hasNextSibling: function hasNextSibling(rowIndex, afterIndex) { return false; },
  getLevel: function getLevel(index) { return 0; },
  getImageSrc: function getImageSrc(row, col) { return null; },
  getProgressMode: function getProgressMode(row, col) { return 0; },
  getCellValue: function getCellValue(row, col) { return null; },
  getCellText: function getCellText(row, col) {
    return this.mFilterList.getFilterAt(row).filterName;
  },
  setTree: function setTree(tree) {
    this.mTree = tree;
  },
  toggleOpenState: function toggleOpenState(index) {},
  cycleHeader: function cycleHeader(col) {},
  selectionChanged: function selectionChanged() {},
  cycleCell: function cycleCell(row, col) {
    if (toggleFilter(row))
      this.mTree.invalidateCell(row, col);
  },
  isEditable: function isEditable(row, col) { return false; }, // XXX Fix me!
  isSelectable: function isSelectable(row, col) { return false; },
  setCellValue: function setCellValue(row, col, value) {},
  setCellText: function setCellText(row, col, value) { /* XXX Write me */ },
  performAction: function performAction(action) {},
  performActionOnRow: function performActionOnRow(action, row) {},
  performActionOnCell: function performActionOnCell(action, row, col) {}
}

const nsMsgFilterMotion = Components.interfaces.nsMsgFilterMotion;

function onLoad()
{
    setHelpFileURI("chrome://communicator/locale/help/suitehelp.rdf");
    gFilterListMsgWindow = Components.classes["@mozilla.org/messenger/msgwindow;1"].createInstance(Components.interfaces.nsIMsgWindow);
    gFilterListMsgWindow.domWindow = window; 
    gFilterListMsgWindow.rootDocShell.appType = Components.interfaces.nsIDocShell.APP_TYPE_MAIL;   
    gFilterListMsgWindow.statusFeedback = gStatusFeedback;

    gFilterBundle = document.getElementById("bundle_filter");
    gFilterTree = document.getElementById("filterTree");

    gEditButton = document.getElementById("editButton");
    gDeleteButton = document.getElementById("deleteButton");
    gReorderUpButton = document.getElementById("reorderUpButton");
    gReorderDownButton = document.getElementById("reorderDownButton");
    gRunFiltersFolderPickerLabel = document.getElementById("folderPickerPrefix");
    gRunFiltersFolderPicker = document.getElementById("runFiltersFolder");
    gRunFiltersButton = document.getElementById("runFiltersButton");
    gStatusBar = document.getElementById("statusbar-icon");
    gStatusText = document.getElementById("statusText");

    gFilterTree.view = gFilterTreeView;

    // get the selected server if it can have filters.
    var firstItem = getSelectedServerForFilters();

    // if the selected server cannot have filters, get the default server
    // if the default server cannot have filters, check all accounts
    // and get a server that can have filters.
    if (!firstItem)
      firstItem = getServerThatCanHaveFilters();

    if (firstItem)
      selectServer(firstItem);
    else
      updateButtons();

    // Focus the list.
    gFilterTree.focus();

    Services.obs.addObserver(onFilterClose,
                             "quit-application-requested", false);

    top.controllers.insertControllerAt(0, gFilterController);
}

/*
function onCancel()
{
    var firstItem = getSelectedServerForFilters();
    if (!firstItem)
        firstItem = getServerThatCanHaveFilters();
    
    if (firstItem) {
        var resource = gRDF.GetResource(firstItem);
        var msgFolder = resource.QueryInterface(Components.interfaces.nsIMsgFolder);
        if (msgFolder)
        {
           msgFolder.ReleaseDelegate("filter");
           msgFolder.setFilterList(null);
           try
           {
              //now find Inbox
              const kInboxFlag = Components.interfaces.nsMsgFolderFlags.Inbox;
              var inboxFolder = msgFolder.getFolderWithFlags(kInboxFlag);
              inboxFolder.setFilterList(null);
           }
           catch(ex)
           {
             dump ("ex " +ex + "\n");
           }
        }
    }
    window.close();
}
*/

function onFilterServerClick(selection)
{
    var itemURI = selection.getAttribute('itemUri');

    if (itemURI == gCurrentServerURI)
      return;

    // Save the current filters to disk before switching because
    // the dialog may be closed and we'll lose current filters.
    var filterList = currentFilterList();
    if (filterList) 
      filterList.saveToDefaultFile();

    selectServer(itemURI);
}

function CanRunFiltersAfterTheFact(aServer)
{
  // filter after the fact is implement using search
  // so if you can't search, you can't filter after the fact
  return aServer.canSearchMessages;
}

// roots the tree at the specified server
function setServer(uri)
{
   if (uri == gCurrentServerURI)
     return;

   var resource = gRDF.GetResource(uri);
   var msgFolder = resource.QueryInterface(Components.interfaces.nsIMsgFolder);

   //Calling getFilterList will detect any errors in rules.dat, backup the file, and alert the user
   gFilterTreeView.filterList = msgFolder.getEditableFilterList(gFilterListMsgWindow);

   // Select the first item in the list, if there is one.
   if (gFilterTreeView.rowCount)
     gFilterTreeView.selection.select(0);

   // this will get the deferred to account root folder, if server is deferred
   msgFolder = msgFolder.server.rootMsgFolder;
   var rootFolderUri = msgFolder.URI;

   // root the folder picker to this server
   gRunFiltersFolderPicker.setAttribute("ref", rootFolderUri);
 
   // run filters after the fact not supported by news
   if (CanRunFiltersAfterTheFact(msgFolder.server)) {
     gRunFiltersFolderPicker.removeAttribute("hidden");
     gRunFiltersButton.removeAttribute("hidden");
     gRunFiltersFolderPickerLabel.removeAttribute("hidden");

     // for POP3 and IMAP, select the first folder, which is the INBOX
     gRunFiltersFolderPicker.selectedIndex = 0;
   }
   else {
     gRunFiltersFolderPicker.setAttribute("hidden", "true");
     gRunFiltersButton.setAttribute("hidden", "true");
     gRunFiltersFolderPickerLabel.setAttribute("hidden", "true");
   }

   // Get the first folder uri for this server. INBOX for
   // imap and pop accts and 1st news group for news.
   var firstFolderURI = getFirstFolderURI(msgFolder);
   SetFolderPicker(firstFolderURI, "runFiltersFolder");
   updateButtons();

   gCurrentServerURI = uri;
}

function toggleFilter(index)
{
    var filter = getFilter(index);
    if (filter.unparseable)
    {
      Services.prompt.alert(window, null,
                            gFilterBundle.getString("cannotEnableFilter"));
      return false;
    }
    filter.enabled = !filter.enabled;
    return true;
}

// sets up the menulist and the gFilterTree
function selectServer(uri)
{
    // update the server menu
    var serverMenu = document.getElementById("serverMenu");
    
    var resource = gRDF.GetResource(uri);
    var msgFolder = resource.QueryInterface(Components.interfaces.nsIMsgFolder);

    // XXX todo
    // See msgFolderPickerOverlay.js, SetFolderPicker()
    // why do we have to do this?  seems like a hack to work around a bug.
    // the bug is that the (deep) content isn't there
    // and so this won't work:
    //
    //   var menuitems = serverMenu.getElementsByAttribute("id", uri);
    //   serverMenu.selectedItem = menuitems[0];
    //
    // we might need help from a XUL template expert to help out here.
    // see bug #XXXXXX
    serverMenu.setAttribute("label", msgFolder.name);
    serverMenu.setAttribute("uri",uri);
    serverMenu.setAttribute("IsServer", msgFolder.isServer);
    serverMenu.setAttribute("IsSecure", msgFolder.server.isSecure);
    serverMenu.setAttribute("ServerType", msgFolder.server.type);

    setServer(uri);
}

function getFilter(index)
{
  return gFilterTreeView.filterList.getFilterAt(index);
}

function currentFilter()
{
  var currentIndex = gFilterTree.currentIndex;
  return currentIndex == -1 ? null : getFilter(currentIndex);
}

function currentFilterList()
{
  return gFilterTreeView.filterList;
}

function onFilterSelect(event)
{
    updateButtons();
}

function onEditFilter() 
{
  var selectedFilter = currentFilter();
  var curFilterList = currentFilterList();
  var args = {filter: selectedFilter, filterList: curFilterList};

  window.openDialog("chrome://messenger/content/FilterEditor.xul", "FilterEditor", "chrome,modal,titlebar,resizable,centerscreen", args);

  // The focus change will cause a repaint of the row updating any name change
}

function onNewFilter(emailAddress)
{
  var curFilterList = currentFilterList();
  var args = {filterList: curFilterList, refresh: false};
  
  window.openDialog("chrome://messenger/content/FilterEditor.xul", "FilterEditor", "chrome,modal,titlebar,resizable,centerscreen", args);

  if (args.refresh)
  {
    gFilterTreeView.tree.rowCountChanged(0, 1);
    gFilterTree.view.selection.select(0);
  }
}

function onDeleteFilter()
{
  var filterList = currentFilterList();
  if (!filterList)
    return;

  var sel = gFilterTree.view.selection;
  var selCount = sel.getRangeCount();
  if (!selCount || 
      Services.prompt.confirmEx(window, null, 
                        gFilterBundle.getString("deleteFilterConfirmation"),
                        Services.prompt.STD_YES_NO_BUTTONS,
                        '', '', '', '', {}))
    return;

  for (var i = selCount - 1; i >= 0; --i) {
    var start = {}, end = {};
    sel.getRangeAt(i, start, end);
    for (var j = end.value; j >= start.value; --j) {
      var curFilter = getFilter(j);
      if (curFilter)
        filterList.removeFilter(curFilter);
    }
    gFilterTreeView.tree.rowCountChanged(start.value, start.value - end.value - 1);
  }
}

function onUp(event)
{
    moveCurrentFilter(nsMsgFilterMotion.up);
}

function onDown(event)
{
    moveCurrentFilter(nsMsgFilterMotion.down);
}

function viewLog()
{
  var filterList = currentFilterList();
  var args = {filterList: filterList};

  window.openDialog("chrome://messenger/content/viewLog.xul", "FilterLog", "chrome,modal,titlebar,resizable,centerscreen", args);
}

function onFilterUnload()
{
  // make sure to save the filter to disk
  var filterList = currentFilterList();
  if (filterList) 
    filterList.saveToDefaultFile();

  Services.obs.removeObserver(onFilterClose, "quit-application-requested");
  top.controllers.removeController(gFilterController);
}

function onFilterClose(aCancelQuit, aTopic, aData)
{
  if (aTopic == "quit-application-requested" &&
      aCancelQuit instanceof Components.interfaces.nsISupportsPRBool &&
      aCancelQuit.data)
    return false;

  if (gRunFiltersButton.getAttribute("label") == gRunFiltersButton.getAttribute("stoplabel")) {
    var promptTitle = gFilterBundle.getString("promptTitle");
    var promptMsg = gFilterBundle.getString("promptMsg");;
    var stopButtonLabel = gFilterBundle.getString("stopButtonLabel");
    var continueButtonLabel = gFilterBundle.getString("continueButtonLabel");

    if (Services.prompt.confirmEx(window, promptTitle, promptMsg,
        (Services.prompt.BUTTON_TITLE_IS_STRING *
         Services.prompt.BUTTON_POS_0) +
        (Services.prompt.BUTTON_TITLE_IS_STRING * Services.prompt.BUTTON_POS_1),
        continueButtonLabel, stopButtonLabel, null, null, {value:0}) == 0) {
      if (aTopic == "quit-application-requested")
        aCancelQuit.data = true;
      return false;
    }
    gFilterListMsgWindow.StopUrls();
  }

  return true;
}

function runSelectedFilters()
{
  // if run button has "stop" label, do stop.
  if (gRunFiltersButton.getAttribute("label") == gRunFiltersButton.getAttribute("stoplabel")) {
    gFilterListMsgWindow.StopUrls();
    return;
  }
  
  var folderURI = gRunFiltersFolderPicker.getAttribute("uri");
  var resource = gRDF.GetResource(folderURI);
  var msgFolder = resource.QueryInterface(Components.interfaces.nsIMsgFolder);
  var filterService = Components.classes["@mozilla.org/messenger/services/filters;1"].getService(Components.interfaces.nsIMsgFilterService);
  var filterList = filterService.getTempFilterList(msgFolder);
  var folders = Components.classes["@mozilla.org/supports-array;1"].createInstance(Components.interfaces.nsISupportsArray);
  folders.AppendElement(msgFolder);

  // make sure the tmp filter list uses the real filter list log stream
  filterList.logStream = currentFilterList().logStream;
  filterList.loggingEnabled = currentFilterList().loggingEnabled;
  var index = 0, sel = gFilterTree.view.selection;
  for (var i = 0; i < sel.getRangeCount(); i++) {
    var start = {}, end = {};
    sel.getRangeAt(i, start, end);
    for (var j = start.value; j <= end.value; j++) {
      var curFilter = getFilter(j);
      if (curFilter)
        filterList.insertFilterAt(index++, curFilter);
    }
  }

  filterService.applyFiltersToFolders(filterList, folders, gFilterListMsgWindow);
}

function moveCurrentFilter(motion)
{
    var filterList = currentFilterList();
    var filter = currentFilter();
    if (!filterList || !filter) 
      return;

    filterList.moveFilter(filter, motion);
    if (motion == nsMsgFilterMotion.up)
      gFilterTree.view.selection.select(gFilterTree.currentIndex - 1);
    else
      gFilterTree.view.selection.select(gFilterTree.currentIndex + 1);

    gFilterTree.treeBoxObject.ensureRowIsVisible(gFilterTree.currentIndex);
}

function updateButtons()
{
    var numFiltersSelected = gFilterTree.view.selection.count;
    var oneFilterSelected = (numFiltersSelected == 1);

    var filter = currentFilter();
    // "edit" only enabled when one filter selected or if we couldn't parse the filter
    gEditButton.disabled = !oneFilterSelected || filter.unparseable;
    
    // "delete" only disabled when no filters are selected
    gDeleteButton.disabled = !numFiltersSelected;

    // we can run multiple filters on a folder
    // so only disable this UI if no filters are selected
    gRunFiltersFolderPickerLabel.disabled = !numFiltersSelected;
    gRunFiltersFolderPicker.disabled = !numFiltersSelected;
    gRunFiltersButton.disabled = !numFiltersSelected;

    // "up" enabled only if one filter selected, and it's not the first
    gReorderUpButton.disabled = !(oneFilterSelected && gFilterTree.currentIndex > 0);
    // "down" enabled only if one filter selected, and it's not the last
    gReorderDownButton.disabled = !(oneFilterSelected && gFilterTree.currentIndex < gFilterTree.view.rowCount-1);
}

/**
  * get the selected server if it can have filters
  */
function getSelectedServerForFilters()
{
    var firstItem = null;
    var args = window.arguments;
    var selectedFolder = args[0].folder;
  
    if (args && args[0] && selectedFolder)
    {
        var msgFolder = selectedFolder.QueryInterface(Components.interfaces.nsIMsgFolder);
        try
        {
            var rootFolder = msgFolder.server.rootFolder;
            if (rootFolder.isServer)
            {
                var server = rootFolder.server;

                if (server.canHaveFilters)
                {
                    // for news, select the news folder
                    // for everything else, select the folder's server
                    firstItem = (server.type == "nntp") ? msgFolder.URI : rootFolder.URI;
                }
            }
        }
        catch (ex)
        {
        }
    }

    return firstItem;
}

/** if the selected server cannot have filters, get the default server
  * if the default server cannot have filters, check all accounts
  * and get a server that can have filters.
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
        firstItem = defaultIncomingServer.serverURI;
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
                firstItem = currentServer.serverURI;
                break;
            }
        }
    }

    return firstItem;
}

function onFilterDoubleClick(event)
{
    // we only care about button 0 (left click) events
    if (event.button != 0)
      return;

    var row = {}, col = {}, childElt = {};
    var filterTree = document.getElementById("filterTree");
    filterTree.treeBoxObject.getCellAt(event.clientX, event.clientY, row, col, childElt);
    if (row.value == -1 || row.value > filterTree.view.rowCount-1 || event.originalTarget.localName != "treechildren") {
      // double clicking on a non valid row should not open the edit filter dialog
      return;
    }

    // if the cell is in a "cycler" column (the enabled column)
    // don't open the edit filter dialog with the selected filter
    if (!col.value.cycler)
      onEditFilter();
}

function onFilterTreeKeyPress(event)
{
  if (event.charCode == KeyEvent.DOM_VK_SPACE)
  {
    var rangeCount = gFilterTree.view.selection.getRangeCount();
    for (var i = 0; i < rangeCount; ++i)
    {
      var start = {}, end = {};
      gFilterTree.view.selection.getRangeAt(i, start, end);
      for (var k = start.value; k <= end.value; ++k)
        toggleFilter(k);
    }
    gFilterTree.view.selection.invalidateSelection();
  }
  else switch (event.keyCode)
  {
    case KeyEvent.DOM_VK_DELETE:
      if (!gDeleteButton.disabled)
        onDeleteFilter();
      break;
    case KeyEvent.DOM_VK_ENTER:
    case KeyEvent.DOM_VK_RETURN:
      if (!gEditButton.disabled)
        onEditFilter();
      break;
  }
}

function doHelpButton()
{
  openHelp("mail-filters");
}

/**
  * For a given server folder, get the uri for the first folder. For imap
  * and pop it's INBOX and it's the very first group for news accounts.
  */
function getFirstFolderURI(msgFolder)
{
  // Sanity check.
  if (! msgFolder.isServer)
    return msgFolder.URI;

  try {
    // Find Inbox for imap and pop
    if (msgFolder.server.type != "nntp")
    {
      const nsMsgFolderFlags = Components.interfaces.nsMsgFolderFlags;
      var inboxFolder = msgFolder.getFolderWithFlags(nsMsgFolderFlags.Inbox);
      if (inboxFolder)
        return inboxFolder.URI;
      else
        // If inbox does not exist then use the server uri as default.
        return msgFolder.URI;
    }
    else
      // XXX TODO: For news, we should find the 1st group/folder off the news groups. For now use server uri.
      return msgFolder.URI;
  }
  catch (ex) {
    dump(ex + "\n");
  }
  return msgFolder.URI;
}

var gFilterController =
{
  supportsCommand: function(aCommand)
  {
    return aCommand == "cmd_selectAll";
  },

  isCommandEnabled: function(aCommand)
  {
    return aCommand == "cmd_selectAll";
  },

  doCommand: function(aCommand)
  {
    if (aCommand == "cmd_selectAll")
      gFilterTree.view.selection.selectAll();
  },

  onEvent: function(aEvent)
  {
  }
};
