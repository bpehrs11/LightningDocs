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
 * Portions created by the Initial Developer are Copyright (C) 2001
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

#ifndef _nsAbView_H_
#define _nsAbView_H_

#include "nsISupports.h"
#include "nsStringGlue.h"
#include "nsIAbView.h"
#include "nsITreeView.h"
#include "nsITreeBoxObject.h"
#include "nsITreeSelection.h"
#include "nsVoidArray.h"
#include "nsIAbDirectory.h"
#include "nsIAtom.h"
#include "nsICollation.h"
#include "nsIAbListener.h"
#include "nsIObserver.h"
#include "nsServiceManagerUtils.h"
#include "nsComponentManagerUtils.h"
#include "nsMemory.h"
#include "nsIStringBundle.h"

typedef struct AbCard
{
  nsIAbCard *card;
  PRUint32 primaryCollationKeyLen;
  PRUint32 secondaryCollationKeyLen;
  PRUint8 *primaryCollationKey;
  PRUint8 *secondaryCollationKey;
} AbCard;


class nsAbView : public nsIAbView, public nsITreeView, public nsIAbListener, public nsIObserver
{
public:
  nsAbView();
  virtual ~nsAbView();

  NS_DECL_ISUPPORTS
  NS_DECL_NSIABVIEW
  NS_DECL_NSITREEVIEW
  NS_DECL_NSIABLISTENER
  NS_DECL_NSIOBSERVER
  
  PRInt32 CompareCollationKeys(PRUint8 *key1, PRUint32 len1, PRUint8 *key2, PRUint32 len2);

private:
  nsresult Initialize();
  PRInt32 FindIndexForInsert(AbCard *abcard);
  PRInt32 FindIndexForCard(nsIAbCard *card);
  nsresult GenerateCollationKeysForCard(const PRUnichar *colID, AbCard *abcard);
  nsresult InvalidateTree(PRInt32 row);
  nsresult RemoveCardAt(PRInt32 row);
  nsresult AddCard(AbCard *abcard, bool selectCardAfterAdding, PRInt32 *index);
  nsresult RemoveCardAndSelectNextCard(nsISupports *item);
  nsresult EnumerateCards();
  nsresult SetGeneratedNameFormatFromPrefs();
  nsresult GetSelectedCards(nsCOMPtr<nsIMutableArray> &aSelectedCards);
  nsresult ReselectCards(nsIArray *aCards, nsIAbCard *aIndexCard);
  nsresult GetCardValue(nsIAbCard *card, const PRUnichar *colID, nsAString &_retval);
  nsresult RefreshTree();

  nsCOMPtr<nsITreeBoxObject> mTree;
  nsCOMPtr<nsITreeSelection> mTreeSelection;
  nsCOMPtr <nsIAbDirectory> mDirectory;
  nsVoidArray mCards;
  nsCOMPtr<nsIAtom> mMailListAtom;
  nsString mSortColumn;
  nsString mSortDirection;
  nsCOMPtr<nsICollation> mCollationKeyGenerator;
  nsCOMPtr<nsIAbViewListener> mAbViewListener;
  nsCOMPtr<nsIStringBundle> mABBundle;

  bool mInitialized;
  bool mSuppressSelectionChange;
  bool mSuppressCountChange;
  PRInt32 mGeneratedNameFormat;
};

#endif /* _nsAbView_H_ */
