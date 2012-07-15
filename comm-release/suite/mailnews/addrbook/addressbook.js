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
 * The Original Code is Mozilla Addressbook.
 *
 * The Initial Developer of the Original Code is
 * Netscape Communications Corp.
 * Portions created by the Initial Developer are Copyright (C) 1999-2001
 * the Initial Developer. All Rights Reserved.
 *
 * Original Author:
 *   Paul Hangas <hangas@netscape.com>
 *
 * Contributor(s):
 *   Seth Spitzer <sspitzer@netscape.com>
 *   Mark Banner <mark@standard8.demon.co.uk>
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

Components.utils.import("resource:///modules/mailServices.js");

const nsIAbListener = Components.interfaces.nsIAbListener;
const kPrefMailAddrBookLastNameFirst = "mail.addr_book.lastnamefirst";
const kPersistCollapseMapStorage = "directoryTree.json";

var cvPrefs = 0;
var gSearchTimer = null;
var gStatusText = null;
var gQueryURIFormat = null;
var gSearchInput;
var gSearchBox;
var gCardViewBox;
var gCardViewBoxEmail1;

// Constants that correspond to choices
// in Address Book->View -->Show Name as
const kDisplayName = 0;
const kLastNameFirst = 1;
const kFirstNameFirst = 2;
const kLDAPDirectory = 0; // defined in nsDirPrefs.h
const kPABDirectory  = 2; // defined in nsDirPrefs.h

function OnUnloadAddressBook()
{
  MailServices.ab.removeAddressBookListener(gDirectoryTreeView);

  // Shutdown the tree view - this will also save the open/collapsed
  // state of the tree view to a JSON file.
  gDirectoryTreeView.shutdown(kPersistCollapseMapStorage);

  CloseAbView();
}

var gAddressBookAbViewListener = {
  onSelectionChanged: function() {
    ResultsPaneSelectionChanged();
  },
  onCountChanged: function(total) {
    SetStatusText(total);
  }
};

function GetAbViewListener()
{
  return gAddressBookAbViewListener;
}

function OnLoadAddressBook()
{
  gSearchInput = document.getElementById("searchInput");

  verifyAccounts(null, false); 	// this will do migration, if we need to.

  InitCommonJS();

  UpgradeAddressBookResultsPaneUI("mailnews.ui.addressbook_results.version");

  GetCurrentPrefs();

  // FIX ME - later we will be able to use onload from the overlay
  OnLoadCardView();

  // Before and after callbacks for the customizeToolbar code
  var abToolbox = getAbToolbox();
  abToolbox.customizeInit = AbToolboxCustomizeInit;
  abToolbox.customizeDone = AbToolboxCustomizeDone;
  abToolbox.customizeChange = AbToolboxCustomizeChange;

  // Initialize the Address Book tree view
  gDirectoryTreeView.init(gDirTree, kPersistCollapseMapStorage);

  SelectFirstAddressBook();

  // if the pref is locked disable the menuitem New->LDAP directory
  if (gPrefs.prefIsLocked("ldap_2.disable_button_add"))
    document.getElementById("addLDAP").setAttribute("disabled", "true");

  // Add a listener, so we can switch directories if the current directory is
  // deleted. This listener cares when a directory (= address book), or a
  // directory item is/are removed. In the case of directory items, we are
  // only really interested in mailing list changes and not cards but we have
  // to have both.
  MailServices.ab.addAddressBookListener(gDirectoryTreeView, nsIAbListener.all);

  gDirTree.controllers.appendController(DirPaneController);

  // Ensure we don't load xul error pages into the main window
  window.QueryInterface(Components.interfaces.nsIInterfaceRequestor)
        .getInterface(Components.interfaces.nsIWebNavigation)
        .QueryInterface(Components.interfaces.nsIDocShell)
        .useErrorPages = false;
}

function GetCurrentPrefs()
{
	// prefs
	if ( cvPrefs == 0 )
		cvPrefs = new Object;

	cvPrefs.prefs = gPrefs;
	
	// check "Show Name As" menu item based on pref
	var menuitemID;
	switch (gPrefs.getIntPref(kPrefMailAddrBookLastNameFirst))
	{
		case kFirstNameFirst:
			menuitemID = 'firstLastCmd';
			break;
		case kLastNameFirst:
			menuitemID = 'lastFirstCmd';
			break;
		case kDisplayName:
		default:
			menuitemID = 'displayNameCmd';
			break;
	}

	var menuitem = top.document.getElementById(menuitemID);
	if ( menuitem )
		menuitem.setAttribute('checked', 'true');

  // show phonetic fields if indicated by the pref
  if (GetLocalizedStringPref("mail.addr_book.show_phonetic_fields") == "true")
    document.getElementById("cmd_SortBy_PhoneticName")
            .setAttribute("hidden", "false");
}


function SetNameColumn(cmd)
{
  var prefValue;

  switch (cmd)
  {
  case 'firstLastCmd':
    prefValue = kFirstNameFirst;
    break;
  case 'lastFirstCmd':
    prefValue = kLastNameFirst;
    break;
  case 'displayNameCmd':
    prefValue = kDisplayName;
    break;
  }

  cvPrefs.prefs.setIntPref(kPrefMailAddrBookLastNameFirst, prefValue);
}

function CommandUpdate_AddressBook()
{
  goUpdateCommand('cmd_delete');
  goUpdateCommand('button_delete');
  goUpdateCommand('cmd_newlist');
}

function ResultsPaneSelectionChanged()
{
  UpdateCardView();
}

function UpdateCardView()
{
  var cards = GetSelectedAbCards();

  // display the selected card, if exactly one card is selected.
  // either no cards, or more than one card is selected, clear the pane.
  if (cards.length == 1)
    OnClickedCard(cards[0])
  else 
    ClearCardViewPane();
}

function OnClickedCard(card)
{ 
  if (card) 
    DisplayCardViewPane(card);
  else
    ClearCardViewPane();
}

function AbClose()
{
  top.close();
}

function AbPrintCardInternal(doPrintPreview, msgType)
{
  var selectedItems = GetSelectedAbCards();
  var numSelected = selectedItems.length;

  if (!numSelected)
    return;

  var uri = GetSelectedDirectory();
  if (!uri)
    return;

   var statusFeedback;
   statusFeedback = Components.classes["@mozilla.org/messenger/statusfeedback;1"].createInstance();
   statusFeedback = statusFeedback.QueryInterface(Components.interfaces.nsIMsgStatusFeedback);

   var selectionArray = new Array(numSelected);

   var totalCard = 0;

   for (var i = 0; i < numSelected; i++)
   {
     var card = selectedItems[i];
     var printCardUrl = CreatePrintCardUrl(card);
     if (printCardUrl)
     {
        selectionArray[totalCard++] = printCardUrl;
     }
  }

  printEngineWindow = window.openDialog("chrome://messenger/content/msgPrintEngine.xul",
                                         "",
                                         "chrome,dialog=no,all",
                                          totalCard, selectionArray, statusFeedback, 
                                          doPrintPreview, msgType);

  return;
}

function AbPrintCard()
{
  AbPrintCardInternal(false, Components.interfaces.nsIMsgPrintEngine.MNAB_PRINT_AB_CARD);
}

function AbPrintPreviewCard()
{
  AbPrintCardInternal(true, Components.interfaces.nsIMsgPrintEngine.MNAB_PRINTPREVIEW_AB_CARD);
}

function CreatePrintCardUrl(card)
{
  return "data:application/xml;base64," + card.translateTo("base64xml");
}

function AbPrintAddressBookInternal(doPrintPreview, msgType)
{
  var uri = GetSelectedDirectory();
  if (!uri)
    return;

  var statusFeedback;
	statusFeedback = Components.classes["@mozilla.org/messenger/statusfeedback;1"].createInstance();
	statusFeedback = statusFeedback.QueryInterface(Components.interfaces.nsIMsgStatusFeedback);

  /*
    turn "moz-abmdbdirectory://abook.mab" into
    "addbook://moz-abmdbdirectory/abook.mab?action=print"
   */

  var abURIArr = uri.split("://");
  var printUrl = "addbook://" + abURIArr[0] + "/" + abURIArr[1] + "?action=print"

	printEngineWindow = window.openDialog("chrome://messenger/content/msgPrintEngine.xul",
										"",
										"chrome,dialog=no,all",
										1, [printUrl], statusFeedback, doPrintPreview, msgType);

	return;
}

function AbPrintAddressBook()
{
  AbPrintAddressBookInternal(false, Components.interfaces.nsIMsgPrintEngine.MNAB_PRINT_ADDRBOOK);
}

function AbPrintPreviewAddressBook()
{
  AbPrintAddressBookInternal(true, Components.interfaces.nsIMsgPrintEngine.MNAB_PRINTPREVIEW_ADDRBOOK);
}

function AbExport()
{
  try {
    var selectedABURI = GetSelectedDirectory();
    if (!selectedABURI) return;
    
    var directory = GetDirectoryFromURI(selectedABURI);
    MailServices.ab.exportAddressBook(window, directory);
  }
  catch (ex) {
    var message;
    switch (ex.result) {
      case Components.results.NS_ERROR_FILE_ACCESS_DENIED:
        message = gAddressBookBundle.getString("failedToExportMessageFileAccessDenied");
        break;
      case Components.results.NS_ERROR_FILE_NO_DEVICE_SPACE:
        message = gAddressBookBundle.getString("failedToExportMessageNoDeviceSpace");
        break;
      default:
        message = ex.message;
        break;
    }

    Services.prompt.alert(window,
      gAddressBookBundle.getString("failedToExportTitle"), 
      message);
  }
}

function SetStatusText(total)
{
  if (!gStatusText)
    gStatusText = document.getElementById('statusText');

  try {
    var statusText;

    if (gSearchInput.value) {
      if (total == 0)
        statusText = gAddressBookBundle.getString("noMatchFound");
      else
      {
        if (total == 1)
          statusText = gAddressBookBundle.getString("matchFound");
        else  
          statusText = gAddressBookBundle.getFormattedString("matchesFound", [total]);
      }
    } 
    else
      statusText = gAddressBookBundle.getFormattedString("totalContactStatus", [gAbView.directory.dirName, total]);   

    gStatusText.setAttribute("label", statusText);
  }
  catch(ex) {
    dump("failed to set status text:  " + ex + "\n");
  }
}

function AbResultsPaneDoubleClick(card)
{
  AbEditCard(card);
}

function onAdvancedAbSearch()
{
  var selectedABURI = GetSelectedDirectory();
  if (!selectedABURI) return;

  var existingSearchWindow = Services.wm.getMostRecentWindow("mailnews:absearch");
  if (existingSearchWindow)
    existingSearchWindow.focus();
  else
    window.openDialog("chrome://messenger/content/ABSearchDialog.xul", "", 
                      "chrome,resizable,status,centerscreen,dialog=no", 
                      {directory: selectedABURI});
}

function onEnterInSearchBar()
{
  ClearCardViewPane();  

  if (!gQueryURIFormat)
    gQueryURIFormat = GetLocalizedStringPref("mail.addr_book.quicksearchquery.format");

  var searchURI = GetSelectedDirectory();
  if (!searchURI) return;

  /*
   XXX todo, handle the case where the LDAP url
   already has a query, like 
   moz-abldapdirectory://nsdirectory.netscape.com:389/ou=People,dc=netscape,dc=com?(or(Department,=,Applications))
  */
  if (gSearchInput.value != "") {
    // replace all instances of @V with the escaped version
    // of what the user typed in the quick search text input
    searchURI += gQueryURIFormat.replace(/@V/g, encodeURIComponent(gSearchInput.value));
  }

  SetAbView(searchURI);
  
  // XXX todo 
  // this works for synchronous searches of local addressbooks, 
  // but not for LDAP searches
  SelectFirstCard();
}

function SwitchPaneFocus(event)
{
  var focusedElement    = WhichPaneHasFocus();
  var cardViewBox       = GetCardViewBox();
  var cardViewBoxEmail1 = GetCardViewBoxEmail1();
  var searchBox         = GetSearchBox();
  var dirTree           = GetDirTree();

  if (event && event.shiftKey)
  {
    if (focusedElement == gAbResultsTree && searchBox.getAttribute('hidden') != 'true')
      searchInput.focus();
    else if ((focusedElement == gAbResultsTree || focusedElement == searchBox) && !IsDirPaneCollapsed())
      dirTree.focus();
    else if (focusedElement != cardViewBox && !IsCardViewAndAbResultsPaneSplitterCollapsed())
    {
      if(cardViewBoxEmail1)
        cardViewBoxEmail1.focus();
      else
        cardViewBox.focus();    
    }
    else 
      gAbResultsTree.focus();
  }
  else
  {
    if (focusedElement == searchBox)
      gAbResultsTree.focus();
    else if (focusedElement == gAbResultsTree && !IsCardViewAndAbResultsPaneSplitterCollapsed())
    {
      if(cardViewBoxEmail1)
        cardViewBoxEmail1.focus();
      else
        cardViewBox.focus();    
    }
    else if (focusedElement != dirTree && !IsDirPaneCollapsed())
      dirTree.focus();
    else if (searchBox.getAttribute('hidden') != 'true')
      gSearchInput.focus();
    else
      gAbResultsTree.focus();
  }
}

function WhichPaneHasFocus()
{
  var cardViewBox       = GetCardViewBox();
  var searchBox         = GetSearchBox();
  var dirTree           = GetDirTree();
    
  var currentNode = top.document.commandDispatcher.focusedElement;
  while (currentNode)
  {
    var nodeId = currentNode.getAttribute('id');

    if(currentNode == gAbResultsTree ||
       currentNode == cardViewBox ||
       currentNode == searchBox ||
       currentNode == dirTree)
      return currentNode;

    currentNode = currentNode.parentNode;
  }

  return null;
}

function GetDirTree()
{
  if (!gDirTree)
    gDirTree = document.getElementById('dirTree');
  return gDirTree;
}

function GetSearchBox()
{
  if (!gSearchBox)
    gSearchBox = document.getElementById('searchBox');
  return gSearchBox;
}

function GetCardViewBox()
{
  if (!gCardViewBox)
    gCardViewBox = document.getElementById('CardViewBox');
  return gCardViewBox;
}

function GetCardViewBoxEmail1()
{
  if (!gCardViewBoxEmail1)
  {
    try {
      gCardViewBoxEmail1 = document.getElementById('cvEmail1');
    }
    catch (ex) {
      gCardViewBoxEmail1 = null;
    }
  }
  return gCardViewBoxEmail1;
}

function IsDirPaneCollapsed()
{
  var dirPaneBox = GetDirTree().parentNode;
  return dirPaneBox.getAttribute("collapsed") == "true" ||
         dirPaneBox.getAttribute("hidden") == "true";
}

function IsCardViewAndAbResultsPaneSplitterCollapsed()
{
  var cardViewBox = document.getElementById('CardViewOuterBox');
  try {
    return (cardViewBox.getAttribute("collapsed") == "true");
  }
  catch (ex) {
    return false;
  }
}

function LaunchUrl(url)
{
  // Doesn't matter if this bit fails, window.location contains its own prompts
  try {
    window.location = url;
  }
  catch (ex) {}
}

function AbIMSelected()
{
  var cards = GetSelectedAbCards();
  var count = cards.length;

  var screennames;
  var screennameCount = 0;

  for (var i=0;i<count;i++) {
    var screenname = cards[i].getProperty("_AimScreenName", "");
    if (screenname) {
      if (screennameCount == 0)
        screennames = screenname;
      else
        screennames += "," + screenname;

      screennameCount++
    }
  }

  var url = "aim:";

  if (screennameCount == 0)
    url += "goim";
  else if (screennameCount == 1)
    url += "goim?screenname=" + screennames;
  else {
    url += "SendChatInvite?listofscreennames=" + screennames;
    url += "&message=" + gAddressBookBundle.getString("joinMeInThisChat");
  }

  LaunchUrl(url);
}

function getAbToolbox()
{
  return document.getElementById("ab-toolbox");
}

function AbToolboxCustomizeInit()
{
  toolboxCustomizeInit("ab-menubar");
}

function AbToolboxCustomizeDone(aToolboxChanged)
{
  toolboxCustomizeDone("ab-menubar", getAbToolbox(), aToolboxChanged);
}

function AbToolboxCustomizeChange(event)
{
  toolboxCustomizeChange(getAbToolbox(), event);
}
