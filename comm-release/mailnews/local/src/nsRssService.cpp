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
 * The Original Code is mozilla mailnews.
 *
 * The Initial Developer of the Original Code is
 * Seth Spitzer <sspitzer@mozilla.org>.
 * Portions created by the Initial Developer are Copyright (C) 2004
 * the Initial Developer. All Rights Reserved.
 *
 * Contributor(s):
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

#include "nsRssService.h"
#include "nsIRssIncomingServer.h"
#include "nsCOMPtr.h"
#include "nsILocalFile.h"
#include "nsMailDirServiceDefs.h"
#include "nsIProperties.h"
#include "nsServiceManagerUtils.h"

nsRssService::nsRssService()
{
}

nsRssService::~nsRssService()
{
}

NS_IMPL_ISUPPORTS2(nsRssService,
                   nsIRssService,
                   nsIMsgProtocolInfo)
                   
NS_IMETHODIMP nsRssService::GetDefaultLocalPath(nsILocalFile * *aDefaultLocalPath)
{
    NS_ENSURE_ARG_POINTER(aDefaultLocalPath);
    *aDefaultLocalPath = nsnull;
    
    nsCOMPtr<nsILocalFile> localFile;
    nsCOMPtr<nsIProperties> dirService(do_GetService("@mozilla.org/file/directory_service;1"));
    if (!dirService) return NS_ERROR_FAILURE;
    dirService->Get(NS_APP_MAIL_50_DIR, NS_GET_IID(nsILocalFile), getter_AddRefs(localFile));
    if (!localFile) return NS_ERROR_FAILURE;

    bool exists;
    nsresult rv = localFile->Exists(&exists);
    if (NS_SUCCEEDED(rv) && !exists)
        rv = localFile->Create(nsIFile::DIRECTORY_TYPE, 0775);
    if (NS_FAILED(rv)) return rv;
   
    NS_IF_ADDREF(*aDefaultLocalPath = localFile);
    return NS_OK;

}

NS_IMETHODIMP nsRssService::SetDefaultLocalPath(nsILocalFile * aDefaultLocalPath)
{
    return NS_ERROR_NOT_IMPLEMENTED;
}

NS_IMETHODIMP nsRssService::GetServerIID(nsIID * *aServerIID)
{
    return NS_ERROR_NOT_IMPLEMENTED;
}

NS_IMETHODIMP nsRssService::GetRequiresUsername(bool *aRequiresUsername)
{
    return NS_ERROR_NOT_IMPLEMENTED;
}

NS_IMETHODIMP nsRssService::GetPreflightPrettyNameWithEmailAddress(bool *aPreflightPrettyNameWithEmailAddress)
{
    return NS_ERROR_NOT_IMPLEMENTED;
}

NS_IMETHODIMP nsRssService::GetCanDelete(bool *aCanDelete)
{
    NS_ENSURE_ARG_POINTER(aCanDelete);
    *aCanDelete = true;
    return NS_OK;
}

NS_IMETHODIMP nsRssService::GetCanLoginAtStartUp(bool *aCanLoginAtStartUp)
{
    NS_ENSURE_ARG_POINTER(aCanLoginAtStartUp);
    *aCanLoginAtStartUp = true;
    return NS_OK;
}

NS_IMETHODIMP nsRssService::GetCanDuplicate(bool *aCanDuplicate)
{
    NS_ENSURE_ARG_POINTER(aCanDuplicate);
    *aCanDuplicate = true;
    return NS_OK;
}

NS_IMETHODIMP nsRssService::GetDefaultServerPort(bool isSecure, PRInt32 *_retval)
{
    *_retval = -1;
    return NS_OK;
}

NS_IMETHODIMP nsRssService::GetCanGetMessages(bool *aCanGetMessages)
{
    NS_ENSURE_ARG_POINTER(aCanGetMessages);
    *aCanGetMessages = true;
    return NS_OK;
}

NS_IMETHODIMP nsRssService::GetCanGetIncomingMessages(bool *aCanGetIncomingMessages)
{
    NS_ENSURE_ARG_POINTER(aCanGetIncomingMessages);
    *aCanGetIncomingMessages = true;
    return NS_OK;
}

NS_IMETHODIMP nsRssService::GetDefaultDoBiff(bool *aDefaultDoBiff)
{
    NS_ENSURE_ARG_POINTER(aDefaultDoBiff);
    // by default, do biff for RSS feeds
    *aDefaultDoBiff = true;    
    return NS_OK;
}

NS_IMETHODIMP nsRssService::GetShowComposeMsgLink(bool *aShowComposeMsgLink)
{
    NS_ENSURE_ARG_POINTER(aShowComposeMsgLink);
    *aShowComposeMsgLink = false;    
    return NS_OK;
}
