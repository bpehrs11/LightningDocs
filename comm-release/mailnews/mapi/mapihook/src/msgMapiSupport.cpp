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
 * The Original Code is Mozilla
 *
 * The Initial Developer of the Original Code is
 * Netscape Communications Corp.
 * Portions created by the Initial Developer are Copyright (C) 2001
 * the Initial Developer. All Rights Reserved.
 *
 * Contributor(s): Krishna Mohan Khandrika (kkhandrika@netscape.com)
 *                 Rajiv Dayal <rdayal@netscape.com>
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
#include "nsCOMPtr.h"
#include "objbase.h"
#include "nsISupports.h"

#include "mozilla/ModuleUtils.h"
#include "mozilla/Services.h"
#include "nsIObserverService.h"
#include "nsIAppStartupNotifier.h"
#include "nsIServiceManager.h"
#include "nsIComponentManager.h"
#include "nsICategoryManager.h"
#include "Registry.h"
#include "msgMapiSupport.h"

#include "msgMapiImp.h"

/** Implementation of the nsIMapiSupport interface.
 *  Use standard implementation of nsISupports stuff.
 */

NS_IMPL_THREADSAFE_ISUPPORTS2(nsMapiSupport, nsIMapiSupport, nsIObserver)

NS_IMETHODIMP
nsMapiSupport::Observe(nsISupports *aSubject, const char *aTopic, const PRUnichar *aData)
{
    nsresult rv = NS_OK ;

    if (!strcmp(aTopic, "profile-after-change"))
        return InitializeMAPISupport();

    if (!strcmp(aTopic, NS_XPCOM_SHUTDOWN_OBSERVER_ID))
        return ShutdownMAPISupport();

    nsCOMPtr<nsIObserverService> observerService =
      mozilla::services::GetObserverService();
    NS_ENSURE_TRUE(observerService, NS_ERROR_UNEXPECTED);
 
    rv = observerService->AddObserver(this,"profile-after-change", false);
    if (NS_FAILED(rv)) return rv;

    rv = observerService->AddObserver(this, NS_XPCOM_SHUTDOWN_OBSERVER_ID, false);
    if (NS_FAILED(rv))  return rv;

    return rv;
}


nsMapiSupport::nsMapiSupport()
: m_dwRegister(0),
  m_nsMapiFactory(nsnull)
{
}

nsMapiSupport::~nsMapiSupport()
{
}

NS_IMETHODIMP
nsMapiSupport::InitializeMAPISupport()
{
    ::OleInitialize(nsnull) ;

    if (m_nsMapiFactory == nsnull)    // No Registering if already done.  Sanity Check!!
    {
        m_nsMapiFactory = new CMapiFactory();

        if (m_nsMapiFactory != nsnull)
        {
            HRESULT hr = ::CoRegisterClassObject(CLSID_CMapiImp, \
                                                 m_nsMapiFactory, \
                                                 CLSCTX_LOCAL_SERVER, \
                                                 REGCLS_MULTIPLEUSE, \
                                                 &m_dwRegister);

            if (FAILED(hr))
            {
                m_nsMapiFactory->Release() ;
                m_nsMapiFactory = nsnull;
                return NS_ERROR_FAILURE;
            }
        }
    }

    return NS_OK;
}

NS_IMETHODIMP
nsMapiSupport::ShutdownMAPISupport()
{
    if (m_dwRegister != 0)
        ::CoRevokeClassObject(m_dwRegister);

    if (m_nsMapiFactory != nsnull)
    {
        m_nsMapiFactory->Release();
        m_nsMapiFactory = nsnull;
    }

    ::OleUninitialize();

    return NS_OK ;
}

NS_IMETHODIMP
nsMapiSupport::RegisterServer()
{
  // TODO: Figure out what kind of error propogation to pass back
  ::RegisterServer(CLSID_CMapiImp, "Mozilla MAPI", "MozillaMapi", "MozillaMapi.1");
  return NS_OK;
}

NS_IMETHODIMP
nsMapiSupport::UnRegisterServer()
{
  // TODO: Figure out what kind of error propogation to pass back
  ::UnregisterServer(CLSID_CMapiImp, "MozillaMapi", "MozillaMapi.1");
  return NS_OK;
}

NS_DEFINE_NAMED_CID(NS_IMAPISUPPORT_CID);

NS_GENERIC_FACTORY_CONSTRUCTOR(nsMapiSupport)

static const mozilla::Module::CategoryEntry kMAPICategories[] = {
  { APPSTARTUP_CATEGORY, "Mapi Support", "service," NS_IMAPISUPPORT_CONTRACTID, },
  { NULL }
};

const mozilla::Module::CIDEntry kMAPICIDs[] = {
  { &kNS_IMAPISUPPORT_CID, false, NULL, nsMapiSupportConstructor },
  { NULL }
};

const mozilla::Module::ContractIDEntry kMAPIContracts[] = {
  { NS_IMAPISUPPORT_CONTRACTID, &kNS_IMAPISUPPORT_CID },
  { NULL }
};

static const mozilla::Module kMAPIModule = {
    mozilla::Module::kVersion,
    kMAPICIDs,
    kMAPIContracts,
    kMAPICategories
};

NSMODULE_DEFN(msgMapiModule) = &kMAPIModule;


