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
 * Mozilla Foundation.
 * Portions created by the Initial Developer are Copyright (C) 2011
 * the Initial Developer. All Rights Reserved.
 *
 * Contributor(s):
 *   David Bienvenu <dbienvenu@mozilla.com>
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
 
#include "msgCore.h"    // precompiled header...
#include "nsMsgLocalStoreUtils.h"
#include "nsILocalFile.h"
#include "prprf.h"

nsMsgLocalStoreUtils::nsMsgLocalStoreUtils()
{
}

nsresult
nsMsgLocalStoreUtils::AddDirectorySeparator(nsILocalFile *path)
{
  nsAutoString leafName;
  path->GetLeafName(leafName);
  leafName.AppendLiteral(FOLDER_SUFFIX);
  return path->SetLeafName(leafName);
}

bool
nsMsgLocalStoreUtils::nsShouldIgnoreFile(nsAString& name)
{
  PRUnichar firstChar = name.First();
  if (firstChar == '.' || firstChar == '#' ||
      name.CharAt(name.Length() - 1) == '~')
    return true;

  if (name.LowerCaseEqualsLiteral("msgfilterrules.dat") ||
      name.LowerCaseEqualsLiteral("rules.dat") ||
      name.LowerCaseEqualsLiteral("filterlog.html") ||
      name.LowerCaseEqualsLiteral("junklog.html") ||
      name.LowerCaseEqualsLiteral("rulesbackup.dat"))
    return true;

  // don't add summary files to the list of folders;
  // don't add popstate files to the list either, or rules (sort.dat).
  if (StringEndsWith(name, NS_LITERAL_STRING(".snm")) ||
      name.LowerCaseEqualsLiteral("popstate.dat") ||
      name.LowerCaseEqualsLiteral("sort.dat") ||
      name.LowerCaseEqualsLiteral("mailfilt.log") ||
      name.LowerCaseEqualsLiteral("filters.js") ||
      StringEndsWith(name, NS_LITERAL_STRING(".toc")))
    return true;

  // ignore RSS data source files
  if (name.LowerCaseEqualsLiteral("feeds.rdf") ||
      name.LowerCaseEqualsLiteral("feeditems.rdf"))
    return true;

  // The .mozmsgs dir is for spotlight support
    return (StringEndsWith(name, NS_LITERAL_STRING(".mozmsgs")) ||
            StringEndsWith(name, NS_LITERAL_STRING(".sbd")) ||
            StringEndsWith(name, NS_LITERAL_STRING(SUMMARY_SUFFIX)));
}

/**
 * We're passed a stream positioned at the start of the message.
 * We start reading lines, looking for x-mozilla-keys: headers; If we're
 * adding the keyword and we find a header with the desired keyword already
 * in it, we don't need to do anything. Likewise, if removing keyword and we
 * don't find it,we don't need to do anything. Otherwise, if adding, we need
 * to see if there's an x-mozilla-keys header with room for the new keyword.
 *  If so, we replace the corresponding number of spaces with the keyword.
 * If no room, we can't do anything until the folder is compacted and another
 * x-mozilla-keys header is added. In that case, we set a property
 * on the header, which the compaction code will check.
 * This is not true for maildir, however, since it won't require compaction.
 */

void
nsMsgLocalStoreUtils::ChangeKeywordsHelper(nsIMsgDBHdr *message,
                                           PRUint64 desiredOffset,
                                           nsLineBuffer<char> *lineBuffer,
                                           nsTArray<nsCString> &keywordArray,
                                           bool aAdd,
                                           nsIOutputStream *outputStream,
                                           nsISeekableStream *seekableStream,
                                           nsIInputStream *inputStream)
{
  PRUint32 bytesWritten;

  for (PRUint32 i = 0; i < keywordArray.Length(); i++)
  {
    nsCAutoString header;
    nsCAutoString keywords;
    bool done = false;
    PRUint32 len = 0;
    nsCAutoString keywordToWrite(" ");

    keywordToWrite.Append(keywordArray[i]);
    seekableStream->Seek(nsISeekableStream::NS_SEEK_SET, desiredOffset);
    // need to reset lineBuffer, which is cheaper than creating a new one.
    lineBuffer->start = lineBuffer->end = lineBuffer->buf;
    bool inKeywordHeader = false;
    bool foundKeyword = false;
    PRInt64 offsetToAddKeyword = 0;
    bool more;
    message->GetMessageSize(&len);
    // loop through
    while (!done)
    {
      PRInt64 lineStartPos;
      seekableStream->Tell(&lineStartPos);
      // we need to adjust the linestart pos by how much extra the line
      // buffer has read from the stream.
      lineStartPos -= (lineBuffer->end - lineBuffer->start);
      // NS_ReadLine doesn't return line termination chars.
      nsCString keywordHeaders;
      nsresult rv = NS_ReadLine(inputStream, lineBuffer, keywordHeaders, &more);
      if (NS_SUCCEEDED(rv))
      {
        if (keywordHeaders.IsEmpty())
          break; // passed headers; no x-mozilla-keywords header; give up.
        if (StringBeginsWith(keywordHeaders,
                             NS_LITERAL_CSTRING(HEADER_X_MOZILLA_KEYWORDS)))
          inKeywordHeader = true;
        else if (inKeywordHeader && (keywordHeaders.CharAt(0) == ' ' ||
                                     keywordHeaders.CharAt(0) == '\t'))
          ; // continuation header line
        else if (inKeywordHeader)
          break;
        else
          continue;
        PRUint32 keywordHdrLength = keywordHeaders.Length();
        PRInt32 startOffset, keywordLength;
        // check if we have the keyword
        if (MsgFindKeyword(keywordArray[i], keywordHeaders, &startOffset,
                           &keywordLength))
        {
          foundKeyword = true;
          if (!aAdd) // if we're removing, remove it, and break;
          {
            keywordHeaders.Cut(startOffset, keywordLength);
            for (PRInt32 j = keywordLength; j > 0; j--)
              keywordHeaders.Append(' ');
            seekableStream->Seek(nsISeekableStream::NS_SEEK_SET, lineStartPos);
            outputStream->Write(keywordHeaders.get(), keywordHeaders.Length(),
                                &bytesWritten);
          }
          offsetToAddKeyword = 0;
          // if adding and we already have the keyword, done
          done = true;
          break;
        }
        // argh, we need to check all the lines to see if we already have the
        // keyword, but if we don't find it, we want to remember the line and
        // position where we have room to add the keyword.
        if (aAdd)
        {
          nsCAutoString curKeywordHdr(keywordHeaders);
          // strip off line ending spaces.
          curKeywordHdr.Trim(" ", false, true);
          if (!offsetToAddKeyword && curKeywordHdr.Length() +
                keywordToWrite.Length() < keywordHdrLength)
            offsetToAddKeyword = lineStartPos + curKeywordHdr.Length();
        }
      }
    }
    if (aAdd && !foundKeyword)
    {
      if (!offsetToAddKeyword)
        message->SetUint32Property("growKeywords", 1);
      else
      {
        seekableStream->Seek(nsISeekableStream::NS_SEEK_SET,
                             offsetToAddKeyword);
        outputStream->Write(keywordToWrite.get(), keywordToWrite.Length(),
                            &bytesWritten);
      }
    }
  }
}

nsresult
nsMsgLocalStoreUtils::UpdateFolderFlag(nsIMsgDBHdr *mailHdr, bool bSet,
                                       nsMsgMessageFlagType flag,
                                       nsIOutputStream *fileStream)
{
  PRUint32 statusOffset;
  PRUint64 msgOffset;
  (void) mailHdr->GetStatusOffset(&statusOffset);
  // This probably means there's no x-mozilla-status header, so
  // we just ignore this.
  if (statusOffset == 0)
    return NS_OK;
  (void)mailHdr->GetMessageOffset(&msgOffset);
  PRUint64 statusPos = msgOffset + statusOffset;
  nsresult rv;
  nsCOMPtr<nsISeekableStream> seekableStream(do_QueryInterface(fileStream, &rv));
  NS_ENSURE_SUCCESS(rv, rv);
  rv = seekableStream->Seek(nsISeekableStream::NS_SEEK_SET, statusPos);
  NS_ENSURE_SUCCESS(rv, rv);
  char buf[50];
  buf[0] = '\0';
  nsCOMPtr<nsIInputStream> inputStream = do_QueryInterface(fileStream, &rv);
  NS_ENSURE_SUCCESS(rv, rv);
  PRUint32 bytesRead;
  if (NS_SUCCEEDED(inputStream->Read(buf, X_MOZILLA_STATUS_LEN + 6,
                                     &bytesRead)))
  {
    buf[bytesRead] = '\0';
    if (strncmp(buf, X_MOZILLA_STATUS, X_MOZILLA_STATUS_LEN) == 0 &&
      strncmp(buf + X_MOZILLA_STATUS_LEN, ": ", 2) == 0 &&
      strlen(buf) >= X_MOZILLA_STATUS_LEN + 6)
    {
      PRUint32 flags;
      PRUint32 bytesWritten;
      (void)mailHdr->GetFlags(&flags);
      if (!(flags & nsMsgMessageFlags::Expunged))
      {
        char *p = buf + X_MOZILLA_STATUS_LEN + 2;

        nsresult errorCode = NS_OK;
        flags = nsDependentCString(p).ToInteger(&errorCode, 16);

        PRUint32 curFlags;
        (void)mailHdr->GetFlags(&curFlags);
        flags = (flags & nsMsgMessageFlags::Queued) |
          (curFlags & ~nsMsgMessageFlags::RuntimeOnly);
        if (bSet)
          flags |= flag;
        else
          flags &= ~flag;
      }
      else
      {
        flags &= ~nsMsgMessageFlags::RuntimeOnly;
      }
      seekableStream->Seek(nsISeekableStream::NS_SEEK_SET, statusPos);
      // We are filing out x-mozilla-status flags here
      PR_snprintf(buf, sizeof(buf), X_MOZILLA_STATUS_FORMAT,
        flags & 0x0000FFFF);
      PRInt32 lineLen = PL_strlen(buf);
      PRUint64 status2Pos = statusPos + lineLen;
      fileStream->Write(buf, lineLen, &bytesWritten);

      if (flag & 0xFFFF0000)
      {
        // time to upate x-mozilla-status2
        // first find it by finding end of previous line, see bug 234935
        seekableStream->Seek(nsISeekableStream::NS_SEEK_SET, status2Pos);
        do
        {
          rv = inputStream->Read(buf, 1, &bytesRead);
          status2Pos++;
        } while (NS_SUCCEEDED(rv) && (*buf == '\n' || *buf == '\r'));
        status2Pos--;
        seekableStream->Seek(nsISeekableStream::NS_SEEK_SET, status2Pos);
        if (NS_SUCCEEDED(inputStream->Read(buf, X_MOZILLA_STATUS2_LEN + 10,
                                           &bytesRead)))
        {
          if (strncmp(buf, X_MOZILLA_STATUS2, X_MOZILLA_STATUS2_LEN) == 0 &&
            strncmp(buf + X_MOZILLA_STATUS2_LEN, ": ", 2) == 0 &&
            strlen(buf) >= X_MOZILLA_STATUS2_LEN + 10)
          {
            PRUint32 dbFlags;
            (void)mailHdr->GetFlags(&dbFlags);
            dbFlags &= 0xFFFF0000;
            seekableStream->Seek(nsISeekableStream::NS_SEEK_SET, status2Pos);
            PR_snprintf(buf, sizeof(buf), X_MOZILLA_STATUS2_FORMAT, dbFlags);
            fileStream->Write(buf, PL_strlen(buf), &bytesWritten);
          }
        }
      }
    }
    else
    {
#ifdef DEBUG
      printf("Didn't find %s where expected at position %ld\n"
        "instead, found %s.\n",
        X_MOZILLA_STATUS, (long) statusPos, buf);
#endif
      rv = NS_ERROR_FAILURE;
    }
  }
  else
    rv = NS_ERROR_FAILURE;
  return rv;
}
