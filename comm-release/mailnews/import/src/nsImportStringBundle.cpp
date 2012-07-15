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
#include "prprf.h"
#include "prmem.h"
#include "nsCOMPtr.h"
#include "nsIStringBundle.h"
#include "nsImportStringBundle.h"
#include "nsIServiceManager.h"
#include "nsIURI.h"
#include "nsServiceManagerUtils.h"
#include "nsComponentManagerUtils.h"
#include "mozilla/Services.h"

nsresult nsImportStringBundle::GetStringBundle(const char *aPropertyURL,
                                               nsIStringBundle **aBundle)
{
  nsresult rv;

  nsCOMPtr<nsIStringBundleService> sBundleService =
    mozilla::services::GetStringBundleService();
  NS_ENSURE_TRUE(sBundleService, NS_ERROR_UNEXPECTED);
  rv = sBundleService->CreateBundle(aPropertyURL, aBundle);

  return rv;
}

void nsImportStringBundle::GetStringByID(PRInt32 aStringID,
                                         nsIStringBundle *aBundle,
                                         nsString &aResult)
{
  aResult.Adopt(GetStringByID(aStringID, aBundle));
}

PRUnichar *nsImportStringBundle::GetStringByID(PRInt32 aStringID,
                                               nsIStringBundle *aBundle)
{
  if (aBundle)
  {
    PRUnichar *ptrv = nsnull;
    nsresult rv = aBundle->GetStringFromID(aStringID, &ptrv);

    if (NS_SUCCEEDED(rv) && ptrv)
      return(ptrv);
  }

  nsString resultString(NS_LITERAL_STRING("[StringID "));
  resultString.AppendInt(aStringID);
  resultString.AppendLiteral("?]");

  return ToNewUnicode(resultString);
}

void nsImportStringBundle::GetStringByName(const char *aName,
                                           nsIStringBundle *aBundle,
                                           nsString &aResult)
{
  aResult.Adopt(GetStringByName(aName, aBundle));
}

PRUnichar *nsImportStringBundle::GetStringByName(const char *aName,
                                                 nsIStringBundle *aBundle)
{
  if (aBundle)
  {
    PRUnichar *ptrv = nsnull;
    nsresult rv = aBundle->GetStringFromName(
        NS_ConvertUTF8toUTF16(aName).get(), &ptrv);

    if (NS_SUCCEEDED(rv) && ptrv)
      return(ptrv);
  }

  nsString resultString(NS_LITERAL_STRING("[StringName "));
  resultString.Append(NS_ConvertUTF8toUTF16(aName).get());
  resultString.AppendLiteral("?]");

  return ToNewUnicode(resultString);
}
