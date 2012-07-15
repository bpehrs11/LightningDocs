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
 *   Philipp Kewisch <mozilla@kewis.ch>
 *   Daniel Boelzle <mozilla@boelzle.org>
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

#include "calRecurrenceDate.h"

#include "calDateTime.h"
#include "calPeriod.h"
#include "calIItemBase.h"
#include "calIEvent.h"
#include "calIErrors.h"
#include "nsServiceManagerUtils.h"

#include "calICSService.h"

#include "nsIClassInfoImpl.h"

#include "calBaseCID.h"

extern "C" {
    #include "ical.h"
}

NS_IMPL_CLASSINFO(calRecurrenceDate, NULL, 0, CAL_RECURRENCEDATE_CID)
NS_IMPL_ISUPPORTS2_CI(calRecurrenceDate, calIRecurrenceItem, calIRecurrenceDate)

calRecurrenceDate::calRecurrenceDate()
    : mImmutable(false),
      mIsNegative(false)
{
}

NS_IMETHODIMP
calRecurrenceDate::GetIsMutable(bool *aResult)
{
    NS_ENSURE_ARG_POINTER(aResult);

    *aResult = !mImmutable;
    return NS_OK;
}

NS_IMETHODIMP
calRecurrenceDate::MakeImmutable()
{
    if (mImmutable)
        return NS_ERROR_FAILURE; // XXX another error code

    mImmutable = true;
    return NS_OK;
}

NS_IMETHODIMP
calRecurrenceDate::Clone(calIRecurrenceItem **_retval)
{
    calRecurrenceDate *crd = new calRecurrenceDate;
    if (!crd)
        return NS_ERROR_OUT_OF_MEMORY;

    crd->mIsNegative = mIsNegative;
    if (mDate)
        mDate->Clone(getter_AddRefs(crd->mDate));
    else
        crd->mDate = nsnull;

    NS_ADDREF(*_retval = crd);
    return NS_OK;
}

/* attribute boolean isNegative; */
NS_IMETHODIMP
calRecurrenceDate::GetIsNegative(bool *_retval)
{
    NS_ENSURE_ARG_POINTER(_retval);

    *_retval = mIsNegative;
    return NS_OK;
}

NS_IMETHODIMP
calRecurrenceDate::SetIsNegative(bool aIsNegative)
{
    if (mImmutable)
        return NS_ERROR_FAILURE; // XXX CAL_ERROR_ITEM_IS_IMMUTABLE

    mIsNegative = aIsNegative;
    return NS_OK;
}

/* readonly attribute boolean isFinite; */
NS_IMETHODIMP
calRecurrenceDate::GetIsFinite(bool *_retval)
{
    NS_ENSURE_ARG_POINTER(_retval);
    *_retval = true;
    return NS_OK;
}

NS_IMETHODIMP
calRecurrenceDate::GetDate(calIDateTime **aDate)
{
    NS_ENSURE_ARG_POINTER(aDate);

    NS_IF_ADDREF(*aDate = mDate);
    return NS_OK;
}

NS_IMETHODIMP
calRecurrenceDate::SetDate(calIDateTime *aDate)
{
    NS_ENSURE_ARG_POINTER(aDate);

    mDate = aDate;
    return NS_OK;
}

NS_IMETHODIMP
calRecurrenceDate::GetNextOccurrence(calIDateTime *aStartTime,
                                     calIDateTime *aOccurrenceTime,
                                     calIDateTime **_retval)
{
    NS_ENSURE_ARG_POINTER(aStartTime);
    NS_ENSURE_ARG_POINTER(aOccurrenceTime);
    NS_ENSURE_ARG_POINTER(_retval);

    if (mDate) {
        PRInt32 result;
        if (NS_SUCCEEDED(mDate->Compare(aStartTime, &result)) && result > 0) {
            NS_ADDREF (*_retval = mDate);
            return NS_OK;
        }
    }

    *_retval = nsnull;
    return NS_OK;
}

NS_IMETHODIMP
calRecurrenceDate::GetOccurrences(calIDateTime *aStartTime,
                                  calIDateTime *aRangeStart,
                                  calIDateTime *aRangeEnd,
                                  PRUint32 aMaxCount,
                                  PRUint32 *aCount, calIDateTime ***aDates)
{
    NS_ENSURE_ARG_POINTER(aStartTime);
    NS_ENSURE_ARG_POINTER(aRangeStart);

    PRInt32 r1, r2;

    if (mDate) {
        if (NS_SUCCEEDED(mDate->Compare(aRangeStart, &r1)) && r1 >= 0 &&
            (!aRangeEnd || (NS_SUCCEEDED(mDate->Compare(aRangeEnd, &r2)) && r2 < 0)))
        {
            calIDateTime **dates = (calIDateTime **) nsMemory::Alloc(sizeof(calIDateTime*));
            NS_ADDREF (dates[0] = mDate);
            *aDates = dates;
            *aCount = 1;
            return NS_OK;
        }
    }

    *aDates = nsnull;
    *aCount = 0;
    return NS_OK;
}

/**
 ** ical property getting/setting
 **/
NS_IMETHODIMP
calRecurrenceDate::GetIcalProperty(calIIcalProperty **aProp)
{
    NS_ENSURE_ARG_POINTER(aProp);
    if (!mDate)
        return NS_ERROR_FAILURE;

    nsresult rc = cal::getICSService()->CreateIcalProperty(
        (mIsNegative ? nsDependentCString("EXDATE") : nsDependentCString("RDATE")), aProp);
    if (NS_FAILED(rc))
        return rc;

    return (*aProp)->SetValueAsDatetime(mDate);
}

NS_IMETHODIMP
calRecurrenceDate::SetIcalProperty(calIIcalProperty *aProp)
{
    NS_ENSURE_ARG_POINTER(aProp);

    nsCAutoString name;
    nsresult rc = aProp->GetPropertyName(name);
    if (NS_FAILED(rc))
        return rc;
    if (name.EqualsLiteral("RDATE")) {
        mIsNegative = false;
        icalvalue * const value = icalproperty_get_value(aProp->GetIcalProperty());
        if (icalvalue_isa(value) == ICAL_PERIOD_VALUE) {
            icalperiodtype const period = icalvalue_get_period(value);
            // take only period's start date and skip end date, but continue parsing;
            // open bug 489747:
            mDate = new calDateTime(&period.start, nsnull /* detect timezone */);
            return NS_OK;
        }
    } else if (name.EqualsLiteral("EXDATE"))
        mIsNegative = true;
    else
        return NS_ERROR_INVALID_ARG;

    return aProp->GetValueAsDatetime(getter_AddRefs(mDate));
}
