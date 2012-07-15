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

#include "msgCore.h"
#include "nsImapUtils.h"
#include "nsCOMPtr.h"
#include "nsIServiceManager.h"
#include "prsystem.h"
#include "prprf.h"
#include "nsNetCID.h"

// stuff for temporary root folder hack
#include "nsIMsgAccountManager.h"
#include "nsIMsgIncomingServer.h"
#include "nsIImapIncomingServer.h"
#include "nsMsgBaseCID.h"
#include "nsImapCore.h"
#include "nsMsgUtils.h"
#include "nsImapFlagAndUidState.h"
#include "nsISupportsObsolete.h"
#include "nsIMAPNamespace.h"
#include "nsIImapFlagAndUidState.h"

nsresult
nsImapURI2FullName(const char* rootURI, const char* hostName, const char* uriStr,
                   char **name)
{
    nsCAutoString uri(uriStr);
    nsCAutoString fullName;
    if (uri.Find(rootURI) != 0)
      return NS_ERROR_FAILURE;
    fullName = Substring(uri, strlen(rootURI));
    uri = fullName;
    PRInt32 hostStart = uri.Find(hostName);
    if (hostStart <= 0) 
      return NS_ERROR_FAILURE;
    fullName = Substring(uri, hostStart);
    uri = fullName;
    PRInt32 hostEnd = uri.FindChar('/');
    if (hostEnd <= 0) 
      return NS_ERROR_FAILURE;
    fullName = Substring(uri, hostEnd + 1);
    if (fullName.IsEmpty())
      return NS_ERROR_FAILURE;
    *name = ToNewCString(fullName);
    return NS_OK;
}

/* parses ImapMessageURI */
nsresult nsParseImapMessageURI(const char* uri, nsCString& folderURI, PRUint32 *key, char **part)
{
  if(!key)
    return NS_ERROR_NULL_POINTER;

  nsCAutoString uriStr(uri);
  PRInt32 folderEnd = -1;
  // imap-message uri's can have imap:// url strings tacked on the end,
  // e.g., when opening/saving attachments. We don't want to look for '#'
  // in that part of the uri, if the attachment name contains '#',
  // so check for that here.
  if (StringBeginsWith(uriStr, NS_LITERAL_CSTRING("imap-message")))
    folderEnd = uriStr.Find("imap://");

  PRInt32 keySeparator = MsgRFindChar(uriStr, '#', folderEnd);
  if(keySeparator != -1)
  {
    PRInt32 keyEndSeparator = MsgFindCharInSet(uriStr, "/?&", keySeparator);
    nsAutoString folderPath;
    folderURI = StringHead(uriStr, keySeparator);
    folderURI.Cut(4, 8); // cut out the _message part of imap-message:
    // folder uri's don't have fully escaped usernames.
    PRInt32 atPos = folderURI.FindChar('@');
    if (atPos != -1)
    {
      nsCString unescapedName, escapedName;
      PRInt32 userNamePos = folderURI.Find("//") + 2;
      PRUint32 origUserNameLen = atPos - userNamePos;
      if (NS_SUCCEEDED(MsgUnescapeString(Substring(folderURI, userNamePos,
                                                   origUserNameLen),
                                         0, unescapedName)))
      {
        // Re-escape the username, matching the way we do it in uris, not the
        // way necko escapes urls. See nsMsgIncomingServer::GetServerURI.
        MsgEscapeString(unescapedName, nsINetUtil::ESCAPE_XALPHAS, escapedName);
        folderURI.Replace(userNamePos, origUserNameLen, escapedName);
      }
    }
    nsCAutoString keyStr;
    if (keyEndSeparator != -1)
      keyStr = Substring(uriStr, keySeparator + 1, keyEndSeparator - (keySeparator + 1));
    else
      keyStr = Substring(uriStr, keySeparator + 1);

    *key = strtoul(keyStr.get(), nsnull, 10);

    if (part && keyEndSeparator != -1)
    {
      PRInt32 partPos = MsgFind(uriStr, "part=", false, keyEndSeparator);
      if (partPos != -1)
      {
        *part = ToNewCString(Substring(uriStr, keyEndSeparator));
      }
    }
  }
  return NS_OK;
}

nsresult nsBuildImapMessageURI(const char *baseURI, PRUint32 key, nsCString& uri)
{
  uri.Append(baseURI);
  uri.Append('#');
  uri.AppendInt(key);
  return NS_OK;
}

nsresult nsCreateImapBaseMessageURI(const nsACString& baseURI, nsCString &baseMessageURI)
{
  nsCAutoString tailURI(baseURI);
  // chop off imap:/
  if (tailURI.Find(kImapRootURI) == 0)
    tailURI.Cut(0, PL_strlen(kImapRootURI));
  baseMessageURI = kImapMessageRootURI;
  baseMessageURI += tailURI;
  return NS_OK;
}

// nsImapMailboxSpec definition
NS_IMPL_THREADSAFE_ISUPPORTS1(nsImapMailboxSpec, nsIMailboxSpec)

nsImapMailboxSpec::nsImapMailboxSpec()
{
  mFolder_UIDVALIDITY = 0;
  mHighestModSeq = 0;
  mNumOfMessages = 0;
  mNumOfUnseenMessages = 0;
  mNumOfRecentMessages = 0;
  mNextUID = 0;
  
  mBoxFlags = 0;
  mSupportedUserFlags = 0;
  
  mHierarchySeparator = '\0';
  
  mFolderSelected = false;
  mDiscoveredFromLsub = false;
  
  mOnlineVerified = false;
  mNamespaceForFolder = nsnull;
}

nsImapMailboxSpec::~nsImapMailboxSpec()
{
}

NS_IMPL_GETSET(nsImapMailboxSpec, Folder_UIDVALIDITY, PRInt32, mFolder_UIDVALIDITY)
NS_IMPL_GETSET(nsImapMailboxSpec, HighestModSeq, PRUint64, mHighestModSeq)
NS_IMPL_GETSET(nsImapMailboxSpec, NumMessages, PRInt32, mNumOfMessages)
NS_IMPL_GETSET(nsImapMailboxSpec, NumUnseenMessages, PRInt32, mNumOfUnseenMessages)
NS_IMPL_GETSET(nsImapMailboxSpec, NumRecentMessages, PRInt32, mNumOfRecentMessages)
NS_IMPL_GETSET(nsImapMailboxSpec, NextUID, PRInt32, mNextUID)
NS_IMPL_GETSET(nsImapMailboxSpec, HierarchyDelimiter, char, mHierarchySeparator)
NS_IMPL_GETSET(nsImapMailboxSpec, FolderSelected, bool, mFolderSelected)
NS_IMPL_GETSET(nsImapMailboxSpec, DiscoveredFromLsub, bool, mDiscoveredFromLsub)
NS_IMPL_GETSET(nsImapMailboxSpec, OnlineVerified, bool, mOnlineVerified)
NS_IMPL_GETSET(nsImapMailboxSpec, SupportedUserFlags, PRUint32, mSupportedUserFlags)
NS_IMPL_GETSET(nsImapMailboxSpec, Box_flags, PRUint32, mBoxFlags)
NS_IMPL_GETSET(nsImapMailboxSpec, NamespaceForFolder, nsIMAPNamespace *, mNamespaceForFolder)

NS_IMETHODIMP nsImapMailboxSpec::GetAllocatedPathName(nsACString &aAllocatedPathName)
{
  aAllocatedPathName = mAllocatedPathName;
  return NS_OK;
} 

NS_IMETHODIMP nsImapMailboxSpec::SetAllocatedPathName(const nsACString &aAllocatedPathName)
{
  mAllocatedPathName = aAllocatedPathName;
  return NS_OK;
} 

NS_IMETHODIMP nsImapMailboxSpec::GetUnicharPathName(nsAString &aUnicharPathName)
{
  aUnicharPathName = aUnicharPathName;
  return NS_OK;
} 

NS_IMETHODIMP nsImapMailboxSpec::SetUnicharPathName(const nsAString &aUnicharPathName)
{
  mUnicharPathName = aUnicharPathName;
  return NS_OK;
} 

NS_IMETHODIMP nsImapMailboxSpec::GetHostName(nsACString &aHostName)
{
  aHostName = mHostName;
  return NS_OK;
} 

NS_IMETHODIMP nsImapMailboxSpec::SetHostName(const nsACString &aHostName)
{
  mHostName = aHostName;
  return NS_OK;
} 

NS_IMETHODIMP nsImapMailboxSpec::GetFlagState(nsIImapFlagAndUidState ** aFlagState)
{
  NS_ENSURE_ARG_POINTER(aFlagState);
  NS_IF_ADDREF(*aFlagState = mFlagState);
  return NS_OK;
}

NS_IMETHODIMP nsImapMailboxSpec::SetFlagState(nsIImapFlagAndUidState * aFlagState)
{
  NS_ENSURE_ARG_POINTER(aFlagState);
  mFlagState = aFlagState;
  return NS_OK;
}

nsImapMailboxSpec& nsImapMailboxSpec::operator= (const nsImapMailboxSpec& aCopy) 
{
  mFolder_UIDVALIDITY = aCopy.mFolder_UIDVALIDITY;
  mHighestModSeq = aCopy.mHighestModSeq;
  mNumOfMessages = aCopy.mNumOfMessages;
  mNumOfUnseenMessages = aCopy.mNumOfUnseenMessages;
  mNumOfRecentMessages = aCopy.mNumOfRecentMessages;
	
  mBoxFlags = aCopy.mBoxFlags;
  mSupportedUserFlags = aCopy.mSupportedUserFlags;
  
  mAllocatedPathName.Assign(aCopy.mAllocatedPathName);
  mUnicharPathName.Assign(aCopy.mUnicharPathName);
  mHierarchySeparator = mHierarchySeparator;
  mHostName.Assign(aCopy.mHostName);
	
  mFlagState = aCopy.mFlagState;
  mNamespaceForFolder = aCopy.mNamespaceForFolder;
	
  mFolderSelected = aCopy.mFolderSelected;
  mDiscoveredFromLsub = aCopy.mDiscoveredFromLsub;

  mOnlineVerified = aCopy.mOnlineVerified;
  
  return *this;
}

// use the flagState to determine if the gaps in the msgUids correspond to gaps in the mailbox,
// in which case we can still use ranges. If flagState is null, we won't do this.
void AllocateImapUidString(PRUint32 *msgUids, PRUint32 &msgCount, 
                           nsImapFlagAndUidState *flagState, nsCString &returnString)
{
  PRUint32 startSequence = (msgCount > 0) ? msgUids[0] : 0xFFFFFFFF;
  PRUint32 curSequenceEnd = startSequence;
  PRUint32 total = msgCount;
  PRInt32  curFlagStateIndex = -1;

  // a partial fetch flag state doesn't help us, so don't use it.
  if (flagState && flagState->GetPartialUIDFetch())
    flagState = nsnull;

  
  for (PRUint32 keyIndex = 0; keyIndex < total; keyIndex++)
  {
    PRUint32 curKey = msgUids[keyIndex];
    PRUint32 nextKey = (keyIndex + 1 < total) ? msgUids[keyIndex + 1] : 0xFFFFFFFF;
    bool lastKey = (nextKey == 0xFFFFFFFF);

    if (lastKey)
      curSequenceEnd = curKey;

    if (!lastKey)
    {
      if (nextKey == curSequenceEnd + 1)
      {
        curSequenceEnd = nextKey;
        curFlagStateIndex++;
        continue;
      }
      if (flagState)
      {
        if (curFlagStateIndex == -1)
        {
          bool foundIt;
          flagState->GetMessageFlagsFromUID(curSequenceEnd, &foundIt, &curFlagStateIndex);
          if (!foundIt)
          {
            NS_WARNING("flag state missing key");
            // The start of this sequence is missing from flag state, so move
            // on to the next key.
            curFlagStateIndex = -1;
            curSequenceEnd = startSequence = nextKey;
            continue;
          }
        }
        curFlagStateIndex++;
        PRUint32 nextUidInFlagState;
        nsresult rv = flagState->GetUidOfMessage(curFlagStateIndex, &nextUidInFlagState);
        if (NS_SUCCEEDED(rv) && nextUidInFlagState == nextKey)
        {
          curSequenceEnd = nextKey;
          continue;
        }
      }
    }
    if (curSequenceEnd > startSequence)
    {
      returnString.AppendInt((PRInt64) startSequence);
      returnString += ':';
      returnString.AppendInt((PRInt64) curSequenceEnd);
      startSequence = nextKey;
      curSequenceEnd = startSequence;
      curFlagStateIndex = -1;
    }
    else
    {
      startSequence = nextKey;
      curSequenceEnd = startSequence;
      returnString.AppendInt((PRInt64) msgUids[keyIndex]);
      curFlagStateIndex = -1;
    }
    // check if we've generated too long a string - if there's no flag state,
    // it means we just need to go ahead and generate a too long string
    // because the calling code won't handle breaking up the strings.
    if (flagState && returnString.Length() > 950) 
    {
      msgCount = keyIndex;
      break;
    }
    // If we are not the last item then we need to add the comma 
    // but it's important we do it here, after the length check 
    if (!lastKey) 
      returnString += ','; 
  }
}

void ParseUidString(const char *uidString, nsTArray<nsMsgKey> &keys)
{
  // This is in the form <id>,<id>, or <id1>:<id2>
  char curChar = *uidString;
  bool isRange = false;
  PRUint32 curToken;
  PRUint32 saveStartToken = 0;

  for (const char *curCharPtr = uidString; curChar && *curCharPtr;)
  {
    const char *currentKeyToken = curCharPtr;
    curChar = *curCharPtr;
    while (curChar != ':' && curChar != ',' && curChar != '\0')
      curChar = *curCharPtr++;

    // we don't need to null terminate currentKeyToken because strtoul
    // stops at non-numeric chars.
    curToken = strtoul(currentKeyToken, nsnull, 10);
    if (isRange)
    {
      while (saveStartToken < curToken)
        keys.AppendElement(saveStartToken++);
    }
    keys.AppendElement(curToken);
    isRange = (curChar == ':');
    if (isRange)
      saveStartToken = curToken + 1;
  }
}

void AppendUid(nsCString &msgIds, PRUint32 uid)
{
  char buf[20];
  PR_snprintf(buf, sizeof(buf), "%u", uid);
  msgIds.Append(buf);
}
