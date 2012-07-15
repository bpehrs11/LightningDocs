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
 * Portions created by the Initial Developer are Copyright (C) 1998
 * the Initial Developer. All Rights Reserved.
 *
 * Contributor(s):
 *   Seth Spitzer <sspitzer@netscape.com>
 *   David Bienvenu <bienvenu@nventure.com>
 *
 * Alternatively, the contents of this file may be used under the terms of
 * either the GNU General Public License Version 2 or later (the "GPL"), or
 * the GNU Lesser General Public License Version 2.1 or later (the "LGPL"),
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

#include "nsNoneService.h"
#include "nsINoIncomingServer.h"
#include "nsINoneService.h"
#include "nsIMsgProtocolInfo.h"

#include "nsMsgLocalCID.h"
#include "nsMsgBaseCID.h"
#include "nsILocalFile.h"
#include "nsCOMPtr.h"
#include "nsMsgUtils.h"

#include "nsIDirectoryService.h"
#include "nsMailDirServiceDefs.h"

#define PREF_MAIL_ROOT_NONE "mail.root.none"        // old - for backward compatibility only
#define PREF_MAIL_ROOT_NONE_REL "mail.root.none-rel"

nsNoneService::nsNoneService()
{
}

nsNoneService::~nsNoneService()
{}

NS_IMPL_ISUPPORTS2(nsNoneService, nsINoneService, nsIMsgProtocolInfo)

NS_IMETHODIMP
nsNoneService::SetDefaultLocalPath(nsILocalFile *aPath)
{
    NS_ENSURE_ARG(aPath);
    return NS_SetPersistentFile(PREF_MAIL_ROOT_NONE_REL, PREF_MAIL_ROOT_NONE, aPath);
}     

NS_IMETHODIMP
nsNoneService::GetDefaultLocalPath(nsILocalFile ** aResult)
{
    NS_ENSURE_ARG_POINTER(aResult);
    *aResult = nsnull;
    
    bool havePref;
    nsCOMPtr<nsILocalFile> localFile;    
    nsresult rv = NS_GetPersistentFile(PREF_MAIL_ROOT_NONE_REL,
                              PREF_MAIL_ROOT_NONE,
                              NS_APP_MAIL_50_DIR,
                              havePref,
                              getter_AddRefs(localFile));
    if (NS_FAILED(rv)) return rv;
    
    bool exists;
    rv = localFile->Exists(&exists);
    if (NS_SUCCEEDED(rv) && !exists)
        rv = localFile->Create(nsIFile::DIRECTORY_TYPE, 0775);
    if (NS_FAILED(rv)) return rv;
    
    if (!havePref || !exists) 
    {
        rv = NS_SetPersistentFile(PREF_MAIL_ROOT_NONE_REL, PREF_MAIL_ROOT_NONE, localFile);
        NS_ASSERTION(NS_SUCCEEDED(rv), "Failed to set root dir pref.");
    }
        
    NS_IF_ADDREF(*aResult = localFile);
    return NS_OK;

}
    

NS_IMETHODIMP
nsNoneService::GetServerIID(nsIID* *aServerIID)
{
    *aServerIID = new nsIID(NS_GET_IID(nsINoIncomingServer));
    return NS_OK;
}

NS_IMETHODIMP
nsNoneService::GetRequiresUsername(bool *aRequiresUsername)
{
  NS_ENSURE_ARG_POINTER(aRequiresUsername);
  *aRequiresUsername = true;
  return NS_OK;
}

NS_IMETHODIMP
nsNoneService::GetPreflightPrettyNameWithEmailAddress(bool *aPreflightPrettyNameWithEmailAddress)
{
  NS_ENSURE_ARG_POINTER(aPreflightPrettyNameWithEmailAddress);
  *aPreflightPrettyNameWithEmailAddress = true;
  return NS_OK;
}

NS_IMETHODIMP
nsNoneService::GetCanLoginAtStartUp(bool *aCanLoginAtStartUp)
{
  NS_ENSURE_ARG_POINTER(aCanLoginAtStartUp);
  *aCanLoginAtStartUp = false;
  return NS_OK;
}

NS_IMETHODIMP
nsNoneService::GetCanDelete(bool *aCanDelete)
{
  NS_ENSURE_ARG_POINTER(aCanDelete);
  *aCanDelete = false;
  return NS_OK;
}  

NS_IMETHODIMP
nsNoneService::GetCanDuplicate(bool *aCanDuplicate)
{
  NS_ENSURE_ARG_POINTER(aCanDuplicate);
  *aCanDuplicate = false;
  return NS_OK;
}  

NS_IMETHODIMP
nsNoneService::GetCanGetMessages(bool *aCanGetMessages)
{
    NS_ENSURE_ARG_POINTER(aCanGetMessages);
    *aCanGetMessages = false;
    return NS_OK;
}  

NS_IMETHODIMP
nsNoneService::GetCanGetIncomingMessages(bool *aCanGetIncomingMessages)
{
    NS_ENSURE_ARG_POINTER(aCanGetIncomingMessages);
    *aCanGetIncomingMessages = false;
    return NS_OK;
} 

NS_IMETHODIMP 
nsNoneService::GetDefaultDoBiff(bool *aDoBiff)
{
    NS_ENSURE_ARG_POINTER(aDoBiff);
    // by default, don't do biff for "none" servers
    *aDoBiff = false;    
    return NS_OK;
}

NS_IMETHODIMP
nsNoneService::GetDefaultServerPort(bool isSecure, PRInt32 *aDefaultPort)
{
    NS_ENSURE_ARG_POINTER(aDefaultPort);
    *aDefaultPort = -1;
    return NS_OK;
}

NS_IMETHODIMP 
nsNoneService::GetShowComposeMsgLink(bool *showComposeMsgLink)
{
    NS_ENSURE_ARG_POINTER(showComposeMsgLink);
    *showComposeMsgLink = false;    
    return NS_OK;
}
