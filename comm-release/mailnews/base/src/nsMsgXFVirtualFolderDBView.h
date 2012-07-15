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
 * Portions created by the Initial Developer are Copyright (C) 2001
 * the Initial Developer. All Rights Reserved.
 *
 * Contributor(s):
 *   David Bienvenu <bienvenu@nventure.com>
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

#ifndef _nsMsgXFVirtualFolderDBView_H_
#define _nsMsgXFVirtualFolderDBView_H_

#include "nsMsgSearchDBView.h"
#include "nsIMsgCopyServiceListener.h"
#include "nsIMsgSearchNotify.h"
#include "nsCOMArray.h"

class nsMsgGroupThread;

class nsMsgXFVirtualFolderDBView : public nsMsgSearchDBView
{
public:
  nsMsgXFVirtualFolderDBView();
  virtual ~nsMsgXFVirtualFolderDBView();

  // we override all the methods, currently. Might change...
  NS_DECL_NSIMSGSEARCHNOTIFY

  virtual const char * GetViewName(void) {return "XFVirtualFolderView"; }
  NS_IMETHOD Open(nsIMsgFolder *folder, nsMsgViewSortTypeValue sortType, nsMsgViewSortOrderValue sortOrder, 
        nsMsgViewFlagsTypeValue viewFlags, PRInt32 *pCount);
  NS_IMETHOD CloneDBView(nsIMessenger *aMessengerInstance, nsIMsgWindow *aMsgWindow, 
                         nsIMsgDBViewCommandUpdater *aCmdUpdater, nsIMsgDBView **_retval);
  NS_IMETHOD CopyDBView(nsMsgDBView *aNewMsgDBView, nsIMessenger *aMessengerInstance, 
                        nsIMsgWindow *aMsgWindow, nsIMsgDBViewCommandUpdater *aCmdUpdater);
  NS_IMETHOD Close();
  NS_IMETHOD GetViewType(nsMsgViewTypeValue *aViewType);
  NS_IMETHOD DoCommand(nsMsgViewCommandTypeValue command);
  NS_IMETHOD SetViewFlags(nsMsgViewFlagsTypeValue aViewFlags);
  NS_IMETHOD OnHdrPropertyChanged(nsIMsgDBHdr *aHdrToChange, bool aPreChange, PRUint32 *aStatus, 
                                 nsIDBChangeListener * aInstigator);
  NS_IMETHOD GetMsgFolder(nsIMsgFolder **aMsgFolder);

  virtual nsresult OnNewHeader(nsIMsgDBHdr *newHdr, nsMsgKey parentKey, bool ensureListed);
  void UpdateCacheAndViewForPrevSearchedFolders(nsIMsgFolder *curSearchFolder);
  void UpdateCacheAndViewForFolder(nsIMsgFolder *folder, nsMsgKey *newHits, PRUint32 numNewHits);
  void RemovePendingDBListeners();

protected:

  virtual nsresult GetMessageEnumerator(nsISimpleEnumerator **enumerator);

  PRUint32 m_cachedFolderArrayIndex; // array index of next folder with cached hits to deal with.
  nsCOMArray<nsIMsgFolder> m_foldersSearchingOver;
  nsCOMArray<nsIMsgDBHdr> m_hdrHits;
  nsCOMPtr <nsIMsgFolder> m_curFolderGettingHits;
  PRUint32 m_curFolderStartKeyIndex; // keeps track of the index of the first hit from the cur folder
  bool m_curFolderHasCachedHits;
  bool m_doingSearch;
  // Are we doing a quick search on top of the virtual folder search?
  bool m_doingQuickSearch;
};

#endif
