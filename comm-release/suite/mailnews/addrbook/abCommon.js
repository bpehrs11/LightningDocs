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

var gDirTree = 0;
var abList = null;
var gAbResultsTree = null;
var gAbView = null;
var gAddressBookBundle;

var gPrefs = Components.classes["@mozilla.org/preferences-service;1"].getService(Components.interfaces.nsIPrefBranch);
var gHeaderParser = Components.classes["@mozilla.org/messenger/headerparser;1"].getService(Components.interfaces.nsIMsgHeaderParser);

const kDefaultSortColumn = "GeneratedName";
const kDefaultAscending = "ascending";
const kDefaultDescending = "descending";
const kLdapUrlPrefix = "moz-abldapdirectory://";
const kPersonalAddressbookURI = "moz-abmdbdirectory://abook.mab";
const kCollectedAddressbookURI = "moz-abmdbdirectory://history.mab";
// The default image for contacts
var defaultPhotoURI = "chrome://messenger/skin/addressbook/icons/contact-generic.png";

// Controller object for Dir Pane
var DirPaneController =
{
  supportsCommand: function(command)
  {
    switch (command) {
      case "cmd_selectAll":
      case "cmd_delete":
      case "button_delete":
      case "button_edit":
      case "cmd_newlist":
        return true;
      default:
        return false;
    }
  },

  isCommandEnabled: function(command)
  {
    var selectedDir;

    switch (command) {
      case "cmd_selectAll":
        // the gDirTree pane
        // only handles single selection
        // so we forward select all to the results pane
        // but if there is no gAbView
        // don't bother sending to the results pane
        return (gAbView != null);
      case "cmd_delete":
      case "button_delete":
        selectedDir = GetSelectedDirectory();
        if (command == "cmd_delete" && selectedDir) {
          goSetMenuValue(command, GetDirectoryFromURI(selectedDir).isMailList ? "valueList" : "valueAddressBook");
        }

        if (selectedDir &&
            (selectedDir != kPersonalAddressbookURI) &&
            (selectedDir != kCollectedAddressbookURI)) {
          // If the directory is a mailing list, and it is read-only, return
          // false.
          var abDir = GetDirectoryFromURI(selectedDir);
          if (abDir.isMailList && abDir.readOnly)
            return false;

          // If the selected directory is an ldap directory
          // and if the prefs for this directory are locked
          // disable the delete button.
          if (selectedDir.lastIndexOf(kLdapUrlPrefix, 0) == 0)
          {
            var disable = false;
            try {
              var prefName = selectedDir.substr(kLdapUrlPrefix.length);
              disable = gPrefs.getBoolPref(prefName + ".disable_delete");
            }
            catch(ex) {
              // if this preference is not set its ok.
            }
            if (disable)
              return false;
          }
          return true;
        }
        else
          return false;
      case "button_edit":
        return (GetSelectedDirectory() != null);
      case "cmd_newlist":
        selectedDir = GetSelectedDirectory();
        if (selectedDir) {
          var abDir = GetDirectoryFromURI(selectedDir);
          if (abDir) {
            return abDir.supportsMailingLists;
          }
        }
        return false;
      default:
        return false;
    }
  },

  doCommand: function(command)
  {
    switch (command) {
      case "cmd_selectAll":
        SendCommandToResultsPane(command);
        break;
      case "cmd_delete":
      case "button_delete":
        if (gDirTree)
          AbDeleteSelectedDirectory();
        break;
      case "button_edit":
        AbEditSelectedDirectory();
        break;
      case "cmd_newlist":
        AbNewList();
        break;
    }
  },

  onEvent: function(event)
  {
    // on blur events set the menu item texts back to the normal values
    if (event == "blur")
      goSetMenuValue("cmd_delete", "valueDefault");
  }
};

function SendCommandToResultsPane(command)
{
  ResultsPaneController.doCommand(command);

  // if we are sending the command so the results pane
  // we should focus the results pane
  gAbResultsTree.focus();
}

function AbNewLDAPDirectory()
{
  window.openDialog("chrome://messenger/content/addressbook/pref-directory-add.xul",
                    "",
                    "chrome,modal,resizable=no,centerscreen",
                    null);
}

function AbNewAddressBook()
{
  window.openDialog("chrome://messenger/content/addressbook/abAddressBookNameDialog.xul",
                    "",
                    "chrome,modal,resizable=no,centerscreen",
                    null);
}

function AbEditSelectedDirectory()
{
  if (gDirectoryTreeView.selection.count == 1) {
    var selecteduri = GetSelectedDirectory();
    var directory = GetDirectoryFromURI(selecteduri);
    if (directory.isMailList) {
      goEditListDialog(null, selecteduri);
    }
    else {
      window.openDialog(directory.propertiesChromeURI,
                        "",
                        "chrome,modal,resizable=no,centerscreen",
                        { selectedDirectory: directory });
    }
  }
}

function AbDeleteSelectedDirectory()
{
  var selectedABURI = GetSelectedDirectory();
  if (!selectedABURI)
    return;

  AbDeleteDirectory(selectedABURI);
}

function AbDeleteDirectory(aURI)
{
  var promptService = Components.classes["@mozilla.org/embedcomp/prompt-service;1"].getService(Components.interfaces.nsIPromptService);

  var directory = GetDirectoryFromURI(aURI);
  var confirmDeleteMessage;
  var clearPrefsRequired = false;

  if (directory.isMailList)
    confirmDeleteMessage = gAddressBookBundle.getString("confirmDeleteMailingList");
  else {
    // Check if this address book is being used for collection
    if (gPrefs.getCharPref("mail.collect_addressbook") == aURI &&
        (gPrefs.getBoolPref("mail.collect_email_address_outgoing") ||
         gPrefs.getBoolPref("mail.collect_email_address_incoming") ||
         gPrefs.getBoolPref("mail.collect_email_address_newsgroup"))) {
      var brandShortName = document.getElementById("bundle_brand").getString("brandShortName");

      confirmDeleteMessage = gAddressBookBundle.getFormattedString("confirmDeleteCollectionAddressbook", [brandShortName]);
      clearPrefsRequired = true;
    }
    else {
      confirmDeleteMessage = gAddressBookBundle.getString("confirmDeleteAddressbook");
    }
  }

  if (!promptService.confirm(window,
                             gAddressBookBundle.getString(
                                                directory.isMailList ?
                                                "confirmDeleteMailingListTitle" :
                                                "confirmDeleteAddressbookTitle"),
                             confirmDeleteMessage))
    return;

  // First clear all the prefs if required
  if (clearPrefsRequired) {
    gPrefs.setBoolPref("mail.collect_email_address_outgoing", false);
    gPrefs.setBoolPref("mail.collect_email_address_incoming", false);
    gPrefs.setBoolPref("mail.collect_email_address_newsgroup", false);

    // Also reset the displayed value so that we don't get a blank item in the
    // prefs dialog if it gets enabled.
    gPrefs.setCharPref("mail.collect_addressbook", kPersonalAddressbookURI);
  }

  MailServices.ab.deleteAddressBook(aURI);
}

function InitCommonJS()
{
  gDirTree = document.getElementById("dirTree");
  abList = document.getElementById("addressbookList");
  gAddressBookBundle = document.getElementById("bundle_addressBook");
}

function UpgradeAddressBookResultsPaneUI(prefName)
{
  // placeholder in case any new columns get added to the address book
  // var resultsPaneUIVersion = gPrefs.getIntPref(prefName);
}

function AbDelete()
{
  var types = GetSelectedCardTypes();

  if (types == kNothingSelected)
    return;

  var promptService = Components.classes["@mozilla.org/embedcomp/prompt-service;1"].getService(Components.interfaces.nsIPromptService);
  // If at least one mailing list is selected then prompt users for deletion.
  if (types != kCardsOnly)
  {
    var confirmDeleteMessage;
    if (types == kListsAndCards)
      confirmDeleteMessage = gAddressBookBundle.getString("confirmDeleteListsAndContacts");
    else if (types == kMultipleListsOnly)
      confirmDeleteMessage = gAddressBookBundle.getString("confirmDeleteMailingLists");
    else
      confirmDeleteMessage = gAddressBookBundle.getString("confirmDeleteMailingList");
    if (!promptService.confirm(window, null, confirmDeleteMessage))
      return;
  }

  gAbView.deleteSelectedCards();
}

function AbNewCard()
{
  goNewCardDialog(GetSelectedDirectory());
}

function AbEditCard(card)
{
  // Need a card,
  if (!card)
    return;

  if (card.isMailList) {
    goEditListDialog(card, card.mailListURI);
  }
  else {
    goEditCardDialog(GetSelectedDirectory(), card);
  }
}

function AbNewMessage()
{
  var params = Components.classes["@mozilla.org/messengercompose/composeparams;1"].createInstance(Components.interfaces.nsIMsgComposeParams);
  if (params)
  {
    var composeFields = Components.classes["@mozilla.org/messengercompose/composefields;1"].createInstance(Components.interfaces.nsIMsgCompFields);
    if (composeFields)
    {
      params.type = Components.interfaces.nsIMsgCompType.New;
      params.format = Components.interfaces.nsIMsgCompFormat.Default;
      if (DirPaneHasFocus())
        composeFields.to = GetSelectedAddressesFromDirTree();
      else
        composeFields.to = GetSelectedAddresses();
      params.composeFields = composeFields;
      var msgComposeService =
        Components.classes["@mozilla.org/messengercompose;1"]
                  .getService(Components.interfaces.nsIMsgComposeService);
      msgComposeService.OpenComposeWindowWithParams(null, params);
    }
  }
}

function AbCopyAddress()
{
  var cards = GetSelectedAbCards();
  if (!cards)
    return;

  var count = cards.length;
  if (!count)
    return;

  var addresses = cards[0].primaryEmail;
  for (var i = 1; i < count; i++)
    addresses += "," + cards[i].primaryEmail;

  Components.classes["@mozilla.org/widget/clipboardhelper;1"]
            .getService(Components.interfaces.nsIClipboardHelper)
            .copyString(addresses);
}

// XXX todo
// could this be moved into utilityOverlay.js?
function goToggleSplitter( id, elementID )
{
  var splitter = document.getElementById( id );
  var element = document.getElementById( elementID );
  if ( splitter )
  {
    var attribValue = splitter.getAttribute("state") ;
    if ( attribValue == "collapsed" )
    {
      splitter.setAttribute("state", "open" );
      if ( element )
        element.setAttribute("checked","true")
    }
    else
    {
      splitter.setAttribute("state", "collapsed");
      if ( element )
        element.setAttribute("checked","false")
    }
    document.persist(id, 'state');
    document.persist(elementID, 'checked');
  }
}

// Generate a list of cards from the selected mailing list 
// and get a comma separated list of card addresses. If the
// item selected in the directory pane is not a mailing list,
// an empty string is returned. 
function GetSelectedAddressesFromDirTree() 
{
  var addresses = "";

  if (gDirTree.currentIndex >= 0) {
    var directory = gDirectoryTreeView.getDirectoryAtIndex(gDirTree.currentIndex);
    if (directory.isMailList) {
      var listCardsCount = directory.addressLists.length;
      var cards = new Array(listCardsCount);
      for (var i = 0; i < listCardsCount; ++i)
        cards[i] = directory.addressLists
                            .queryElementAt(i, Components.interfaces.nsIAbCard);
      addresses = GetAddressesForCards(cards);
    }
  }

  return addresses;
}

// Generate a comma separated list of addresses from a given
// set of cards.
function GetAddressesForCards(cards)
{
  var addresses = "";

  if (!cards)
    return addresses;

  var count = cards.length;
  for (var i = 0; i < count; ++i) {
    var generatedAddress = GenerateAddressFromCard(cards[i]);
    if (generatedAddress) {
      // If it's not the first address in the list, add a comma separator.
      if (addresses)
        addresses += ",";
      addresses += generatedAddress;
    }
  }

  return addresses;
}


function SelectFirstAddressBook()
{
  gDirectoryTreeView.selection.select(0);

  ChangeDirectoryByURI(GetSelectedDirectory());
  gAbResultsTree.focus();
}

function DirPaneClick(event)
{
  // we only care about left button events
  if (event.button != 0)
    return;

  // if the user clicks on the header / trecol, do nothing
  if (event.originalTarget.localName == "treecol") {
    event.stopPropagation();
    return;
  }

  var searchInput = document.getElementById("searchInput");
  // if there is a searchInput element, and it's not blank 
  // then we need to act like the user cleared the
  // search text
  if (searchInput && searchInput.value) {
    searchInput.value = "";
    onEnterInSearchBar();
  }
}

function DirPaneDoubleClick(event)
{
  // we only care about left button events
  if (event.button != 0)
    return;

  var row = gDirTree.treeBoxObject.getRowAt(event.clientX, event.clientY);
  if (row == -1 || row >= gDirectoryTreeView.rowCount) {
    // double clicking on a non valid row should not open the dir properties dialog
    return;
  }

  if (gDirectoryTreeView.selection &&
      gDirectoryTreeView.selection.count == 1)
    AbEditSelectedDirectory();
}

function DirPaneSelectionChange()
{
  if (gDirectoryTreeView.selection &&
      gDirectoryTreeView.selection.count == 1) {
    ChangeDirectoryByURI(GetSelectedDirectory());
  }
}

function ChangeDirectoryByURI(uri)
{
  if (!uri)
    uri = kPersonalAddressbookURI;

  SetAbView(uri);
  
  // only select the first card if there is a first card
  if (gAbView && gAbView.getCardFromRow(0))
    SelectFirstCard();
  else
    // the selection changes if we were switching directories.
    ResultsPaneSelectionChanged()
}

function AbNewList()
{
  goNewListDialog(GetSelectedDirectory());
}

function goNewListDialog(selectedAB)
{
  window.openDialog("chrome://messenger/content/addressbook/abMailListDialog.xul",
                    "",
                    "chrome,modal,resizable,centerscreen",
                    {selectedAB:selectedAB});
}

function goEditListDialog(abCard, listURI)
{
  window.openDialog("chrome://messenger/content/addressbook/abEditListDialog.xul",
                    "",
                    "chrome,modal,resizable,centerscreen",
                    {abCard:abCard, listURI:listURI});
}

function goNewCardDialog(selectedAB)
{
  window.openDialog("chrome://messenger/content/addressbook/abNewCardDialog.xul",
                    "",
                    "chrome,modal,resizable=no,centerscreen",
                    {selectedAB:selectedAB});
}

function goEditCardDialog(abURI, card)
{
  window.openDialog("chrome://messenger/content/addressbook/abEditCardDialog.xul",
                    "",
                    "chrome,modal,resizable=no,centerscreen",
                    {abURI:abURI, card:card});
}


function setSortByMenuItemCheckState(id, value)
{
    var menuitem = document.getElementById(id);
    if (menuitem) {
      menuitem.setAttribute("checked", value);
    }
}

function InitViewSortByMenu()
{
    var sortColumn = kDefaultSortColumn;
    var sortDirection = kDefaultAscending;

    if (gAbView) {
      sortColumn = gAbView.sortColumn;
      sortDirection = gAbView.sortDirection;
    }

    // this approach is necessary to support generic columns that get overlayed.
    var elements = document.getElementsByAttribute("name","sortas");
    for (var i=0; i<elements.length; i++) {
        var cmd = elements[i].getAttribute("id");
        var columnForCmd = cmd.split("cmd_SortBy")[1];
        setSortByMenuItemCheckState(cmd, (sortColumn == columnForCmd));
    }

    setSortByMenuItemCheckState("sortAscending", (sortDirection == kDefaultAscending));
    setSortByMenuItemCheckState("sortDescending", (sortDirection == kDefaultDescending));
}

function GenerateAddressFromCard(card)
{
  if (!card)
    return "";

  var email;
  if (card.isMailList) 
  {
    var directory = GetDirectoryFromURI(card.mailListURI);
    email = directory.description || card.displayName;
  }
  else 
    email = card.primaryEmail;
  return gHeaderParser.makeFullAddress(card.displayName, email);
}

function GetDirectoryFromURI(uri)
{
  return MailServices.ab.getDirectory(uri);
}

// returns null if abURI is not a mailing list URI
function GetParentDirectoryFromMailingListURI(abURI)
{
  var abURIArr = abURI.split("/");
  /*
   turn turn "moz-abmdbdirectory://abook.mab/MailList6"
   into ["moz-abmdbdirectory:","","abook.mab","MailList6"]
   then, turn ["moz-abmdbdirectory:","","abook.mab","MailList6"]
   into "moz-abmdbdirectory://abook.mab"
  */
  if (abURIArr.length == 4 && abURIArr[0] == "moz-abmdbdirectory:" && abURIArr[3] != "") {
    return abURIArr[0] + "/" + abURIArr[1] + "/" + abURIArr[2];
  }

  return null;
} 

function DirPaneHasFocus()
{
  // Returns true if directory pane has the focus. Returns false, otherwise.
  return top.document.commandDispatcher.focusedElement == gDirTree;
}

function GetSelectedDirectory()
{
  if (abList)
    return abList.value;

  if (gDirTree.currentIndex < 0)
    return null;

  return gDirectoryTreeView.getDirectoryAtIndex(gDirTree.currentIndex).URI;
}

/**
 * Returns an nsIFile of the directory in which contact photos are stored.
 * This will create the directory if it does not yet exist.
 */
function getPhotosDir() {
  var file = Components.classes["@mozilla.org/file/directory_service;1"]
                       .getService(Components.interfaces.nsIProperties)
                       .get("ProfD", Components.interfaces.nsIFile);
  // Get the Photos directory
  file.append("Photos");
  if (!file.exists() || !file.isDirectory())
    file.create(Components.interfaces.nsIFile.DIRECTORY_TYPE, 0777);
  return file;
}

/**
 * Returns a URI specifying the location of a photo based on its name.
 * If the name is blank, or if the photo with that name is not in the Photos
 * directory then the default photo URI is returned.
 *
 * @param aPhotoName The name of the photo from the Photos folder, if any.
 *
 * @return A URI pointing to a photo.
 */
function getPhotoURI(aPhotoName) {
  if (!aPhotoName)
    return defaultPhotoURI;
  var file = getPhotosDir();
  try {
    file.append(aPhotoName);
  }
  catch (e) {
    return defaultPhotoURI;
  }
  if (!file.exists())
    return defaultPhotoURI;
  return Components.classes["@mozilla.org/network/io-service;1"]
                   .getService(Components.interfaces.nsIIOService)
                   .newFileURI(file).spec;
}

/**
 * Saves the given input stream to a file.
 *
 * @param aIStream The input stream to save.
 * @param aFile    The file to which the stream is saved.
 */
function saveStreamToFile(aIStream, aFile) {
  if (!(aIStream instanceof Components.interfaces.nsIInputStream))
    throw "Invalid stream passed to saveStreamToFile";
  if (!(aFile instanceof Components.interfaces.nsIFile))
    throw "Invalid file passed to saveStreamToFile";
  // Write the input stream to the file
  var fstream = Components.classes["@mozilla.org/network/safe-file-output-stream;1"]
                          .createInstance(Components.interfaces.nsIFileOutputStream);
  var buffer  = Components.classes["@mozilla.org/network/buffered-output-stream;1"]
                          .createInstance(Components.interfaces.nsIBufferedOutputStream);
  fstream.init(aFile, 0x04 | 0x08 | 0x20, 0600, 0); // write, create, truncate
  buffer.init(fstream, 8192);

  buffer.writeFrom(aIStream, aIStream.available());

  // Close the output streams
  if (buffer instanceof Components.interfaces.nsISafeOutputStream)
      buffer.finish();
  else
      buffer.close();
  if (fstream instanceof Components.interfaces.nsISafeOutputStream)
      fstream.finish();
  else
      fstream.close();
  // Close the input stream
  aIStream.close();
  return aFile;
}

/**
 * Copies the photo at the given URI in a folder named "Photos" in the current
 * profile folder.
 * The filename is randomly generated and is unique.
 * The URI is used to obtain a channel which is then opened synchronously and
 * this stream is written to the new file to store an offline, local copy of the
 * photo.
 *
 * @param aUri The URI of the photo.
 *
 * @return An nsIFile representation of the photo.
 */
function savePhoto(aUri) {
  if (!aUri)
    return false;

  // Get the photos directory and check that it exists
  var file = getPhotosDir();

  // Create a channel from the URI and open it as an input stream
  var ios = Components.classes["@mozilla.org/network/io-service;1"]
                      .getService(Components.interfaces.nsIIOService);
  var channel = ios.newChannelFromURI(ios.newURI(aUri, null, null));
  var istream = channel.open();

  // Get the photo file
  file = makePhotoFile(file, findPhotoExt(channel));

  return saveStreamToFile(istream, file);
}

/**
 * Finds the file extension of the photo identified by the URI, if possible.
 * This function can be overridden (with a copy of the original) for URIs that
 * do not identify the extension or when the Content-Type response header is
 * either not set or isn't 'image/png', 'image/jpeg', or 'image/gif'.
 * The original function can be called if the URI does not match.
 *
 * @param aUri The URI of the photo.
 * @param aChannel The opened channel for the URI.
 *
 * @return The extension of the file, if any, including the period.
 */
function findPhotoExt(aChannel) {
  var mimeSvc = Components.classes["@mozilla.org/mime;1"]
                          .getService(Components.interfaces.nsIMIMEService);
  var ext = "";
  var uri = aChannel.URI;
  if (uri instanceof Components.interfaces.nsIURL)
    ext = uri.fileExtension;
  try {
    return mimeSvc.getPrimaryExtension(aChannel.contentType, ext);
  } catch (e) {}
  return ext;
}

/**
 * Generates a unique filename to be used for a local copy of a contact's photo.
 *
 * @param aPath      The path to the folder in which the photo will be saved.
 * @param aExtension The file extension of the photo.
 *
 * @return A unique filename in the given path.
 */
function makePhotoFile(aDir, aExtension) {
  var filename, newFile;
  // Find a random filename for the photo that doesn't exist yet
  do {
    filename = new String(Math.random()).replace("0.", "") + "." + aExtension;
    newFile = aDir.clone();
    newFile.append(filename);
  } while (newFile.exists());
  return newFile;
}
