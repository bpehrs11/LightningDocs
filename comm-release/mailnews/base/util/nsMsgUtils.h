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

#ifndef _NSMSGUTILS_H
#define _NSMSGUTILS_H

#include "nsIURL.h"
#include "nsStringGlue.h"
#include "nsIEnumerator.h"
#include "msgCore.h"
#include "nsCOMPtr.h"
#include "MailNewsTypes2.h"
#include "nsTArray.h"
#include "nsIInterfaceRequestor.h"
#include "nsILoadGroup.h"
#include "nsISupportsArray.h"
#include "nsIAtom.h"
#include "nsINetUtil.h"
#include "nsServiceManagerUtils.h"
#include "nsUnicharUtils.h"
#include "nsILocalFile.h"

class nsILocalFile;
class nsIPrefBranch;
class nsIMsgFolder;
class nsIMsgMessageService;
class nsIUrlListener;
class nsIOutputStream;
class nsIInputStream;
class nsIMsgDatabase;
class nsIMutableArray;
class nsIProxyInfo;
class nsIMsgWindow;
class nsISupportsArray;
class nsIStreamListener;

//These are utility functions that can used throughout the mailnews code

NS_MSG_BASE nsresult GetMessageServiceContractIDForURI(const char *uri, nsCString &contractID);

NS_MSG_BASE nsresult GetMessageServiceFromURI(const nsACString& uri, nsIMsgMessageService **aMessageService);

NS_MSG_BASE nsresult GetMsgDBHdrFromURI(const char *uri, nsIMsgDBHdr **msgHdr);

NS_MSG_BASE nsresult CreateStartupUrl(const char *uri, nsIURI** aUrl);

NS_MSG_BASE nsresult NS_MsgGetPriorityFromString(
                       const char * const priority,
                       nsMsgPriorityValue & outPriority);

NS_MSG_BASE nsresult NS_MsgGetPriorityValueString(
                       const nsMsgPriorityValue p,
                       nsACString & outValueString);

NS_MSG_BASE nsresult NS_MsgGetUntranslatedPriorityName(
                       const nsMsgPriorityValue p,
                       nsACString & outName);

NS_MSG_BASE nsresult NS_MsgHashIfNecessary(nsAutoString &name);
NS_MSG_BASE nsresult NS_MsgHashIfNecessary(nsCAutoString &name);

NS_MSG_BASE nsresult FormatFileSize(PRUint64 size, bool useKB, nsAString &formattedSize);


/**
 * given a folder uri, return the path to folder in the user profile directory.
 *
 * @param aFolderURI uri of folder we want the path to, without the scheme
 * @param[out] aPathString result path string
 * @param aScheme scheme of the uri
 * @param[optional] aIsNewsFolder is this a news folder?
 */
NS_MSG_BASE nsresult 
NS_MsgCreatePathStringFromFolderURI(const char *aFolderURI,
                                    nsCString& aPathString,
                                    const nsCString &aScheme,
                                    bool aIsNewsFolder=false);

/**
 * Given a string and a length, removes any "Re:" strings from the front.
 * It also deals with that dumbass "Re[2]:" thing that some losing mailers do.
 *
 * If mailnews.localizedRe is set, it will also remove localized "Re:" strings.
 *
 * @return true if it made a change (in which case the caller should look to
 *         modifiedSubject for the result) and false otherwise (in which
 *         case the caller should look at stringp/length for the result) 
 *
 * @note In the case of a true return value, the string is not altered:
 *       the pointer to its head is merely advanced, and the length
 *       correspondingly decreased.
 * 
 * @note This API is insane and should be fixed.
 */
NS_MSG_BASE bool NS_MsgStripRE(const char **stringP, PRUint32 *lengthP, char **modifiedSubject=nsnull);

NS_MSG_BASE char * NS_MsgSACopy(char **destination, const char *source);

NS_MSG_BASE char * NS_MsgSACat(char **destination, const char *source);

NS_MSG_BASE nsresult NS_MsgEscapeEncodeURLPath(const nsAString& aStr,
                                               nsCString& aResult);

NS_MSG_BASE nsresult NS_MsgDecodeUnescapeURLPath(const nsACString& aPath,
                                                 nsAString& aResult);

NS_MSG_BASE bool WeAreOffline();

// Check if a folder with aFolderUri exists
NS_MSG_BASE nsresult GetExistingFolder(const nsCString& aFolderURI, nsIMsgFolder **aFolder);

// Escape lines starting with "From ", ">From ", etc. in a buffer.
NS_MSG_BASE nsresult EscapeFromSpaceLine(nsIOutputStream *ouputStream, char *start, const char *end);
NS_MSG_BASE bool IsAFromSpaceLine(char *start, const char *end);

NS_MSG_BASE nsresult NS_GetPersistentFile(const char *relPrefName,
                                          const char *absPrefName,
                                          const char *dirServiceProp, // Can be NULL
                                          bool& gotRelPref,
                                          nsILocalFile **aFile,
                                          nsIPrefBranch *prefBranch = nsnull);

NS_MSG_BASE nsresult NS_SetPersistentFile(const char *relPrefName,
                                          const char *absPrefName,
                                          nsILocalFile *aFile,
                                          nsIPrefBranch *prefBranch = nsnull);

NS_MSG_BASE nsresult IsRFC822HeaderFieldName(const char *aHdr, bool *aResult);

NS_MSG_BASE nsresult NS_GetUnicharPreferenceWithDefault(nsIPrefBranch *prefBranch,   //can be null, if so uses the root branch
                                                        const char *prefName,
                                                        const nsAString& defValue,
                                                        nsAString& prefValue);
 
NS_MSG_BASE nsresult NS_GetLocalizedUnicharPreferenceWithDefault(nsIPrefBranch *prefBranch,   //can be null, if so uses the root branch
                                                                 const char *prefName,
                                                                 const nsAString& defValue,
                                                                 nsAString& prefValue);

  /**
   * this needs a listener, because we might have to create the folder
   * on the server, and that is asynchronous
   */
NS_MSG_BASE nsresult GetOrCreateFolder(const nsACString & aURI, nsIUrlListener *aListener);

// Returns true if the nsIURI is a message under an RSS account
NS_MSG_BASE nsresult IsRSSArticle(nsIURI * aMsgURI, bool *aIsRSSArticle);

// digest needs to be a pointer to a 16 byte buffer
#define DIGEST_LENGTH 16

NS_MSG_BASE nsresult MSGCramMD5(const char *text, PRInt32 text_len, const char *key, PRInt32 key_len, unsigned char *digest);
NS_MSG_BASE nsresult MSGApopMD5(const char *text, PRInt32 text_len, const char *password, PRInt32 password_len, unsigned char *digest);

// helper functions to convert a 64bits PRTime into a 32bits value (compatible time_t) and vice versa.
NS_MSG_BASE void PRTime2Seconds(PRTime prTime, PRUint32 *seconds);
NS_MSG_BASE void PRTime2Seconds(PRTime prTime, PRInt32 *seconds);
NS_MSG_BASE void Seconds2PRTime(PRUint32 seconds, PRTime *prTime);
// helper function to generate current date+time as a string
NS_MSG_BASE void MsgGenerateNowStr(nsACString &nowStr);

// Appends the correct summary file extension onto the supplied fileLocation
// and returns it in summaryLocation.
NS_MSG_BASE nsresult GetSummaryFileLocation(nsILocalFile* fileLocation,
                                            nsILocalFile** summaryLocation);

// Gets a special directory and appends the supplied file name onto it.
NS_MSG_BASE nsresult GetSpecialDirectoryWithFileName(const char* specialDirName,
                                                     const char* fileName,
                                                     nsIFile** result);

NS_MSG_BASE nsresult MsgGetFileStream(nsILocalFile *file, nsIOutputStream **fileStream);

NS_MSG_BASE nsresult MsgReopenFileStream(nsILocalFile *file, nsIInputStream *fileStream);

// Automatically creates an output stream with a 4K buffer
NS_MSG_BASE nsresult MsgNewBufferedFileOutputStream(nsIOutputStream **aResult, nsIFile *aFile, PRInt32 aIOFlags = -1, PRInt32 aPerm = -1);

// fills in the position of the passed in keyword in the passed in keyword list
// and returns false if the keyword isn't present
NS_MSG_BASE bool MsgFindKeyword(const nsCString &keyword, nsCString &keywords, PRInt32 *aStartOfKeyword, PRInt32 *aLength);

NS_MSG_BASE bool MsgHostDomainIsTrusted(nsCString &host, nsCString &trustedMailDomains);

// gets an nsILocalFile from a UTF-8 file:// path
NS_MSG_BASE nsresult MsgGetLocalFileFromURI(const nsACString &aUTF8Path, nsILocalFile **aFile);

NS_MSG_BASE void MsgStripQuotedPrintable (unsigned char *src);

/*
 * Utility function copied from nsReadableUtils
 */
NS_MSG_BASE bool MsgIsUTF8(const nsACString& aString);

/*
 * Utility functions that call functions from nsINetUtil
 */

NS_MSG_BASE nsresult MsgEscapeString(const nsACString &aStr,
                                     PRUint32 aType, nsACString &aResult);

NS_MSG_BASE nsresult MsgUnescapeString(const nsACString &aStr, 
                                       PRUint32 aFlags, nsACString &aResult);

NS_MSG_BASE nsresult MsgEscapeURL(const nsACString &aStr, PRUint32 aFlags,
                                  nsACString &aResult);

// Converts an array of nsMsgKeys plus a database, to an array of nsIMsgDBHdrs.
NS_MSG_BASE nsresult MsgGetHeadersFromKeys(nsIMsgDatabase *aDB, 
                                           const nsTArray<nsMsgKey> &aKeys,
                                           nsIMutableArray *aHeaders);
 
NS_MSG_BASE nsresult MsgExamineForProxy(const char *scheme, const char *host,
                                        PRInt32 port, nsIProxyInfo **proxyInfo);

NS_MSG_BASE PRInt32 MsgFindCharInSet(const nsCString &aString,
                                     const char* aChars, PRUint32 aOffset = 0);
NS_MSG_BASE PRInt32 MsgFindCharInSet(const nsString &aString,
                                     const char* aChars, PRUint32 aOffset = 0);


// advances bufferOffset to the beginning of the next line, if we don't
// get to maxBufferOffset first. Returns false if we didn't get to the
// next line.
NS_MSG_BASE bool MsgAdvanceToNextLine(const char *buffer, PRUint32 &bufferOffset,
                                   PRUint32 maxBufferOffset);

/**
 * Alerts the user that the login to the server failed. Asks whether the
 * connection should: retry, cancel, or request a new password.
 *
 * @param aMsgWindow The message window associated with this action (cannot
 *                   be null).
 * @param aHostname  The hostname of the server for which the login failed.
 * @param aResult    The button pressed. 0 for retry, 1 for cancel,
 *                   2 for enter a new password.
 * @return           NS_OK for success, NS_ERROR_* if there was a failure in
 *                   creating the dialog.
 */
NS_MSG_BASE nsresult MsgPromptLoginFailed(nsIMsgWindow *aMsgWindow,
                                          const nsCString &aHostname,
                                          PRInt32 *aResult);

/**
 * Calculate a PRTime value used to determine if a date is XX
 * days ago. This is used by various retention setting algorithms.
 */
NS_MSG_BASE PRTime MsgConvertAgeInDaysToCutoffDate(PRInt32 ageInDays);

/**
 * Converts the passed in term list to its string representation.
 *
 * @param      aTermList    Array of nsIMsgSearchTerms
 * @param[out] aOutString   result representation of search terms.
 *
 */
NS_MSG_BASE nsresult MsgTermListToString(nsISupportsArray *aTermList, nsCString &aOutString);

NS_MSG_BASE nsresult
MsgStreamMsgHeaders(nsIInputStream *aInputStream, nsIStreamListener *aConsumer);

/**
 * convert string to PRUint64
 *
 * @param str conveted string
 * @returns   PRUint64 vaule for success, 0 for parse failure
 */
NS_MSG_BASE PRUint64 ParseUint64Str(const char *str);

/**
 * Detect charset of file
 *
 * @param      aFile    The target of nsILocalFile
 * @param[out] aCharset The charset string
 */
NS_MSG_BASE nsresult MsgDetectCharsetFromFile(nsILocalFile *aFile, nsACString &aCharset);

/*
 * Converts a buffer to plain text. Some conversions may
 * or may not work with certain end charsets which is why we
 * need that as an argument to the function. If charset is
 * unknown or deemed of no importance NULL could be passed.
 * @param[in/out] aConBuf      Variable with the text to convert
 * @param         formatFlowed Use format flowed?
 * @param         formatOutput Reformat the output?
 */
NS_MSG_BASE nsresult ConvertBufToPlainText(nsString &aConBuf, bool formatFlowed, bool formatOutput);

/**
 * The following definitons exist for compatibility between the internal and
 * external APIs. Where possible they just forward to the existing API.
 */

#ifdef MOZILLA_INTERNAL_API
#include "nsEscape.h"

/**
 * The internal API expects nsCaseInsensitiveC?StringComparator() and true.
 * Redefine CaseInsensitiveCompare so that Find works.
 */
#define CaseInsensitiveCompare true
/**
 * The following methods are not exposed to the external API, but when we're
 * using the internal API we can simply redirect the calls appropriately.
 */
#define MsgLowerCaseEqualsLiteral(str, l) \
        (str).LowerCaseEqualsLiteral(l)
#define MsgRFindChar(str, ch, len) \
        (str).RFindChar(ch, len)
#define MsgCompressWhitespace(str) \
        (str).CompressWhitespace()
#define MsgEscapeHTML(str) \
        nsEscapeHTML(str)
#define MsgEscapeHTML2(buffer, len) \
        nsEscapeHTML2(buffer, len)
#define MsgReplaceSubstring(str, what, replacement) \
        (str).ReplaceSubstring(what, replacement)
#define MsgIsUTF8(str) \
        IsUTF8(str)
#define MsgNewInterfaceRequestorAggregation(aFirst, aSecond, aResult) \
        NS_NewInterfaceRequestorAggregation(aFirst, aSecond, aResult)
#define MsgNewNotificationCallbacksAggregation(aCallbacks, aLoadGroup, aResult) \
        NS_NewNotificationCallbacksAggregation(aCallbacks, aLoadGroup, aResult)
#define MsgGetAtom(aString) \
        do_GetAtom(aString)
#define MsgNewAtom(aString) \
        NS_NewAtom(aString)
#define MsgReplaceChar(aString, aNeedle, aReplacement) \
        (aString).ReplaceChar(aNeedle, aReplacement)
#define MsgFind(str, what, ignore_case, offset) \
        (str).Find(what, ignore_case, offset)

#else

/**
 * The external API expects CaseInsensitiveCompare. Redefine
 * nsCaseInsensitiveC?StringComparator() so that Equals works.
 */
#define nsCaseInsensitiveCStringComparator() \
        CaseInsensitiveCompare
#define nsCaseInsensitiveStringComparator() \
        CaseInsensitiveCompare
/// The external API does not provide kNotFound.
#define kNotFound -1
/**
 * The external API does not provide the following methods. While we can
 * reasonably easily define them in terms of existing methods, we only want
 * to do this when using the external API.
 */
#define AppendASCII \
        AppendLiteral
#define AppendUTF16toUTF8(source, dest) \
        (dest).Append(NS_ConvertUTF16toUTF8(source))
#define AppendUTF8toUTF16(source, dest) \
        (dest).Append(NS_ConvertUTF8toUTF16(source))
#define AppendASCIItoUTF16(source, dest) \
        (dest).Append(NS_ConvertASCIItoUTF16(source))
#define Compare(str1, str2, comp) \
        (str1).Compare(str2, comp)
#define CaseInsensitiveFindInReadable(what, str) \
        ((str).Find(what, CaseInsensitiveCompare) != kNotFound)
#define LossyAppendUTF16toASCII(source, dest) \
        (dest).Append(NS_LossyConvertUTF16toASCII(source))
#define Last() \
        EndReading()[-1]
#define SetCharAt(ch, index) \
        Replace(index, 1, ch)
#define NS_NewISupportsArray(result) \
        CallCreateInstance(NS_SUPPORTSARRAY_CONTRACTID, static_cast<nsISupportsArray**>(result))
/**
 * The internal and external methods expect the parameters in a different order.
 * The internal API also always expects a flag rather than a comparator.
 */
inline PRInt32 MsgFind(nsAString &str, const char *what, bool ignore_case, PRUint32 offset)
{
  return str.Find(what, offset, ignore_case);
}

inline PRInt32 MsgFind(nsACString &str, const char *what, bool ignore_case, PRUint32 offset)
{
  if (ignore_case)
    return str.Find(what, offset, CaseInsensitiveCompare);
  return str.Find(what, offset);
}

inline PRInt32 MsgFind(nsACString &str, const nsACString &what, bool ignore_case, PRUint32 offset)
{
  if (ignore_case)
    return str.Find(what, offset, CaseInsensitiveCompare);
  return str.Find(what, offset);
}

/**
 * The following methods are not exposed to the external API so we define
 * equivalent versions here.
 */
/// Equivalent of LowerCaseEqualsLiteral(literal)
#define MsgLowerCaseEqualsLiteral(str, literal) \
        (str).Equals(literal, CaseInsensitiveCompare)
/// Equivalent of RFindChar(ch, len)
#define MsgRFindChar(str, ch, len) \
        StringHead(str, len).RFindChar(ch)
/// Equivalent of aString.CompressWhitespace()
NS_MSG_BASE void MsgCompressWhitespace(nsCString& aString);
/// Equivalent of nsEscapeHTML(aString)
NS_MSG_BASE char *MsgEscapeHTML(const char *aString);
/// Equivalent of nsEscapeHTML2(aBuffer, aLen)
NS_MSG_BASE PRUnichar *MsgEscapeHTML2(const PRUnichar *aBuffer, PRInt32 aLen);
// Existing replacement for IsUTF8
NS_MSG_BASE bool MsgIsUTF8(const nsACString& aString);
/// Equivalent of NS_NewAtom(aUTF8String)
NS_MSG_BASE nsIAtom* MsgNewAtom(const char* aString);
/// Replacement of NS_RegisterStaticAtoms
NS_MSG_BASE nsIAtom* MsgNewPermanentAtom(const char* aString);
/// Equivalent of do_GetAtom(aUTF8String)
inline already_AddRefed<nsIAtom> MsgGetAtom(const char* aUTF8String)
{
  return MsgNewAtom(aUTF8String);
}
/// Equivalent of ns(C)String::ReplaceSubstring(what, replacement)
NS_MSG_BASE void MsgReplaceSubstring(nsAString &str, const nsAString &what, const nsAString &replacement);
NS_MSG_BASE void MsgReplaceSubstring(nsACString &str, const char *what, const char *replacement);
/// Equivalent of ns(C)String::ReplaceChar(what, replacement)
NS_MSG_BASE void MsgReplaceChar(nsString& str, const char *set, const PRUnichar replacement);
NS_MSG_BASE void MsgReplaceChar(nsCString& str, const char needle, const char replacement);
// Equivalent of NS_NewInterfaceRequestorAggregation(aFirst, aSecond, aResult)
NS_MSG_BASE nsresult MsgNewInterfaceRequestorAggregation(nsIInterfaceRequestor *aFirst,
                                                         nsIInterfaceRequestor *aSecond,
                                                         nsIInterfaceRequestor **aResult);

/**
 * This function is based on NS_NewNotificationCallbacksAggregation from
 * nsNetUtil.h
 *
 * This function returns a nsIInterfaceRequestor instance that returns the
 * same result as NS_QueryNotificationCallbacks when queried.
 */
inline nsresult
MsgNewNotificationCallbacksAggregation(nsIInterfaceRequestor  *callbacks,
                                       nsILoadGroup           *loadGroup,
                                       nsIInterfaceRequestor **result)
{
    nsCOMPtr<nsIInterfaceRequestor> cbs;
    if (loadGroup)
        loadGroup->GetNotificationCallbacks(getter_AddRefs(cbs));
    return MsgNewInterfaceRequestorAggregation(callbacks, cbs, result);
}

/**
 * Helper class for do_QueryElementAt
 */
class NS_MSG_BASE MsgQueryElementAt : public nsCOMPtr_helper
  {
    public:
      MsgQueryElementAt( nsISupportsArray* anArray, PRUint32 aIndex, nsresult* aErrorPtr )
          : mArray(anArray),
            mIndex(aIndex),
            mErrorPtr(aErrorPtr)
        {
          // nothing else to do here
        }
      virtual nsresult NS_FASTCALL operator()( const nsIID& aIID, void** ) const;
    private:
      nsISupportsArray*  mArray;
      PRUint32           mIndex;
      nsresult*          mErrorPtr;
  };

/**
 * Overload function for nsISupportsArray. The do_QueryElementAt which belongs to
 * internal API only accepts nsICollection* aCollection.
 */
inline
const MsgQueryElementAt
do_QueryElementAt( nsISupportsArray* array, PRUint32 aIndex, nsresult* aErrorPtr = 0 )
{
    return MsgQueryElementAt(array, aIndex, aErrorPtr);
}
#endif

#endif
