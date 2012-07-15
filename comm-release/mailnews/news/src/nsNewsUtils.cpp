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
 *   H�kan Waara <hwaara@chello.se>
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
#include "nntpCore.h"
#include "nsNewsUtils.h"
#include "nsMsgUtils.h"


/* parses NewsMessageURI */
nsresult
nsParseNewsMessageURI(const char* uri, nsCString& group, PRUint32 *key)
{
  NS_ENSURE_ARG_POINTER(uri);
  NS_ENSURE_ARG_POINTER(key);

  nsCAutoString uriStr(uri);
  PRInt32 keySeparator = uriStr.FindChar('#');
  if(keySeparator != -1)
  {
    PRInt32 keyEndSeparator = MsgFindCharInSet(uriStr, "?&", keySeparator);

    // Grab between the last '/' and the '#' for the key
    group = StringHead(uriStr, keySeparator);
    PRInt32 groupSeparator = group.RFind("/");
    if (groupSeparator == -1)
      return NS_ERROR_FAILURE;

    // Our string APIs don't let us unescape into the same buffer from earlier,
    // so escape into a temporary
    nsCAutoString unescapedGroup;
    MsgUnescapeString(Substring(group, groupSeparator + 1), 0, unescapedGroup);
    group = unescapedGroup;

    nsCAutoString keyStr;
    if (keyEndSeparator != -1)
      keyStr = Substring(uriStr, keySeparator + 1, keyEndSeparator - (keySeparator + 1));
    else
      keyStr = Substring(uriStr, keySeparator + 1);
    nsresult errorCode;
    *key = keyStr.ToInteger(&errorCode);

    return errorCode;
  }
  return NS_ERROR_FAILURE;
}

nsresult nsCreateNewsBaseMessageURI(const char *baseURI, nsCString &baseMessageURI)
{
  nsCAutoString tailURI(baseURI);

  // chop off news:/
  if (tailURI.Find(kNewsRootURI) == 0)
    tailURI.Cut(0, PL_strlen(kNewsRootURI));

  baseMessageURI = kNewsMessageRootURI;
  baseMessageURI += tailURI;

  return NS_OK;
}
