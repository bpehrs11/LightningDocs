/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
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

#ifndef __nsNntpIncomingServer_h
#define __nsNntpIncomingServer_h

#include "nsINntpIncomingServer.h"
#include "nsIUrlListener.h"
#include "nscore.h"

#include "nsMsgIncomingServer.h"

#include "prmem.h"
#include "plstr.h"
#include "prprf.h"

#include "nsIMsgWindow.h"
#include "nsISubscribableServer.h"
#include "nsITimer.h"
#include "nsILocalFile.h"
#include "nsITreeView.h"
#include "nsITreeSelection.h"
#include "nsIAtom.h"
#include "nsCOMArray.h"

#include "nsNntpMockChannel.h"
#include "nsAutoPtr.h"

class nsINntpUrl;
class nsIMsgMailNewsUrl;

/* get some implementation from nsMsgIncomingServer */
class nsNntpIncomingServer : public nsMsgIncomingServer,
                             public nsINntpIncomingServer,
                             public nsIUrlListener,
                             public nsISubscribableServer,
                             public nsITreeView
                             
{
public:
    NS_DECL_ISUPPORTS_INHERITED
    NS_DECL_NSINNTPINCOMINGSERVER
    NS_DECL_NSIURLLISTENER
    NS_DECL_NSISUBSCRIBABLESERVER
    NS_DECL_NSITREEVIEW

    nsNntpIncomingServer();
    virtual ~nsNntpIncomingServer();

    NS_IMETHOD GetLocalStoreType(nsACString& type);
    NS_IMETHOD CloseCachedConnections();
    NS_IMETHOD PerformBiff(nsIMsgWindow *aMsgWindow);
    NS_IMETHOD PerformExpand(nsIMsgWindow *aMsgWindow);
    NS_IMETHOD OnUserOrHostNameChanged(const nsACString& oldName, const nsACString& newName);

    // for nsMsgLineBuffer
    virtual PRInt32 HandleLine(const char *line, PRUint32 line_size);

    // override to clear all passwords associated with server
    NS_IMETHODIMP ForgetPassword();
    NS_IMETHOD GetCanSearchMessages(bool *canSearchMessages);
    NS_IMETHOD GetOfflineSupportLevel(PRInt32 *aSupportLevel);
    NS_IMETHOD GetDefaultCopiesAndFoldersPrefsToServer(bool *aCopiesAndFoldersOnServer);
    NS_IMETHOD GetCanCreateFoldersOnServer(bool *aCanCreateFoldersOnServer);
    NS_IMETHOD GetCanFileMessagesOnServer(bool *aCanFileMessagesOnServer);
    NS_IMETHOD GetFilterScope(nsMsgSearchScopeValue *filterScope);
    NS_IMETHOD GetSearchScope(nsMsgSearchScopeValue *searchScope);

    NS_IMETHOD GetSocketType(PRInt32 *aSocketType); // override nsMsgIncomingServer impl
    NS_IMETHOD SetSocketType(PRInt32 aSocketType); // override nsMsgIncomingServer impl

protected:
   virtual nsresult CreateRootFolderFromUri(const nsCString &serverUri,
                                            nsIMsgFolder **rootFolder);
    nsresult GetNntpConnection(nsIURI *url, nsIMsgWindow *window,
                               nsINNTPProtocol **aNntpConnection);
    nsresult CreateProtocolInstance(nsINNTPProtocol **aNntpConnection,
                                    nsIURI *url, nsIMsgWindow *window);
    bool ConnectionTimeOut(nsINNTPProtocol* aNntpConnection);
    nsCOMArray<nsINNTPProtocol> mConnectionCache;
    nsTArray<nsRefPtr<nsNntpMockChannel> > m_queuedChannels;

    /**
     * Downloads the newsgroup headers.
     */
    nsresult DownloadMail(nsIMsgWindow *aMsgWindow);

    NS_IMETHOD GetServerRequiresPasswordForBiff(bool *aServerRequiresPasswordForBiff);
    nsresult SetupNewsrcSaveTimer();
    static void OnNewsrcSaveTimer(nsITimer *timer, void *voidIncomingServer);
    void WriteLine(nsIOutputStream *stream, nsCString &str);

private:
    nsTArray<nsCString> mSubscribedNewsgroups;
    nsTArray<nsCString> mGroupsOnServer;
    nsTArray<nsCString> mSubscribeSearchResult;
    bool mSearchResultSortDescending;
    // the list of of subscribed newsgroups within a given
    // subscribed dialog session.  
    // we need to keep track of them so we know what to show as "checked"
    // in the search view
    nsTArray<nsCString> mTempSubscribed;
    nsCOMPtr<nsIAtom> mSubscribedAtom;
    nsCOMPtr<nsIAtom> mNntpAtom;

    nsString mSearchValue;
    nsCOMPtr<nsITreeBoxObject> mTree;
    nsCOMPtr<nsITreeSelection> mTreeSelection;

    bool     mHasSeenBeginGroups;
    bool     mGetOnlyNew;
    nsresult WriteHostInfoFile();
    nsresult LoadHostInfoFile();
    nsresult AddGroupOnServer(const nsACString &name);

    bool mNewsrcHasChanged;
    bool mHostInfoLoaded;
    bool mHostInfoHasChanged;
    nsCOMPtr <nsILocalFile> mHostInfoFile;
    
    PRUint32 mLastGroupDate;
    PRTime mFirstNewDate;
    PRInt32 mUniqueId;    
    PRUint32 mLastUpdatedTime;
    PRInt32 mVersion;
    bool mPostingAllowed;

    nsCOMPtr<nsITimer> mNewsrcSaveTimer;
    nsCOMPtr <nsIMsgWindow> mMsgWindow;

    nsCOMPtr <nsISubscribableServer> mInner;
    nsresult EnsureInner();
    nsresult ClearInner();
    nsresult IsValidRow(PRInt32 row);
    nsCOMPtr<nsILocalFile> mNewsrcFilePath;
};

#endif
