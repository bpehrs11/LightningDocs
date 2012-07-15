/* -*- Mode: C++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*-
 *
 * ***** BEGIN LICENSE BLOCK *****
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
 * The Original Code is mozilla.org Code.
 *
 * The Initial Developer of the Original Code is
 * Netscape Communications Corporation.
 * Portions created by the Initial Developer are Copyright (C) 1999
 * the Initial Developer. All Rights Reserved.
 *
 * Contributor(s):
 *   Jeff Beckley <beckley@qualcomm.com>
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
#include "nsEudoraStringBundle.h"
#include "nsServiceManagerUtils.h"
#include "nsIURI.h"
#include "nsTextFormatter.h"
#include "mozilla/Services.h"

#define EUDORA_MSGS_URL       "chrome://messenger/locale/eudoraImportMsgs.properties"

nsIStringBundle *  nsEudoraStringBundle::m_pBundle = nsnull;

nsIStringBundle *nsEudoraStringBundle::GetStringBundle( void)
{
  if (m_pBundle)
    return m_pBundle;

  const char*       propertyURL = EUDORA_MSGS_URL;
  nsIStringBundle*  sBundle = nsnull;

  nsCOMPtr<nsIStringBundleService> sBundleService =
    mozilla::services::GetStringBundleService();
  if (sBundleService)
    sBundleService->CreateBundle(propertyURL, &sBundle);

  m_pBundle = sBundle;
  return( sBundle);
}

void nsEudoraStringBundle::GetStringByID( PRInt32 stringID, nsString& result)
{

  PRUnichar *ptrv = GetStringByID(stringID);
  result = ptrv;
  FreeString( ptrv);
}

PRUnichar *nsEudoraStringBundle::GetStringByID(PRInt32 stringID)
{
  if (!m_pBundle)
    m_pBundle = GetStringBundle();

  if (m_pBundle)
  {
    PRUnichar *ptrv = nsnull;
    nsresult rv = m_pBundle->GetStringFromID(stringID, &ptrv);

    if (NS_SUCCEEDED( rv) && ptrv)
      return( ptrv);
  }

  nsString resultString(NS_LITERAL_STRING("[StringID "));
  resultString.AppendInt(stringID);
  resultString.AppendLiteral("?]");

  return ToNewUnicode(resultString);
}

nsString nsEudoraStringBundle::FormatString(PRInt32 stringID, ...)
{
  // Yeah, I know.  This causes an extra string buffer allocation, but there's no guarantee
  // that nsString's free and nsTextFormatter::smprintf_free deallocate memory the same way.
  nsAutoString format;
  GetStringByID(stringID, format);

  va_list args;
  va_start(args, stringID);

  PRUnichar *pText = nsTextFormatter::vsmprintf(format.get(), args);
  va_end(args);

  nsString result(pText);
  nsTextFormatter::smprintf_free(pText);
  return result;
}

void nsEudoraStringBundle::Cleanup( void)
{
  if (m_pBundle)
    m_pBundle->Release();
  m_pBundle = nsnull;
}
