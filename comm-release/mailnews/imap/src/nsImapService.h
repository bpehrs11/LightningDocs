/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
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

#ifndef nsImapService_h___
#define nsImapService_h___

#include "nsIImapService.h"
#include "nsIMsgMessageService.h"
#include "nsCOMPtr.h"
#include "nsIFile.h"
#include "nsIProtocolHandler.h"
#include "nsIMsgProtocolInfo.h"
#include "nsIContentHandler.h"
#include "nsICacheSession.h"

class nsIImapHostSessionList; 
class nsCString;
class nsIImapUrl;
class nsIMsgFolder;
class nsIMsgStatusFeedback;
class nsIMsgIncomingServer;

class nsImapService : public nsIImapService,
                      public nsIMsgMessageService,
                      public nsIMsgMessageFetchPartService,
                      public nsIProtocolHandler,
                      public nsIMsgProtocolInfo,
                      public nsIContentHandler
{
public:
  nsImapService();
  virtual ~nsImapService();

  NS_DECL_ISUPPORTS
  NS_DECL_NSIMSGPROTOCOLINFO
  NS_DECL_NSIIMAPSERVICE
  NS_DECL_NSIMSGMESSAGESERVICE
  NS_DECL_NSIPROTOCOLHANDLER
  NS_DECL_NSIMSGMESSAGEFETCHPARTSERVICE
  NS_DECL_NSICONTENTHANDLER

protected:
  char GetHierarchyDelimiter(nsIMsgFolder *aMsgFolder);

  nsresult GetFolderName(nsIMsgFolder *aImapFolder, nsACString &aFolderName);

  // This is called by both FetchMessage and StreamMessage
  nsresult GetMessageFromUrl(nsIImapUrl *aImapUrl,
                             nsImapAction aImapAction,
                             nsIMsgFolder *aImapMailFolder, 
                             nsIImapMessageSink *aImapMessage,
                             nsIMsgWindow *aMsgWindow,
                             nsISupports *aDisplayConsumer, 
                             bool aConvertDataToText,
                             nsIURI **aURL);

  nsresult CreateStartOfImapUrl(const nsACString &aImapURI,  // a RDF URI for the current message/folder, can be empty
                                nsIImapUrl  **imapUrl,
                                nsIMsgFolder *aImapFolder,
                                nsIUrlListener *aUrlListener,
                                nsACString &urlSpec,
                                char &hierarchyDelimiter);

  nsresult GetImapConnectionAndLoadUrl(nsIImapUrl *aImapUrl,
                                       nsISupports *aConsumer,
                                       nsIURI **aURL);

  nsresult SetImapUrlSink(nsIMsgFolder *aMsgFolder, nsIImapUrl *aImapUrl);

  nsresult FetchMimePart(nsIImapUrl *aImapUrl,
                         nsImapAction aImapAction,
                         nsIMsgFolder *aImapMailFolder, 
                         nsIImapMessageSink *aImapMessage,
                         nsIURI **aURL,
                         nsISupports *aDisplayConsumer, 
                         const nsACString &messageIdentifierList,
                         const nsACString &mimePart);

  nsresult FolderCommand(nsIMsgFolder *imapMailFolder,
                         nsIUrlListener *urlListener,
                         const char *aCommand,
                         nsImapAction imapAction,
                         nsIMsgWindow *msgWindow,
                         nsIURI **url);

  nsresult ChangeFolderSubscription(nsIMsgFolder *folder,
                                    const nsAString &folderName,
                                    const char *aCommand,
                                    nsIUrlListener *urlListener,
                                    nsIURI **url);

  nsresult DiddleFlags(nsIMsgFolder *aImapMailFolder,
                       nsIUrlListener *aUrlListener,
                       nsIURI **aURL,
                       const nsACString &messageIdentifierList,
                       const char *howToDiddle,
                       imapMessageFlagsType flags,
                       bool messageIdsAreUID);

  nsresult OfflineAppendFromFile(nsIFile *aFile,
                                 nsIURI *aUrl,
                                 nsIMsgFolder *aDstFolder,
                                 const nsACString &messageId,  // to be replaced
                                 bool inSelectedState, // needs to be in
                                 nsIUrlListener *aListener,
                                 nsIURI **aURL,
                                 nsISupports *aCopyState);

  nsresult GetServerFromUrl(nsIImapUrl *aImapUrl, nsIMsgIncomingServer **aServer);

  // just a little helper method...maybe it should be a macro? which helps break down a imap message uri
  // into the folder and message key equivalents
  nsresult DecomposeImapURI(const nsACString &aMessageURI, nsIMsgFolder **aFolder, nsACString &msgKey);
  nsresult DecomposeImapURI(const nsACString &aMessageURI, nsIMsgFolder **aFolder, nsMsgKey *msgKey);


  nsCOMPtr<nsICacheSession> mCacheSession;  // handle to the cache session for imap.....
  bool mPrintingOperation;                // Flag for printing operations
};

#endif /* nsImapService_h___ */
