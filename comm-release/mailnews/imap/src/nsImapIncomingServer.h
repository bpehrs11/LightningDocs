/* -*- Mode: C++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
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
 *   Seth Spitzer <sspitzer@netscape.com>
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

#ifndef __nsImapIncomingServer_h
#define __nsImapIncomingServer_h

#include "msgCore.h"
#include "nsIImapIncomingServer.h"
#include "nsMsgIncomingServer.h"
#include "nsIImapServerSink.h"
#include "nsIStringBundle.h"
#include "nsISubscribableServer.h"
#include "nsIUrlListener.h"
#include "nsIMsgImapMailFolder.h"
#include "nsCOMArray.h"
#include "mozilla/Mutex.h"

class nsIRDFService;

/* get some implementation from nsMsgIncomingServer */
class nsImapIncomingServer : public nsMsgIncomingServer,
                             public nsIImapIncomingServer,
                             public nsIImapServerSink,
                             public nsISubscribableServer,
                             public nsIUrlListener
{
public:
    NS_DECL_ISUPPORTS_INHERITED

    nsImapIncomingServer();
    virtual ~nsImapIncomingServer();

    // overriding nsMsgIncomingServer methods
  NS_IMETHOD SetKey(const nsACString& aKey);  // override nsMsgIncomingServer's implementation...
  NS_IMETHOD GetLocalStoreType(nsACString& type);

  NS_DECL_NSIIMAPINCOMINGSERVER
  NS_DECL_NSIIMAPSERVERSINK
  NS_DECL_NSISUBSCRIBABLESERVER
  NS_DECL_NSIURLLISTENER

  NS_IMETHOD PerformBiff(nsIMsgWindow *aMsgWindow);
  NS_IMETHOD PerformExpand(nsIMsgWindow *aMsgWindow);
  NS_IMETHOD CloseCachedConnections();
  NS_IMETHOD GetConstructedPrettyName(nsAString& retval);
  NS_IMETHOD GetCanBeDefaultServer(bool *canBeDefaultServer);
  NS_IMETHOD GetCanCompactFoldersOnServer(bool *canCompactFoldersOnServer);
  NS_IMETHOD GetCanUndoDeleteOnServer(bool *canUndoDeleteOnServer);
  NS_IMETHOD GetCanSearchMessages(bool *canSearchMessages);
  NS_IMETHOD GetCanEmptyTrashOnExit(bool *canEmptyTrashOnExit);
  NS_IMETHOD GetOfflineSupportLevel(PRInt32 *aSupportLevel);
  NS_IMETHOD GeneratePrettyNameForMigration(nsAString& aPrettyName);
  NS_IMETHOD GetSupportsDiskSpace(bool *aSupportsDiskSpace);
  NS_IMETHOD GetCanCreateFoldersOnServer(bool *aCanCreateFoldersOnServer);
  NS_IMETHOD GetCanFileMessagesOnServer(bool *aCanFileMessagesOnServer);
  NS_IMETHOD GetFilterScope(nsMsgSearchScopeValue *filterScope);
  NS_IMETHOD GetSearchScope(nsMsgSearchScopeValue *searchScope);
  NS_IMETHOD GetServerRequiresPasswordForBiff(bool *aServerRequiresPasswordForBiff);
  NS_IMETHOD OnUserOrHostNameChanged(const nsACString& oldName, const nsACString& newName);
  NS_IMETHOD GetNumIdleConnections(PRInt32 *aNumIdleConnections);
  NS_IMETHOD ForgetSessionPassword();
  NS_IMETHOD GetMsgFolderFromURI(nsIMsgFolder *aFolderResource, const nsACString& aURI, nsIMsgFolder **aFolder);
  NS_IMETHOD SetSocketType(PRInt32 aSocketType);
  NS_IMETHOD VerifyLogon(nsIUrlListener *aUrlListener, nsIMsgWindow *aMsgWindow,
                         nsIURI **aURL);

protected:
  nsresult GetFolder(const nsACString& name, nsIMsgFolder** pFolder);
  virtual nsresult CreateRootFolderFromUri(const nsCString &serverUri,
                                           nsIMsgFolder **rootFolder);
  nsresult ResetFoldersToUnverified(nsIMsgFolder *parentFolder);
  void GetUnverifiedSubFolders(nsIMsgFolder *parentFolder,
                               nsCOMArray<nsIMsgImapMailFolder> &aFoldersArray);
  void GetUnverifiedFolders(nsCOMArray<nsIMsgImapMailFolder> &aFolderArray);
  nsresult DeleteNonVerifiedFolders(nsIMsgFolder *parentFolder);
  bool NoDescendentsAreVerified(nsIMsgFolder *parentFolder);
  bool AllDescendentsAreNoSelect(nsIMsgFolder *parentFolder);

  nsresult GetStringBundle();
  nsString GetImapStringByName(const nsString &aName);
  static nsresult AlertUser(const nsAString& aString, nsIMsgMailNewsUrl *aUrl);

private:
  nsresult SubscribeToFolder(const PRUnichar *aName, bool subscribe);
  nsresult GetImapConnection(nsIImapUrl* aImapUrl,
                             nsIImapProtocol** aImapConnection);
  nsresult CreateProtocolInstance(nsIImapProtocol ** aImapConnection);
  nsresult CreateHostSpecificPrefName(const char *prefPrefix, nsCAutoString &prefName);

  nsresult DoomUrlIfChannelHasError(nsIImapUrl *aImapUrl, bool *urlDoomed);
  bool ConnectionTimeOut(nsIImapProtocol* aImapConnection);
  nsresult GetFormattedStringFromID(const nsAString& aValue, PRInt32 aID, nsAString& aResult);
  nsresult GetPrefForServerAttribute(const char *prefSuffix, bool *prefValue);
  bool CheckSpecialFolder(nsIRDFService *rdf, nsCString &folderUri,
                            PRUint32 folderFlag, nsCString &existingUri);

  nsCOMArray<nsIImapProtocol> m_connectionCache;
  nsCOMArray<nsIImapUrl> m_urlQueue;
  nsCOMPtr<nsIStringBundle>	m_stringBundle;
  nsCOMArray<nsIMsgFolder> m_subscribeFolders; // used to keep folder resources around while subscribe UI is up.
  nsCOMArray<nsIMsgImapMailFolder> m_foldersToStat; // folders to check for new mail with Status
  nsVoidArray       m_urlConsumers;
  PRUint32          m_capability;
  nsCString         m_manageMailAccountUrl;
  bool              m_userAuthenticated;
  bool              mDoingSubscribeDialog;
  bool              mDoingLsub;
  bool              m_shuttingDown;

  mozilla::Mutex mLock;
  // subscribe dialog stuff
  nsresult AddFolderToSubscribeDialog(const char *parentUri, const char *uri,const char *folderName);
  nsCOMPtr <nsISubscribableServer> mInner;
  nsresult EnsureInner();
  nsresult ClearInner();
};

#endif
