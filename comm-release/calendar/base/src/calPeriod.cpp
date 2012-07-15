/* -*- Mode: C++; tab-width: 20; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
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
 * The Original Code is mozilla calendar code.
 *
 * The Initial Developer of the Original Code is
 *   Michiel van Leeuwen <mvl@exedo.nl>
 * Portions created by the Initial Developer are Copyright (C) 2006
 * the Initial Developer. All Rights Reserved.
 *
 * Contributor(s):
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

#include "calPeriod.h"
#include "calBaseCID.h"

#include "nsIClassInfoImpl.h"

#include "calUtils.h"

NS_IMPL_CLASSINFO(calPeriod, NULL, 0, CAL_PERIOD_CID)
NS_IMPL_ISUPPORTS1_CI(calPeriod, calIPeriod)

calPeriod::calPeriod()
    : mImmutable(false)
{
}

calPeriod::calPeriod(const calPeriod& cpt)
    : mImmutable(false)
{
    if (cpt.mStart)
        cpt.mStart->Clone(getter_AddRefs(mStart));
    if (cpt.mEnd)
        cpt.mEnd->Clone(getter_AddRefs(mEnd));
}

calPeriod::calPeriod(struct icalperiodtype const* aPeriodPtr)
    : mImmutable(false)
{
    FromIcalPeriod(aPeriodPtr);
}

NS_IMETHODIMP
calPeriod::GetIsMutable(bool *aResult)
{
    NS_ENSURE_ARG_POINTER(aResult);

    *aResult = !mImmutable;
    return NS_OK;
}

NS_IMETHODIMP
calPeriod::MakeImmutable()
{
    mImmutable = true;
    return NS_OK;
}

NS_IMETHODIMP
calPeriod::Clone(calIPeriod **aResult)
{
    NS_ENSURE_ARG_POINTER(aResult);
    calPeriod *cpt = new calPeriod(*this);
    if (!cpt)
        return NS_ERROR_OUT_OF_MEMORY;

    NS_ADDREF(*aResult = cpt);
    return NS_OK;
}


NS_IMETHODIMP calPeriod::GetStart(calIDateTime **_retval)
{
    NS_ENSURE_ARG_POINTER(_retval);
    *_retval = mStart;
    NS_IF_ADDREF(*_retval);
    return NS_OK;
}
NS_IMETHODIMP calPeriod::SetStart(calIDateTime *aValue)
{
    NS_ENSURE_ARG_POINTER(aValue);
    if (mImmutable)
        return NS_ERROR_OBJECT_IS_IMMUTABLE;
    // rfc2445 says that periods are always in utc. libical ignore that,
    // so we need the conversion here.
    nsresult const rv = aValue->GetInTimezone(cal::UTC(), getter_AddRefs(mStart));
    NS_ENSURE_SUCCESS(rv, rv);
    return mStart->MakeImmutable();
}

NS_IMETHODIMP calPeriod::GetEnd(calIDateTime **_retval)
{
    NS_ENSURE_ARG_POINTER(_retval);
    *_retval = mEnd;
    NS_IF_ADDREF(*_retval);
    return NS_OK;
}
NS_IMETHODIMP calPeriod::SetEnd(calIDateTime *aValue)
{
    NS_ENSURE_ARG_POINTER(aValue);
    if (mImmutable)
        return NS_ERROR_OBJECT_IS_IMMUTABLE;
    nsresult const rv = aValue->GetInTimezone(cal::UTC(), getter_AddRefs(mEnd));
    NS_ENSURE_SUCCESS(rv, rv);
    return mEnd->MakeImmutable();
}

NS_IMETHODIMP calPeriod::GetDuration(calIDuration **_retval)
{
    NS_ENSURE_ARG_POINTER(_retval);
    if (!mStart || !mEnd)
        return NS_ERROR_UNEXPECTED;
    return mEnd->SubtractDate(mStart, _retval);
}

NS_IMETHODIMP
calPeriod::ToString(nsACString& aResult)
{
    return GetIcalString(aResult);
}

NS_IMETHODIMP_(void)
calPeriod::ToIcalPeriod(struct icalperiodtype *icalp)
{
    // makes no sense to create a duration without bath a start and end
    if (!mStart || !mEnd) {
        *icalp = icalperiodtype_null_period();
        return;
    }
    
    mStart->ToIcalTime(&icalp->start);
    mEnd->ToIcalTime(&icalp->end);
}

void
calPeriod::FromIcalPeriod(struct icalperiodtype const* icalp)
{
    mStart = new calDateTime(&(icalp->start), nsnull);
    mStart->MakeImmutable();
    mEnd = new calDateTime(&(icalp->end), nsnull);
    mEnd->MakeImmutable();
    return;
}

NS_IMETHODIMP
calPeriod::GetIcalString(nsACString& aResult)
{
    struct icalperiodtype ip;
    ToIcalPeriod(&ip);
    
    // note that ics is owned by libical, so we don't need to free
    const char *ics = icalperiodtype_as_ical_string(ip);
    
    if (ics) {
        aResult.Assign(ics);
        return NS_OK;
    }

    return NS_ERROR_OUT_OF_MEMORY;
}

NS_IMETHODIMP
calPeriod::SetIcalString(const nsACString& aIcalString)
{
    if (mImmutable)
        return NS_ERROR_OBJECT_IS_IMMUTABLE;
    struct icalperiodtype ip;
    ip = icalperiodtype_from_string(PromiseFlatCString(aIcalString).get());
    //XXX Shortcut. Assumes nobody tried to overrule our impl. of calIDateTime
    mStart = new calDateTime(&ip.start, nsnull);
    mEnd = new calDateTime(&ip.end, nsnull);
    return NS_OK;
}
