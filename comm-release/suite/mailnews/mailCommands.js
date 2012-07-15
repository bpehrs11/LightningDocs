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

function GetNewMessages(selectedFolders, server)
{
  if (!selectedFolders.length)
    return;

  var msgFolder = selectedFolders[0];

  // Whenever we do get new messages, clear the old new messages.
  if (msgFolder)
  {
    var nsIMsgFolder = Components.interfaces.nsIMsgFolder;
    msgFolder.biffState = nsIMsgFolder.nsMsgBiffState_NoMail;
    msgFolder.clearNewMessages();
  }
  server.getNewMessages(msgFolder, msgWindow, null);
}

function getBestIdentity(identities, optionalHint)
{
  var identity = null;

  var identitiesCount = identities.Count();

  try
  {
    // if we have more than one identity and a hint to help us pick one
    if (identitiesCount > 1 && optionalHint) {
      // normalize case on the optional hint to improve our chances of finding a match
      optionalHint = optionalHint.toLowerCase();

      var id;
      // iterate over all of the identities
      var tempID;

      var lengthOfLongestMatchingEmail = 0;
      for (id = 0; id < identitiesCount; ++id) {
        tempID = identities.GetElementAt(id).QueryInterface(Components.interfaces.nsIMsgIdentity);
        if (optionalHint.indexOf(tempID.email.toLowerCase()) >= 0) {
          // Be careful, the user can have several adresses with the same
          // postfix e.g. aaa.bbb@ccc.ddd and bbb@ccc.ddd. Make sure we get the
          // longest match.
          if (tempID.email.length > lengthOfLongestMatchingEmail) {
            identity = tempID;
            lengthOfLongestMatchingEmail = tempID.email.length;
          }
        }
      }

      // if we could not find an exact email address match within the hint fields then maybe the message
      // was to a mailing list. In this scenario, we won't have a match based on email address.
      // Before we just give up, try and search for just a shared domain between the hint and
      // the email addresses for our identities. Hey, it is better than nothing and in the case
      // of multiple matches here, we'll end up picking the first one anyway which is what we would have done
      // if we didn't do this second search. This helps the case for corporate users where mailing lists will have the same domain
      // as one of your multiple identities.

      if (!identity) {
        for (id = 0; id < identitiesCount; ++id) {
          tempID = identities.GetElementAt(id).QueryInterface(Components.interfaces.nsIMsgIdentity);
          // extract out the partial domain
          var start = tempID.email.lastIndexOf("@"); // be sure to include the @ sign in our search to reduce the risk of false positives
          if (optionalHint.search(tempID.email.slice(start).toLowerCase()) >= 0) {
            identity = tempID;
            break;
          }
        }
      }
    }
  }
  catch (ex) {dump (ex + "\n");}

  // Still no matches ?
  // Give up and pick the first one (if it exists), like we used to.
  if (!identity && identitiesCount > 0)
    identity = identities.GetElementAt(0).QueryInterface(Components.interfaces.nsIMsgIdentity);

  return identity;
}

function getIdentityForServer(server, optionalHint)
{
    var identity = null;

    if (server) {
        // Get the identities associated with this server.
        var identities = accountManager.GetIdentitiesForServer(server);
        // dump("identities = " + identities + "\n");
        // Try and find the best one.
        identity = getBestIdentity(identities, optionalHint);
    }

    return identity;
}

function GetIdentityForHeader(aMsgHdr, aType)
{
  // If we treat reply from sent specially, do we check for that folder flag here ?
  var isTemplate = aType == Components.interfaces.nsIMsgCompType.Template;
  var hintForIdentity = isTemplate ? aMsgHdr.author
                                   : aMsgHdr.recipients + aMsgHdr.ccList;
  var identity = null;
  var server = null;

  var folder = aMsgHdr.folder;
  if (folder)
  {
    server = folder.server;
    identity = folder.customIdentity;
  }

  if (!identity)
  {
    var accountKey = aMsgHdr.accountKey;
    if (accountKey.length > 0)
    {
      let account = accountManager.getAccount(accountKey);
      if (account)
        server = account.incomingServer;
    }

    if (server)
      identity = getIdentityForServer(server, hintForIdentity);

    if (!identity)
      identity = getBestIdentity(accountManager.allIdentities, hintForIdentity);
  }
  return identity;
}

function GetNextNMessages(folder)
{
  if (folder) {
    var newsFolder = folder.QueryInterface(Components.interfaces.nsIMsgNewsFolder);
    if (newsFolder) {
      newsFolder.getNextNMessages(msgWindow);
    }
  }
}

// type is a nsIMsgCompType and format is a nsIMsgCompFormat
function ComposeMessage(type, format, folder, messageArray)
{
  var msgComposeType = Components.interfaces.nsIMsgCompType;
  var identity = null;
  var newsgroup = null;
  var hdr;

  // dump("ComposeMessage folder=" + folder + "\n");
  try
  {
    if (folder)
    {
      // Get the incoming server associated with this uri.
      var server = folder.server;

      // If they hit new or reply and they are reading a newsgroup,
      // turn this into a new post or a reply to group.
      if (!folder.isServer && server.type == "nntp" && type == msgComposeType.New)
      {
        type = msgComposeType.NewsPost;
        newsgroup = folder.folderURL;
      }

      identity = getIdentityForServer(server);
      // dump("identity = " + identity + "\n");
    }
  }
  catch (ex)
  {
    dump("failed to get an identity to pre-select: " + ex + "\n");
  }

  // dump("\nComposeMessage from XUL: " + identity + "\n");

  if (!msgComposeService)
  {
    dump("### msgComposeService is invalid\n");
    return;
  }

  switch (type)
  {
    case msgComposeType.New: //new message
      // dump("OpenComposeWindow with " + identity + "\n");

      // If the addressbook sidebar panel is open and has focus, get
      // the selected addresses from it.
      if (document.commandDispatcher.focusedWindow &&
          document.commandDispatcher.focusedWindow
                  .document.documentElement.hasAttribute("selectedaddresses"))
        NewMessageToSelectedAddresses(type, format, identity);
      else
        msgComposeService.OpenComposeWindow(null, null, null, type,
                                            format, identity, msgWindow);
      return;
    case msgComposeType.NewsPost:
      // dump("OpenComposeWindow with " + identity + " and " + newsgroup + "\n");
      msgComposeService.OpenComposeWindow(null, null, newsgroup, type,
                                          format, identity, msgWindow);
      return;
    case msgComposeType.ForwardAsAttachment:
      if (messageArray && messageArray.length)
      {
        // If we have more than one ForwardAsAttachment then pass null instead
        // of the header to tell the compose service to work out the attachment
        // subjects from the URIs.
        hdr = messageArray.length > 1 ? null : messenger.msgHdrFromURI(messageArray[0]);
        msgComposeService.OpenComposeWindow(null, hdr, messageArray.join(','),
                                            type, format, identity, msgWindow);
        return;
      }
    default:
      if (!messageArray)
        return;

      // Limit the number of new compose windows to 8. Why 8 ?
      // I like that number :-)
      if (messageArray.length > 8)
        messageArray.length = 8;

      for (var i = 0; i < messageArray.length; ++i)
      {
        var messageUri = messageArray[i];
        hdr = messenger.msgHdrFromURI(messageUri);
        identity = GetIdentityForHeader(hdr, type);
        if (hdr.folder && hdr.folder.server.type == "rss")
          openComposeWindowForRSSArticle(null, hdr, messageUri, type,
                                         format, identity, msgWindow);
        else
          msgComposeService.OpenComposeWindow(null, hdr, messageUri, type,
                                              format, identity, msgWindow);
      }
  }
}

function NewMessageToSelectedAddresses(type, format, identity) {
  var abSidebarPanel = document.commandDispatcher.focusedWindow;
  var abResultsTree = abSidebarPanel.document.getElementById("abResultsTree");
  var abResultsBoxObject = abResultsTree.treeBoxObject;
  var abView = abResultsBoxObject.view;
  abView = abView.QueryInterface(Components.interfaces.nsIAbView);
  var addresses = abView.selectedAddresses;
  var params = Components.classes["@mozilla.org/messengercompose/composeparams;1"].createInstance(Components.interfaces.nsIMsgComposeParams);
  if (params) {
    params.type = type;
    params.format = format;
    params.identity = identity;
    var composeFields = Components.classes["@mozilla.org/messengercompose/composefields;1"].createInstance(Components.interfaces.nsIMsgCompFields);
    if (composeFields) {
      var addressList = "";
      for (var i = 0; i < addresses.Count(); i++) {
        addressList = addressList + (i > 0 ? ",":"") + addresses.QueryElementAt(i,Components.interfaces.nsISupportsString).data;
      }
      composeFields.to = addressList;
      params.composeFields = composeFields;
      msgComposeService.OpenComposeWindowWithParams(null, params);
    }
  }
}

function NewFolder(name, folder)
{
  if (!folder || !name)
    return;

  folder.createSubfolder(name, msgWindow);
}

function UnSubscribe(folder)
{
  // Unsubscribe the current folder from the newsserver, this assumes any confirmation has already
  // been made by the user  SPL

  var server = folder.server;
  var subscribableServer = server.QueryInterface(Components.interfaces.nsISubscribableServer);
  subscribableServer.unsubscribe(folder.name);
  subscribableServer.commitSubscribeChanges();
}

function Subscribe(preselectedMsgFolder)
{
  window.openDialog("chrome://messenger/content/subscribe.xul",
                    "subscribe", "chrome,modal,titlebar,resizable=yes",
                    {folder:preselectedMsgFolder,
                      okCallback:SubscribeOKCallback});
}

function SubscribeOKCallback(changeTable)
{
  for (var serverURI in changeTable) {
    var folder = GetMsgFolderFromUri(serverURI, true);
    var server = folder.server;
    var subscribableServer =
          server.QueryInterface(Components.interfaces.nsISubscribableServer);

    for (var name in changeTable[serverURI]) {
      if (changeTable[serverURI][name] == true) {
        try {
          subscribableServer.subscribe(name);
        }
        catch (ex) {
          dump("failed to subscribe to " + name + ": " + ex + "\n");
        }
      }
      else if (changeTable[serverURI][name] == false) {
        try {
          subscribableServer.unsubscribe(name);
        }
        catch (ex) {
          dump("failed to unsubscribe to " + name + ": " + ex + "\n");
        }
      }
      else {
        // no change
      }
    }

    try {
      subscribableServer.commitSubscribeChanges();
    }
    catch (ex) {
      dump("failed to commit the changes: " + ex + "\n");
    }
  }
}

function SaveAsFile(aUris)
{
  if (/type=application\/x-message-display/.test(aUris[0]))
  {
    saveURL(aUris[0], null, "", true, false, null);
    return;
  }

  var num = aUris.length;
  var fileNames = [];
  for (let i = 0; i < num; i++)
  {
    let subject = messenger.messageServiceFromURI(aUris[i])
                           .messageURIToMsgHdr(aUris[i])
                           .mime2DecodedSubject;
    fileNames[i] = suggestUniqueFileName(subject.substr(0, 120), ".eml",
                                         fileNames);
  }
  if (num == 1)
    messenger.saveAs(aUris[0], true, null, fileNames[0]);
  else
    messenger.saveMessages(num, fileNames, aUris);
}

function saveAsUrlListener(aUri, aIdentity)
{
  this.uri = aUri;
  this.identity = aIdentity;
}

saveAsUrlListener.prototype = {
  OnStartRunningUrl: function(aUrl)
  {
  },
  OnStopRunningUrl: function(aUrl, aExitCode)
  {
    messenger.saveAs(this.uri, false, this.identity, null);
  }
};

function SaveAsTemplate(uri)
{
  if (uri)
  {
    var hdr = messenger.msgHdrFromURI(uri);
    var identity = GetIdentityForHeader(hdr, Components.interfaces.nsIMsgCompType.Template);
    var templates = MailUtils.getFolderForURI(identity.stationeryFolder, false);
    if (!templates.parent)
    {
      templates.setFlag(Components.interfaces.nsMsgFolderFlags.Templates);
      let isImap = templates.server.type == "imap";
      templates.createStorageIfMissing(new saveAsUrlListener(uri, identity));
      if (isImap)
        return;
    }
    messenger.saveAs(uri, false, identity, null);
  }
}

function MarkSelectedMessagesRead(markRead)
{
  ClearPendingReadTimer();
  gDBView.doCommand(markRead ? nsMsgViewCommandType.markMessagesRead : nsMsgViewCommandType.markMessagesUnread);
}

function MarkSelectedMessagesFlagged(markFlagged)
{
  gDBView.doCommand(markFlagged ? nsMsgViewCommandType.flagMessages : nsMsgViewCommandType.unflagMessages);
}

function ViewPageSource(messages)
{
  var numMessages = messages.length;

  if (numMessages == 0)
  {
    dump("MsgViewPageSource(): No messages selected.\n");
    return false;
  }

    try {
        // First, get the mail session
        const nsIMsgMailSession = Components.interfaces.nsIMsgMailSession;
        var mailSession = Components.classes["@mozilla.org/messenger/services/session;1"]
                                    .getService(nsIMsgMailSession);

        var mailCharacterSet = "charset=" + msgWindow.mailCharacterSet;

        for (var i = 0; i < numMessages; i++)
        {
            // Now, we need to get a URL from a URI
            var url = mailSession.ConvertMsgURIToMsgURL(messages[i], msgWindow);

            // Strip out the message-display parameter to ensure that attached
            // emails display the message source, not the processed HTML.
            url = url.replace(/(\?|&)type=application\/x-message-display(&|$)/, "$1")
                     .replace(/\?$/, "");
            window.openDialog( "chrome://global/content/viewSource.xul",
                               "_blank", "all,dialog=no", url,
                               mailCharacterSet);
        }
        return true;
    } catch (e) {
        // Couldn't get mail session
        return false;
    }
}

function doHelpButton()
{
    openHelp("mail-offline-items");
}

function confirmToProceed(commandName)
{
  const kDontAskAgainPref = "mailnews."+commandName+".dontAskAgain";
  // default to ask user if the pref is not set
  var dontAskAgain = false;
  try {
    var pref = Components.classes["@mozilla.org/preferences-service;1"]
                        .getService(Components.interfaces.nsIPrefBranch);
    dontAskAgain = pref.getBoolPref(kDontAskAgainPref);
  } catch (ex) {}

  if (!dontAskAgain)
  {
    var checkbox = {value:false};
    var choice = Services.prompt.confirmEx(
                   window,
                   gMessengerBundle.getString(commandName+"Title"),
                   gMessengerBundle.getString(commandName+"Message"),
                   Services.prompt.STD_YES_NO_BUTTONS,
                   null, null, null,
                   gMessengerBundle.getString(commandName+"DontAsk"),
                   checkbox);
    try {
      if (checkbox.value)
        pref.setBoolPref(kDontAskAgainPref, true);
    } catch (ex) {}

    if (choice != 0)
      return false;
  }
  return true;
}

function deleteAllInFolder(commandName)
{
  var folder = GetMsgFolderFromUri(GetSelectedFolderURI(), true);
  if (!folder)
    return;

  if (!confirmToProceed(commandName))
    return;

  // Delete sub-folders.
  var iter = folder.subFolders;
  while (iter.hasMoreElements())
    folder.propagateDelete(iter.getNext(), true, msgWindow);
  
  var children = Components.classes["@mozilla.org/array;1"]
                  .createInstance(Components.interfaces.nsIMutableArray);
                  
  // Delete messages.
  iter = folder.messages;
  while (iter.hasMoreElements()) {
    children.appendElement(iter.getNext(), false);
  }
  folder.deleteMessages(children, msgWindow, true, false, null, false); 
  children.clear();
}
