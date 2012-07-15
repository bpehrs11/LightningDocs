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
 *   Seth Spitzer <sspitzer@netscape.com>
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

#ifndef _nsAbAddressCollector_H_
#define _nsAbAddressCollector_H_

#include "nsIAbAddressCollector.h"
#include "nsCOMPtr.h"
#include "nsIAbDirectory.h"
#include "nsIAbCard.h"
#include "nsIObserver.h"
#include "nsStringGlue.h"

class nsIPrefBranch;

class nsAbAddressCollector : public nsIAbAddressCollector,
                             public nsIObserver
{
public:
  nsAbAddressCollector();
  virtual ~nsAbAddressCollector();

  NS_DECL_ISUPPORTS
  NS_DECL_NSIABADDRESSCOLLECTOR
  NS_DECL_NSIOBSERVER

  nsresult Init();

private:
  already_AddRefed<nsIAbCard> GetCardFromProperty(const char *aName,
                                                  const nsACString &aValue,
                                                  nsIAbDirectory **aDirectory);
  void AutoCollectScreenName(nsIAbCard *aCard, const nsACString &aEmail);
  bool SetNamesForCard(nsIAbCard *aSenderCard, const nsACString &aFullName);
  void SplitFullName(const nsCString &aFullName, nsCString &aFirstName,
                     nsCString &aLastName);
  void SetUpAbFromPrefs(nsIPrefBranch *aPrefBranch);
  nsCOMPtr <nsIAbDirectory> mDirectory;
  nsCString mABURI;
};

#endif  // _nsAbAddressCollector_H_

