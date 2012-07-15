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
 * Portions created by the Initial Developer are Copyright (C) 2001
 * the Initial Developer. All Rights Reserved.
 *
 * Contributor(s):
 *   Navin Gupta <naving@netscape.com> (Original Author)
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

#ifndef _nsMsgQuickSearchDBView_H_
#define _nsMsgQuickSearchDBView_H_

#include "nsMsgThreadedDBView.h"
#include "nsIMsgSearchNotify.h"
#include "nsIMsgSearchSession.h"
#include "nsCOMArray.h"
#include "nsIMsgHdr.h"


class nsMsgQuickSearchDBView : public nsMsgThreadedDBView, public nsIMsgSearchNotify
{
public:
  nsMsgQuickSearchDBView();
  virtual ~nsMsgQuickSearchDBView();

  NS_DECL_ISUPPORTS_INHERITED
  NS_DECL_NSIMSGSEARCHNOTIFY

  virtual const char * GetViewName(void) {return "QuickSearchView"; }
  NS_IMETHOD Open(nsIMsgFolder *folder, nsMsgViewSortTypeValue sortType, 
                  nsMsgViewSortOrderValue sortOrder, 
                  nsMsgViewFlagsTypeValue viewFlags, PRInt32 *pCount);
  NS_IMETHOD OpenWithHdrs(nsISimpleEnumerator *aHeaders, 
                          nsMsgViewSortTypeValue aSortType, 
                          nsMsgViewSortOrderValue aSortOrder, 
                          nsMsgViewFlagsTypeValue aViewFlags, 
                          PRInt32 *aCount);
  NS_IMETHOD CloneDBView(nsIMessenger *aMessengerInstance,
                         nsIMsgWindow *aMsgWindow,
                         nsIMsgDBViewCommandUpdater *aCommandUpdater,
                         nsIMsgDBView **_retval);
  NS_IMETHOD CopyDBView(nsMsgDBView *aNewMsgDBView,
                        nsIMessenger *aMessengerInstance,
                        nsIMsgWindow *aMsgWindow,
                        nsIMsgDBViewCommandUpdater *aCmdUpdater);
  NS_IMETHOD DoCommand(nsMsgViewCommandTypeValue aCommand);
  NS_IMETHOD GetViewType(nsMsgViewTypeValue *aViewType);
  NS_IMETHOD SetViewFlags(nsMsgViewFlagsTypeValue aViewFlags);
  NS_IMETHOD SetSearchSession(nsIMsgSearchSession *aSearchSession);
  NS_IMETHOD GetSearchSession(nsIMsgSearchSession* *aSearchSession);
  NS_IMETHOD OnHdrFlagsChanged(nsIMsgDBHdr *aHdrChanged, PRUint32 aOldFlags, 
                         PRUint32 aNewFlags, nsIDBChangeListener *aInstigator);
  NS_IMETHOD OnHdrPropertyChanged(nsIMsgDBHdr *aHdrToChange, bool aPreChange, PRUint32 *aStatus, 
                                 nsIDBChangeListener * aInstigator);
  NS_IMETHOD OnHdrDeleted(nsIMsgDBHdr *aHdrDeleted, nsMsgKey aParentKey,
                          PRInt32 aFlags, nsIDBChangeListener *aInstigator);
  NS_IMETHOD GetNumMsgsInView(PRInt32 *aNumMsgs);

protected:
  nsWeakPtr m_searchSession;
  nsTArray<nsMsgKey> m_origKeys;
  bool      m_usingCachedHits;
  bool      m_cacheEmpty;
  nsCOMArray <nsIMsgDBHdr> m_hdrHits;
  virtual nsresult AddHdr(nsIMsgDBHdr *msgHdr, nsMsgViewIndex *resultIndex = nsnull);
  virtual nsresult OnNewHeader(nsIMsgDBHdr *newHdr, nsMsgKey aParentKey, bool ensureListed);
  virtual nsresult DeleteMessages(nsIMsgWindow *window, nsMsgViewIndex *indices, PRInt32 numIndices, bool deleteStorage);
  virtual nsresult SortThreads(nsMsgViewSortTypeValue sortType, nsMsgViewSortOrderValue sortOrder);
  virtual nsresult GetFirstMessageHdrToDisplayInThread(nsIMsgThread *threadHdr, nsIMsgDBHdr **result);
  virtual nsresult ExpansionDelta(nsMsgViewIndex index, PRInt32 *expansionDelta);
  virtual nsresult ListCollapsedChildren(nsMsgViewIndex viewIndex,
                                         nsIMutableArray *messageArray);
  virtual nsresult ListIdsInThread(nsIMsgThread *threadHdr, nsMsgViewIndex startOfThreadViewIndex, PRUint32 *pNumListed);
  virtual nsresult ListIdsInThreadOrder(nsIMsgThread *threadHdr,
                                        nsMsgKey parentKey, PRUint32 level,
                                        nsMsgViewIndex *viewIndex,
                                        PRUint32 *pNumListed);
  virtual nsresult ListIdsInThreadOrder(nsIMsgThread *threadHdr,
                                        nsMsgKey parentKey, PRUint32 level,
                                        PRUint32 callLevel,
                                        nsMsgKey keyToSkip,
                                        nsMsgViewIndex *viewIndex,
                                        PRUint32 *pNumListed);
  virtual nsresult GetMessageEnumerator(nsISimpleEnumerator **enumerator);
  void      SavePreSearchInfo();
  void      ClearPreSearchInfo();

};

#endif
