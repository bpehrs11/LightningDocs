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

#include "msgCore.h"
#include "nsMsgSpecialViews.h"
#include "nsIMsgThread.h"
#include "nsMsgMessageFlags.h"

nsMsgThreadsWithUnreadDBView::nsMsgThreadsWithUnreadDBView()
: m_totalUnwantedMessagesInView(0)
{
  
}

nsMsgThreadsWithUnreadDBView::~nsMsgThreadsWithUnreadDBView()
{
}

NS_IMETHODIMP nsMsgThreadsWithUnreadDBView::GetViewType(nsMsgViewTypeValue *aViewType)
{
    NS_ENSURE_ARG_POINTER(aViewType);
    *aViewType = nsMsgViewType::eShowThreadsWithUnread;
    return NS_OK;
}

bool nsMsgThreadsWithUnreadDBView::WantsThisThread(nsIMsgThread *threadHdr)
{
  if (threadHdr)
  {
    PRUint32 numNewChildren;

    threadHdr->GetNumUnreadChildren(&numNewChildren);
    if (numNewChildren > 0)
      return true;
    PRUint32 numChildren;
    threadHdr->GetNumChildren(&numChildren);
    m_totalUnwantedMessagesInView += numChildren;
  }
  return false;
}

nsresult nsMsgThreadsWithUnreadDBView::AddMsgToThreadNotInView(nsIMsgThread *threadHdr, nsIMsgDBHdr *msgHdr, bool ensureListed)
{
  nsresult rv = NS_OK;

  nsCOMPtr <nsIMsgDBHdr> parentHdr;
  PRUint32 msgFlags;
  msgHdr->GetFlags(&msgFlags);
  GetFirstMessageHdrToDisplayInThread(threadHdr, getter_AddRefs(parentHdr));
  if (parentHdr && (ensureListed || !(msgFlags & nsMsgMessageFlags::Read)))
  {
    nsMsgKey key;
    PRUint32 numMsgsInThread;
    rv = AddHdr(parentHdr);
    threadHdr->GetNumChildren(&numMsgsInThread);
    if (numMsgsInThread > 1)
    {
      parentHdr->GetMessageKey(&key);
      nsMsgViewIndex viewIndex = FindViewIndex(key);
      if (viewIndex != nsMsgViewIndex_None)
        OrExtraFlag(viewIndex, nsMsgMessageFlags::Elided | MSG_VIEW_FLAG_HASCHILDREN);
    }
    m_totalUnwantedMessagesInView -= numMsgsInThread;
  }
  else
    m_totalUnwantedMessagesInView++;
  return rv;
}

NS_IMETHODIMP
nsMsgThreadsWithUnreadDBView::CloneDBView(nsIMessenger *aMessengerInstance, nsIMsgWindow *aMsgWindow, nsIMsgDBViewCommandUpdater *aCmdUpdater, nsIMsgDBView **_retval)
{
  nsMsgThreadsWithUnreadDBView* newMsgDBView = new nsMsgThreadsWithUnreadDBView();

  if (!newMsgDBView)
    return NS_ERROR_OUT_OF_MEMORY;

  nsresult rv = CopyDBView(newMsgDBView, aMessengerInstance, aMsgWindow, aCmdUpdater);
  NS_ENSURE_SUCCESS(rv,rv);

  NS_IF_ADDREF(*_retval = newMsgDBView);
  return NS_OK;
}

NS_IMETHODIMP nsMsgThreadsWithUnreadDBView::GetNumMsgsInView(PRInt32 *aNumMsgs)
{
  nsresult rv = nsMsgDBView::GetNumMsgsInView(aNumMsgs);
  NS_ENSURE_SUCCESS(rv, rv);
  *aNumMsgs = *aNumMsgs - m_totalUnwantedMessagesInView;
  return rv;
}

nsMsgWatchedThreadsWithUnreadDBView::nsMsgWatchedThreadsWithUnreadDBView()
: m_totalUnwantedMessagesInView(0)
{
}

NS_IMETHODIMP nsMsgWatchedThreadsWithUnreadDBView::GetViewType(nsMsgViewTypeValue *aViewType)
{
    NS_ENSURE_ARG_POINTER(aViewType);
    *aViewType = nsMsgViewType::eShowWatchedThreadsWithUnread;
    return NS_OK;
}

bool nsMsgWatchedThreadsWithUnreadDBView::WantsThisThread(nsIMsgThread *threadHdr)
{
  if (threadHdr)
  {
    PRUint32 numNewChildren;
    PRUint32 threadFlags;

    threadHdr->GetNumUnreadChildren(&numNewChildren);
    threadHdr->GetFlags(&threadFlags);
    if (numNewChildren > 0 && (threadFlags & nsMsgMessageFlags::Watched) != 0)
      return true;
    PRUint32 numChildren;
    threadHdr->GetNumChildren(&numChildren);
    m_totalUnwantedMessagesInView += numChildren;
  }
  return false;
}

nsresult nsMsgWatchedThreadsWithUnreadDBView::AddMsgToThreadNotInView(nsIMsgThread *threadHdr, nsIMsgDBHdr *msgHdr, bool ensureListed)
{
  nsresult rv = NS_OK;
  PRUint32 threadFlags;
  PRUint32 msgFlags;
  msgHdr->GetFlags(&msgFlags);
  threadHdr->GetFlags(&threadFlags);
  if (threadFlags & nsMsgMessageFlags::Watched)
  {
    nsCOMPtr <nsIMsgDBHdr> parentHdr;
    GetFirstMessageHdrToDisplayInThread(threadHdr, getter_AddRefs(parentHdr));
    if (parentHdr && (ensureListed || !(msgFlags & nsMsgMessageFlags::Read)))
    {
      PRUint32 numChildren;
      threadHdr->GetNumChildren(&numChildren);
      rv = AddHdr(parentHdr);
      if (numChildren > 1)
      {
        nsMsgKey key;
        parentHdr->GetMessageKey(&key);
        nsMsgViewIndex viewIndex = FindViewIndex(key);
        if (viewIndex != nsMsgViewIndex_None)
          OrExtraFlag(viewIndex, nsMsgMessageFlags::Elided | MSG_VIEW_FLAG_ISTHREAD | MSG_VIEW_FLAG_HASCHILDREN | nsMsgMessageFlags::Watched);
      }
      m_totalUnwantedMessagesInView -= numChildren;
      return rv;
    }
  }
  m_totalUnwantedMessagesInView++;
  return rv;
}

NS_IMETHODIMP
nsMsgWatchedThreadsWithUnreadDBView::CloneDBView(nsIMessenger *aMessengerInstance, nsIMsgWindow *aMsgWindow, nsIMsgDBViewCommandUpdater *aCmdUpdater, nsIMsgDBView **_retval)
{
  nsMsgWatchedThreadsWithUnreadDBView* newMsgDBView = new nsMsgWatchedThreadsWithUnreadDBView();

  if (!newMsgDBView)
    return NS_ERROR_OUT_OF_MEMORY;

  nsresult rv = CopyDBView(newMsgDBView, aMessengerInstance, aMsgWindow, aCmdUpdater);
  NS_ENSURE_SUCCESS(rv,rv);

  NS_IF_ADDREF(*_retval = newMsgDBView);
  return NS_OK;
}

NS_IMETHODIMP
nsMsgWatchedThreadsWithUnreadDBView::GetNumMsgsInView(PRInt32 *aNumMsgs)
{
  nsresult rv = nsMsgDBView::GetNumMsgsInView(aNumMsgs);
  NS_ENSURE_SUCCESS(rv, rv);
  *aNumMsgs = *aNumMsgs - m_totalUnwantedMessagesInView;
  return rv;
}
