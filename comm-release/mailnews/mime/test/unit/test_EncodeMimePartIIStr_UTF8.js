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
 * Kent James <kent@caspia.com>.
 * Portions created by the Initial Developer are Copyright (C) 2008
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

// This tests minimal mime encoding fixed in bug 458685

var converter = Components.classes["@mozilla.org/messenger/mimeconverter;1"]
                .getService(Components.interfaces.nsIMimeConverter);

function run_test() {
  var i;

  var checks =
  [
    ["", false, ""],
    ["\u0436", false, "=?UTF-8?B?0LY=?="], //CYRILLIC SMALL LETTER ZHE
    ["IamASCII", false, "IamASCII"],
    // Although an invalid email, we shouldn't crash on it (bug 479206)
    ["crash test@invalid.com>", true, "crash test@invalid.com>"],
  ];

  for (i = 0; i < checks.length; ++i)
  {
    do_check_eq(
      converter.encodeMimePartIIStr_UTF8(checks[i][0], checks[i][1], "UTF-8", 0, 72),
      checks[i][2]);
  }
}
