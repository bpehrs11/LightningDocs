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
 * ***** END LICENSE BLOCK *****
 * This Original Code has been modified by IBM Corporation. Modifications made by IBM 
 * described herein are Copyright (c) International Business Machines Corporation, 2000.
 * Modifications to Mozilla code or documentation identified per MPL Section 3.3
 *
 * Date             Modified by     Description of modification
 * 04/20/2000       IBM Corp.      OS/2 VisualAge build.
 */

#include "nscore.h"
#include "nsIMsgAccountManager.h"
#include "nsCOMPtr.h"
#include "nsISmtpServer.h"
#include "nsIPrefBranch.h"
#include "nsIMsgFolderCache.h"
#include "nsIMsgFolder.h"
#include "nsIObserver.h"
#include "nsWeakReference.h"
#include "nsIUrlListener.h"
#include "nsCOMArray.h"
#include "nsInterfaceHashtable.h"
#include "nsIMsgDatabase.h"
#include "nsIDBChangeListener.h"
#include "nsAutoPtr.h"
#include "nsTObserverArray.h"

class nsIRDFService;

class VirtualFolderChangeListener : public nsIDBChangeListener
{
public:
  VirtualFolderChangeListener();
  ~VirtualFolderChangeListener() {}

  NS_DECL_ISUPPORTS
  NS_DECL_NSIDBCHANGELISTENER

  nsresult Init();
  /**
   * Posts an event to update the summary totals and commit the db.
   * We post the event to avoid committing each time we're called
   * in a synchronous loop.
   */
  nsresult PostUpdateEvent(nsIMsgFolder *folder, nsIMsgDatabase *db);
  /// Handles event posted to event queue to batch notifications.
  void ProcessUpdateEvent(nsIMsgFolder *folder, nsIMsgDatabase *db);

  void DecrementNewMsgCount();

  nsCOMPtr <nsIMsgFolder> m_virtualFolder; // folder we're listening to db changes on behalf of.
  nsCOMPtr <nsIMsgFolder> m_folderWatching; // folder whose db we're listening to.
  nsCOMPtr <nsISupportsArray> m_searchTerms;
  nsCOMPtr <nsIMsgSearchSession> m_searchSession;
  bool m_searchOnMsgStatus;
  bool m_batchingEvents;
};


class nsMsgAccountManager: public nsIMsgAccountManager,
    public nsIObserver,
    public nsSupportsWeakReference,
    public nsIUrlListener,
    public nsIFolderListener
{
public:

  nsMsgAccountManager();
  virtual ~nsMsgAccountManager();
  
  NS_DECL_ISUPPORTS
 
  /* nsIMsgAccountManager methods */
  
  NS_DECL_NSIMSGACCOUNTMANAGER
  NS_DECL_NSIOBSERVER  
  NS_DECL_NSIURLLISTENER
  NS_DECL_NSIFOLDERLISTENER

  nsresult Init();
  nsresult Shutdown();
  void LogoutOfServer(nsIMsgIncomingServer *aServer);

private:

  bool m_accountsLoaded;
  nsCOMPtr <nsIMsgFolderCache> m_msgFolderCache;
  nsCOMPtr<nsIAtom> kDefaultServerAtom;
  nsCOMPtr<nsIAtom> mFolderFlagAtom;
  nsCOMPtr<nsISupportsArray> m_accounts;
  nsInterfaceHashtable<nsCStringHashKey, nsIMsgIdentity> m_identities;
  nsInterfaceHashtable<nsCStringHashKey, nsIMsgIncomingServer> m_incomingServers;
  nsCOMPtr<nsIMsgAccount> m_defaultAccount;
  nsCOMArray<nsIIncomingServerListener> m_incomingServerListeners;
  nsTObserverArray<nsRefPtr<VirtualFolderChangeListener> > m_virtualFolderListeners;
  nsCOMPtr<nsIMsgFolder> m_folderDoingEmptyTrash;
  nsCOMPtr<nsIMsgFolder> m_folderDoingCleanupInbox;
  bool m_emptyTrashInProgress;
  bool m_cleanupInboxInProgress;

  nsCString mAccountKeyList;

  // These are static because the account manager may go away during
  // shutdown, and get recreated.
  static bool m_haveShutdown;
  static bool m_shutdownInProgress;

  bool m_userAuthenticated;
  bool m_loadingVirtualFolders;
  bool m_virtualFoldersLoaded;

  /* we call FindServer() a lot.  so cache the last server found */
  nsCOMPtr <nsIMsgIncomingServer> m_lastFindServerResult;
  nsCString m_lastFindServerHostName;
  nsCString m_lastFindServerUserName;
  PRInt32 m_lastFindServerPort;
  nsCString m_lastFindServerType;

  void SetLastServerFound(nsIMsgIncomingServer *server, const nsACString& hostname,
                          const nsACString& username, const PRInt32 port, const nsACString& type);

  // Cache the results of the last call to FolderUriFromDirInProfile
  nsCOMPtr<nsIFile> m_lastPathLookedUp;
  nsCString m_lastFolderURIForPath;

  /* internal creation routines - updates m_identities and m_incomingServers */
  nsresult createKeyedAccount(const nsCString& key,
                              nsIMsgAccount **_retval);
  nsresult createKeyedServer(const nsACString& key,
                             const nsACString& username,
                             const nsACString& password,
                             const nsACString& type,
                             nsIMsgIncomingServer **_retval);

  nsresult createKeyedIdentity(const nsACString& key,
                               nsIMsgIdentity **_retval);

  nsresult GetLocalFoldersPrettyName(nsString &localFoldersName);

  // sets the pref for the default server
  nsresult setDefaultAccountPref(nsIMsgAccount *aDefaultAccount);

  // Write out the accounts pref from the m_accounts list of accounts.
  nsresult OutputAccountsPref();

  // fires notifications to the appropriate root folders
  nsresult notifyDefaultServerChange(nsIMsgAccount *aOldAccount,
                                     nsIMsgAccount *aNewAccount);
    
  static PLDHashOperator hashUnloadServer(nsCStringHashKey::KeyType aKey, nsCOMPtr<nsIMsgIncomingServer>& aServer, void* aClosure);

  //
  // account enumerators
  // ("element" is always an account)
  //

  // find the identities that correspond to the given server
  static bool findIdentitiesForServer(nsISupports *element, void *aData);

  // find the servers that correspond to the given identity
  static bool findServersForIdentity (nsISupports *element, void *aData);

  static bool findServerIndexByServer(nsISupports *element, void *aData);
  // find the account with the given key
  static bool findAccountByKey (nsISupports *element, void *aData);

  static bool findAccountByServerKey (nsISupports *element, void *aData);

  // load up the identities into the given nsISupportsArray
  static bool getIdentitiesToArray(nsISupports *element, void *aData);

  // add identities if they don't alreadby exist in the given nsISupportsArray
  static bool addIdentityIfUnique(nsISupports *element, void *aData);

  //
  // server enumerators
  // ("element" is always a server)
  //

  // find the server given by {username, hostname, port, type}
  static PLDHashOperator findServerUrl(nsCStringHashKey::KeyType aKey,
                                       nsCOMPtr<nsIMsgIncomingServer>& aServer,
                                       void *data);

  // save the server's saved searches to virtualFolders.dat
  static PLDHashOperator saveVirtualFolders(nsCStringHashKey::KeyType aKey,
                                       nsCOMPtr<nsIMsgIncomingServer>& aServer,
                                       void *outputStream);

  nsresult findServerInternal(const nsACString& username,
                              const nsACString& hostname,
                              const nsACString& type,
                              PRInt32 port,
                              bool aRealFlag,
                              nsIMsgIncomingServer** aResult);

  // handle virtual folders
  static nsresult GetVirtualFoldersFile(nsCOMPtr<nsILocalFile>& file);
  static nsresult WriteLineToOutputStream(const char *prefix, const char * line, nsIOutputStream *outputStream);
  void     ParseAndVerifyVirtualFolderScope(nsCString &buffer,
                                            nsIRDFService *rdf);
  nsresult AddVFListenersForVF(nsIMsgFolder *virtualFolder,
                               const nsCString& srchFolderUris,
                               nsIRDFService *rdf,
                               nsIMsgDBService *msgDBService);

  nsresult RemoveVFListenerForVF(nsIMsgFolder *virtualFolder,
                                 nsIMsgFolder *folder);

  static void getUniqueAccountKey(const char * prefix,
                                  nsISupportsArray *accounts,
                                  nsCString& aResult);


  nsresult RemoveFolderFromSmartFolder(nsIMsgFolder *aFolder,
                                       PRUint32 flagsChanged);

  nsresult SetSendLaterUriPref(nsIMsgIncomingServer *server);

  nsCOMPtr<nsIPrefBranch> m_prefs;

  //
  // root folder listener stuff
  //

  // this array is for folder listeners that are supposed to be listening
  // on the root folders.
  // When a new server is created, all of the the folder listeners
  //    should be added to the new server
  // When a new listener is added, it should be added to all root folders.
  // similar for when servers are deleted or listeners removed
  nsCOMPtr<nsISupportsArray> mFolderListeners;

  // folder listener enumerators
  static bool addListenerToFolder(nsISupports *element, void *data);
  static bool removeListenerFromFolder(nsISupports *element, void *data);
};
