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
 * Portions created by the Initial Developer are Copyright (C) 1999
 * the Initial Developer. All Rights Reserved.
 *
 * Contributor(s):
 *   Pierre Phaneuf <pp@ludusdesign.com>
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

#include "msgCore.h"  // for pre-compiled headers

#include "nsImapCore.h"
#include "nsImapFlagAndUidState.h"
#include "prcmon.h"
#include "nspr.h"

NS_IMPL_THREADSAFE_ISUPPORTS1(nsImapFlagAndUidState, nsIImapFlagAndUidState)

using namespace mozilla;

NS_IMETHODIMP nsImapFlagAndUidState::GetNumberOfMessages(PRInt32 *result)
{
  if (!result)
    return NS_ERROR_NULL_POINTER;
  *result = fUids.Length();
  return NS_OK;
}

NS_IMETHODIMP nsImapFlagAndUidState::GetUidOfMessage(PRInt32 zeroBasedIndex, PRUint32 *aResult)
{
  NS_ENSURE_ARG_POINTER(aResult);

  PR_CEnterMonitor(this);
  *aResult = fUids.SafeElementAt(zeroBasedIndex, nsMsgKey_None);
  PR_CExitMonitor(this);
  return NS_OK;
}

NS_IMETHODIMP nsImapFlagAndUidState::GetMessageFlags(PRInt32 zeroBasedIndex, PRUint16 *aResult)
{
  NS_ENSURE_ARG_POINTER(aResult);
  *aResult = fFlags.SafeElementAt(zeroBasedIndex, kNoImapMsgFlag);
  return NS_OK;
}

NS_IMETHODIMP nsImapFlagAndUidState::SetMessageFlags(PRInt32 zeroBasedIndex, unsigned short flags)
{
  if (zeroBasedIndex < fUids.Length())
    fFlags[zeroBasedIndex] = flags;
  return NS_OK;
}

NS_IMETHODIMP nsImapFlagAndUidState::GetNumberOfRecentMessages(PRInt32 *result)
{
  if (!result)
    return NS_ERROR_NULL_POINTER;
  
  PR_CEnterMonitor(this);
  PRUint32 counter = 0;
  PRInt32 numUnseenMessages = 0;
  
  for (counter = 0; counter < fUids.Length(); counter++)
  {
    if (fFlags[counter] & kImapMsgRecentFlag)
      numUnseenMessages++;
  }
  PR_CExitMonitor(this);
  
  *result = numUnseenMessages;
  
  return NS_OK;
}

NS_IMETHODIMP nsImapFlagAndUidState::GetPartialUIDFetch(bool *aPartialUIDFetch)
{
  NS_ENSURE_ARG_POINTER(aPartialUIDFetch);
  *aPartialUIDFetch = fPartialUIDFetch;
  return NS_OK;
}

/* amount to expand for imap entry flags when we need more */

nsImapFlagAndUidState::nsImapFlagAndUidState(PRInt32 numberOfMessages)
  : fUids(numberOfMessages),
    fFlags(numberOfMessages),
    mLock("nsImapFlagAndUidState.mLock")
{
  fSupportedUserFlags = 0;
  fNumberDeleted = 0;
  fPartialUIDFetch = true;
  m_customFlagsHash.Init(10);
}

/* static */PLDHashOperator nsImapFlagAndUidState::FreeCustomFlags(const PRUint32 &aKey, char *aData,
                                        void *closure)
{
  PR_Free(aData);
  return PL_DHASH_NEXT;
}

nsImapFlagAndUidState::~nsImapFlagAndUidState()
{
  if (m_customFlagsHash.IsInitialized())
    m_customFlagsHash.EnumerateRead(FreeCustomFlags, nsnull);
}

NS_IMETHODIMP
nsImapFlagAndUidState::OrSupportedUserFlags(uint16 flags)
{
  fSupportedUserFlags |= flags;
  return NS_OK;
}

NS_IMETHODIMP
nsImapFlagAndUidState::GetSupportedUserFlags(uint16 *aFlags)
{
  NS_ENSURE_ARG_POINTER(aFlags);
  *aFlags = fSupportedUserFlags;
  return NS_OK;
}

// we need to reset our flags, (re-read all) but chances are the memory allocation needed will be
// very close to what we were already using

NS_IMETHODIMP nsImapFlagAndUidState::Reset()
{
  PR_CEnterMonitor(this);
  fNumberDeleted = 0;
  if (m_customFlagsHash.IsInitialized())
    m_customFlagsHash.EnumerateRead(FreeCustomFlags, nsnull);
  m_customFlagsHash.Clear();
  fUids.Clear();
  fFlags.Clear();
  fPartialUIDFetch = true;
  PR_CExitMonitor(this);
  return NS_OK;
}


// Remove (expunge) a message from our array, since now it is gone for good

NS_IMETHODIMP nsImapFlagAndUidState::ExpungeByIndex(PRUint32 msgIndex)
{
  // protect ourselves in case the server gave us an index key of -1 or 0
  if ((PRInt32) msgIndex <= 0)
    return NS_ERROR_INVALID_ARG;

  if ((PRUint32) fUids.Length() < msgIndex)
    return NS_ERROR_INVALID_ARG;

  PR_CEnterMonitor(this);
  msgIndex--;  // msgIndex is 1-relative
  if (fFlags[msgIndex] & kImapMsgDeletedFlag) // see if we already had counted this one as deleted
    fNumberDeleted--;
  fUids.RemoveElementAt(msgIndex);
  fFlags.RemoveElementAt(msgIndex);
  PR_CExitMonitor(this);
  return NS_OK;
}


// adds to sorted list, protects against duplicates and going past array bounds.
NS_IMETHODIMP nsImapFlagAndUidState::AddUidFlagPair(PRUint32 uid, imapMessageFlagsType flags, PRUint32 zeroBasedIndex)
{
  if (uid == nsMsgKey_None) // ignore uid of -1
    return NS_OK;
  // check for potential overflow in buffer size for uid array
  if (zeroBasedIndex > 0x3FFFFFFF)
    return NS_ERROR_INVALID_ARG;
  PR_CEnterMonitor(this);
  // make sure there is room for this pair
  if (zeroBasedIndex >= fUids.Length())
  {
    PRInt32 sizeToGrowBy = zeroBasedIndex - fUids.Length() + 1;
    fUids.InsertElementsAt(fUids.Length(), sizeToGrowBy, 0);
    fFlags.InsertElementsAt(fFlags.Length(), sizeToGrowBy, 0);
  }

  fUids[zeroBasedIndex] = uid;
  fFlags[zeroBasedIndex] = flags;
  if (flags & kImapMsgDeletedFlag)
    fNumberDeleted++;
  PR_CExitMonitor(this);
  return NS_OK;
}


NS_IMETHODIMP nsImapFlagAndUidState::GetNumberOfDeletedMessages(PRInt32 *numDeletedMessages)
{
  NS_ENSURE_ARG_POINTER(numDeletedMessages);
  *numDeletedMessages = NumberOfDeletedMessages();
  return NS_OK;
}

PRInt32 nsImapFlagAndUidState::NumberOfDeletedMessages()
{
  return fNumberDeleted;
}
	
// since the uids are sorted, start from the back (rb)

PRUint32  nsImapFlagAndUidState::GetHighestNonDeletedUID()
{
  PRUint32 msgIndex = fUids.Length();
  do 
  {
    if (msgIndex <= 0)
      return(0);
    msgIndex--;
    if (fUids[msgIndex] && !(fFlags[msgIndex] & kImapMsgDeletedFlag))
      return fUids[msgIndex];
  }
  while (msgIndex > 0);
  return 0;
}


// Has the user read the last message here ? Used when we first open the inbox to see if there
// really is new mail there.

bool nsImapFlagAndUidState::IsLastMessageUnseen()
{
  PRUint32 msgIndex = fUids.Length();
  
  if (msgIndex <= 0)
    return false;
  msgIndex--;
  // if last message is deleted, it was probably filtered the last time around
  if (fUids[msgIndex] && (fFlags[msgIndex] & (kImapMsgSeenFlag | kImapMsgDeletedFlag)))
    return false;
  return true; 
}

// find a message flag given a key with non-recursive binary search, since some folders
// may have thousand of messages, once we find the key set its index, or the index of
// where the key should be inserted

imapMessageFlagsType nsImapFlagAndUidState::GetMessageFlagsFromUID(PRUint32 uid, bool *foundIt, PRInt32 *ndx)
{
  PR_CEnterMonitor(this);
  *foundIt = fUids.GreatestIndexLtEq(uid,
                                     nsDefaultComparator<PRUint32, PRUint32>(),
                                    (PRUint32 *) ndx);
  imapMessageFlagsType retFlags = (*foundIt) ? fFlags[*ndx] : kNoImapMsgFlag;
  PR_CExitMonitor(this);
  return retFlags;
}

NS_IMETHODIMP nsImapFlagAndUidState::AddUidCustomFlagPair(PRUint32 uid, const char *customFlag)
{
  MutexAutoLock mon(mLock);
  if (!m_customFlagsHash.IsInitialized())
    return NS_ERROR_OUT_OF_MEMORY;
  char *ourCustomFlags;
  char *oldValue = nsnull;
  m_customFlagsHash.Get(uid, &oldValue);
  if (oldValue)
  {
  // we'll store multiple keys as space-delimited since space is not
  // a valid character in a keyword. First, we need to look for the
    // customFlag in the existing flags;
    char *existingCustomFlagPtr = PL_strstr(oldValue, customFlag);
    PRUint32 customFlagLen = strlen(customFlag);
    while (existingCustomFlagPtr)
    {
      // if existing flags ends with this exact flag, or flag + ' ', we have this flag already;
      if (strlen(existingCustomFlagPtr) == customFlagLen || existingCustomFlagPtr[customFlagLen] == ' ')
        return NS_OK;
      // else, advance to next flag
      existingCustomFlagPtr = PL_strstr(existingCustomFlagPtr + 1, customFlag);
    }
    ourCustomFlags = (char *) PR_Malloc(strlen(oldValue) + customFlagLen + 2);
    strcpy(ourCustomFlags, oldValue);
    strcat(ourCustomFlags, " ");
    strcat(ourCustomFlags, customFlag);
    PR_Free(oldValue);
    m_customFlagsHash.Remove(uid);
  }
  else
  {
    ourCustomFlags = NS_strdup(customFlag);
    if (!ourCustomFlags)
      return NS_ERROR_OUT_OF_MEMORY;
  }
  return (m_customFlagsHash.Put(uid, ourCustomFlags) == 0) ? NS_OK : NS_ERROR_OUT_OF_MEMORY;
}

NS_IMETHODIMP nsImapFlagAndUidState::GetCustomFlags(PRUint32 uid, char **customFlags)
{
  MutexAutoLock mon(mLock);
  if (m_customFlagsHash.IsInitialized())
  {
    char *value = nsnull;
    m_customFlagsHash.Get(uid, &value);
    if (value)
    {
      *customFlags = NS_strdup(value);
      return (*customFlags) ? NS_OK : NS_ERROR_FAILURE;
    }
  }
  *customFlags = nsnull;
  return NS_OK;
}

NS_IMETHODIMP nsImapFlagAndUidState::ClearCustomFlags(PRUint32 uid)
{
  MutexAutoLock mon(mLock);
  m_customFlagsHash.Remove(uid);
  return NS_OK;
}

