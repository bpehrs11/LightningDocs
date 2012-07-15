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
 * The Original Code is Oracle Corporation code.
 *
 * The Initial Developer of the Original Code is
 *  Oracle Corporation
 * Portions created by the Initial Developer are Copyright (C) 2004
 * the Initial Developer. All Rights Reserved.
 *
 * Contributor(s):
 *   Vladimir Vukicevic <vladimir.vukicevic@oracle.com>
 *   Daniel Boelzle <mozilla@boelzle.org>
 *   Philipp Kewisch <mozilla@kewis.ch>
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
#include "calDateTime.h"
#include "calDuration.h"
#include "calPeriod.h"
#include "calICSService.h"
#include "calRecurrenceRule.h"
#include "calRecurrenceDate.h"
#include "calRecurrenceDateSet.h"

#include "calBaseCID.h"

NS_GENERIC_FACTORY_CONSTRUCTOR(calDateTime)
NS_DEFINE_NAMED_CID(CAL_DATETIME_CID);

NS_GENERIC_FACTORY_CONSTRUCTOR(calDuration)
NS_DEFINE_NAMED_CID(CAL_DURATION_CID);

NS_GENERIC_FACTORY_CONSTRUCTOR(calICSService)
NS_DEFINE_NAMED_CID(CAL_ICSSERVICE_CID);

NS_GENERIC_FACTORY_CONSTRUCTOR(calPeriod)
NS_DEFINE_NAMED_CID(CAL_PERIOD_CID);

NS_GENERIC_FACTORY_CONSTRUCTOR(calRecurrenceDate)
NS_DEFINE_NAMED_CID(CAL_RECURRENCEDATE_CID);

NS_GENERIC_FACTORY_CONSTRUCTOR(calRecurrenceDateSet)
NS_DEFINE_NAMED_CID(CAL_RECURRENCEDATESET_CID);

NS_GENERIC_FACTORY_CONSTRUCTOR(calRecurrenceRule)
NS_DEFINE_NAMED_CID(CAL_RECURRENCERULE_CID);


const mozilla::Module::CIDEntry kCalBaseCIDs[] = {
    { &kCAL_DATETIME_CID, false, NULL, calDateTimeConstructor },
    { &kCAL_DURATION_CID, false, NULL, calDurationConstructor },
    { &kCAL_ICSSERVICE_CID, true, NULL, calICSServiceConstructor },
    { &kCAL_PERIOD_CID, false, NULL, calPeriodConstructor },
    { &kCAL_RECURRENCERULE_CID, false, NULL, calRecurrenceRuleConstructor },
    { &kCAL_RECURRENCEDATE_CID, false, NULL, calRecurrenceDateConstructor },
    { &kCAL_RECURRENCEDATESET_CID, false, NULL, calRecurrenceDateSetConstructor },
    { NULL }
};

const mozilla::Module::ContractIDEntry kCalBaseContracts[] = {
    { CAL_DATETIME_CONTRACTID, &kCAL_DATETIME_CID },
    { CAL_DURATION_CONTRACTID, &kCAL_DURATION_CID },
    { CAL_ICSSERVICE_CONTRACTID, &kCAL_ICSSERVICE_CID },
    { CAL_PERIOD_CONTRACTID, &kCAL_PERIOD_CID },
    { CAL_RECURRENCERULE_CONTRACTID, &kCAL_RECURRENCERULE_CID },
    { CAL_RECURRENCEDATE_CONTRACTID, &kCAL_RECURRENCEDATE_CID },
    { CAL_RECURRENCEDATESET_CONTRACTID, &kCAL_RECURRENCEDATESET_CID },
    { NULL }
};

static nsresult
nsInitBaseModule()
{
    // This needs to be done once in the application, we want to make
    // sure that new parameters are not thrown away
    ical_set_unknown_token_handling_setting(ICAL_ASSUME_IANA_TOKEN);
    return NS_OK;
}

static const mozilla::Module kCalBaseModule = {
    mozilla::Module::kVersion,
    kCalBaseCIDs,
    kCalBaseContracts,
    NULL,
    NULL,
    nsInitBaseModule
};

NSMODULE_DEFN(calBaseModule) = &kCalBaseModule;
