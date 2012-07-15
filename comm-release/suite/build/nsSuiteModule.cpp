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
 *   Joe Hewitt <hewitt@netscape.com> (Original Author)
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

#include "mozilla/ModuleUtils.h"
#include "nsSuiteDirectoryProvider.h"
#include "nsProfileMigrator.h"
#include "nsThunderbirdProfileMigrator.h"
#include "nsNetCID.h"
#include "nsRDFCID.h"
#include "nsFeedSniffer.h"

#if defined(XP_WIN)
#include "nsWindowsShellService.h"
#elif defined(XP_MACOSX)
#include "nsMacShellService.h"
#elif defined(MOZ_WIDGET_GTK2)
#include "nsGNOMEShellService.h"
#endif

/////////////////////////////////////////////////////////////////////////////

#if defined(XP_WIN)
NS_GENERIC_FACTORY_CONSTRUCTOR_INIT(nsWindowsShellService, Init)
#elif defined(XP_MACOSX)
NS_GENERIC_FACTORY_CONSTRUCTOR(nsMacShellService)
#elif defined(MOZ_WIDGET_GTK2)
NS_GENERIC_FACTORY_CONSTRUCTOR_INIT(nsGNOMEShellService, Init)
#endif
NS_GENERIC_FACTORY_CONSTRUCTOR(nsSuiteDirectoryProvider)
NS_GENERIC_FACTORY_CONSTRUCTOR(nsProfileMigrator)
NS_GENERIC_FACTORY_CONSTRUCTOR(nsThunderbirdProfileMigrator)
NS_GENERIC_FACTORY_CONSTRUCTOR(nsFeedSniffer)

#if defined(NS_SUITEWININTEGRATION_CID)
NS_DEFINE_NAMED_CID(NS_SUITEWININTEGRATION_CID);
#elif defined(NS_SUITEMACINTEGRATION_CID)
NS_DEFINE_NAMED_CID(NS_SUITEMACINTEGRATION_CID);
#elif defined(NS_SUITEGNOMEINTEGRATION_CID)
NS_DEFINE_NAMED_CID(NS_SUITEGNOMEINTEGRATION_CID);
#endif
NS_DEFINE_NAMED_CID(NS_SUITEDIRECTORYPROVIDER_CID);
NS_DEFINE_NAMED_CID(NS_SUITEPROFILEMIGRATOR_CID);
NS_DEFINE_NAMED_CID(NS_THUNDERBIRDPROFILEMIGRATOR_CID);
NS_DEFINE_NAMED_CID(NS_FEEDSNIFFER_CID);

/////////////////////////////////////////////////////////////////////////////

static const mozilla::Module::CIDEntry kSuiteCIDs[] = {
#if defined(NS_SUITEWININTEGRATION_CID)
  { &kNS_SUITEWININTEGRATION_CID, false, NULL, nsWindowsShellServiceConstructor },
#elif defined(NS_SUITEMACINTEGRATION_CID)
  { &kNS_SUITEMACINTEGRATION_CID, false, NULL, nsMacShellServiceConstructor },
#elif defined(NS_SUITEGNOMEINTEGRATION_CID)
  { &kNS_SUITEGNOMEINTEGRATION_CID, false, NULL, nsGNOMEShellServiceConstructor },
#endif
  { &kNS_SUITEDIRECTORYPROVIDER_CID, false, NULL, nsSuiteDirectoryProviderConstructor },
  { &kNS_SUITEPROFILEMIGRATOR_CID, false, NULL, nsProfileMigratorConstructor },
  { &kNS_THUNDERBIRDPROFILEMIGRATOR_CID, false, NULL, nsThunderbirdProfileMigratorConstructor },
  { &kNS_FEEDSNIFFER_CID, false, NULL, nsFeedSnifferConstructor },
  { NULL }
};

static const mozilla::Module::ContractIDEntry kSuiteContracts[] = {
#if defined(NS_SUITEWININTEGRATION_CID)
  { NS_SUITESHELLSERVICE_CONTRACTID, &kNS_SUITEWININTEGRATION_CID },
  { NS_SUITEFEEDSERVICE_CONTRACTID, &kNS_SUITEWININTEGRATION_CID },
#elif defined(NS_SUITEMACINTEGRATION_CID)
  { NS_SUITEFEEDSERVICE_CONTRACTID, &kNS_SUITEMACINTEGRATION_CID },
#elif defined(NS_SUITEGNOMEINTEGRATION_CID)
  { NS_SUITEFEEDSERVICE_CONTRACTID, &kNS_SUITEGNOMEINTEGRATION_CID },
#endif
  { NS_SUITEDIRECTORYPROVIDER_CONTRACTID, &kNS_SUITEDIRECTORYPROVIDER_CID },
  { NS_PROFILEMIGRATOR_CONTRACTID, &kNS_SUITEPROFILEMIGRATOR_CID },
  { NS_SUITEPROFILEMIGRATOR_CONTRACTID_PREFIX "thunderbird", &kNS_THUNDERBIRDPROFILEMIGRATOR_CID },
  { NS_FEEDSNIFFER_CONTRACTID, &kNS_FEEDSNIFFER_CID },
  { NULL }
};

static const mozilla::Module::CategoryEntry kSuiteCategories[] = {
  { XPCOM_DIRECTORY_PROVIDER_CATEGORY, "suite-directory-provider", NS_SUITEDIRECTORYPROVIDER_CONTRACTID },
  { NS_CONTENT_SNIFFER_CATEGORY, "Feed Sniffer", NS_FEEDSNIFFER_CONTRACTID },
  { NULL }
};

static const mozilla::Module kSuiteModule = {
  mozilla::Module::kVersion,
  kSuiteCIDs,
  kSuiteContracts,
  kSuiteCategories
};

NSMODULE_DEFN(SuiteModule) = &kSuiteModule;
