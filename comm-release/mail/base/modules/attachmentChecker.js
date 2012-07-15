/* ***** BEGIN LICENSE BLOCK *****
 *   Version: MPL 1.1/GPL 2.0/LGPL 2.1
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
 * the Mozilla Foundation.
 * Portions created by the Initial Developer are Copyright (C) 2009
 * the Initial Developer. All Rights Reserved.
 *
 * Contributor(s):
 *   Blake Winton <bwinton@latte.ca>
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

const EXPORTED_SYMBOLS = ["GetAttachmentKeywords"];

/**
 * Check whether the character is a CJK character or not.
 *
 * @return true if it is a CJK character.
 */
function IsCJK(code)
{
  if (code >= 0x2000 && code <= 0x9fff) {
    // Hiragana, Katakana and Kanaji
    return true;
  }
  else if (code >= 0xac00 && code <= 0xd7ff) {
    // Hangul
    return true;
  }
  else if (code >= 0xf900 && code <= 0xffff) {
    // Hiragana, Katakana and Kanaji
    return true;
  }
  return false;
}

/**
 * Get the (possibly-empty) list of attachment keywords in this message.
 *
 * @return the (possibly-empty) list of attachment keywords in this message
 **/
function GetAttachmentKeywords(mailData,keywordsInCsv)
{
  // The empty string would get split to an array of size 1.  Avoid that...
  var keywordsArray = (keywordsInCsv) ? keywordsInCsv.split(",") : [];

  function escapeRegxpSpecials(inputString)
  {
    const specials = [".", "\\", "^", "$", "*", "+", "?", "|",
                      "(", ")" , "[", "]", "{", "}"];
    var re = new RegExp("(\\"+specials.join("|\\")+")", "g");
    inputString = inputString.replace(re, "\\$1");
    return inputString.replace(" ", "\\s+");
  }

  // NOT_W is the character class that isn't in the Unicode classes "Ll",
  // "Lu" and "Lt".  It should work like \W, if \W knew about Unicode.
  const NOT_W = "[^\\u0041-\\u005a\\u0061-\\u007a\\u00aa\\u00b5\\u00ba\\u00c0-\\u00d6\\u00d8-\\u00f6\\u00f8-\\u01ba\\u01bc-\\u01bf\\u01c4-\\u02ad\\u0386\\u0388-\\u0481\\u048c-\\u0556\\u0561-\\u0587\\u10a0-\\u10c5\\u1e00-\\u1fbc\\u1fbe\\u1fc2-\\u1fcc\\u1fd0-\\u1fdb\\u1fe0-\\u1fec\\u1ff2-\\u1ffc\\u207f\\u2102\\u2107\\u210a-\\u2113\\u2115\\u2119-\\u211d\\u2124\\u2126\\u2128\\u212a-\\u212d\\u212f-\\u2131\\u2133\\u2134\\u2139\\ufb00-\\ufb17\\uff21-\\uff3a\\uff41-\\uff5a]";

  var keywordsFound = [];
  for (var i = 0; i < keywordsArray.length; i++) {
    var kw = escapeRegxpSpecials(keywordsArray[i]);
    // If the keyword starts (ends) with a CJK character, we don't care
    // what the previous (next) character is, because the words aren't
    // space delimited.
    var re;
    var matching;
    var isFileType = kw.charAt(1) == ".";
    var start;
    var end;
    if (isFileType) {
      start = "(([^\\s]*)\\b)";
      end = IsCJK(kw.charCodeAt(kw.length - 1)) ? "" : "(\\s|$)";
      re = new RegExp(start + kw + end, "ig");
      matching = mailData.match(re);
      if (matching) {
        var j, len;
        for (j = 0, len = matching.length; j < len; j++) {
          // Ignore the match if it was a URL.
          if (!(/^(http|ftp|https):\/\//i.test(matching[j])))
            keywordsFound.push(matching[j].trim());
        }
      }
    } else {
      start = IsCJK(kw.charCodeAt(0)) ? "" : ("(^|" + NOT_W + ")");
      end = IsCJK(kw.charCodeAt(kw.length - 1)) ? "" : ("(" + NOT_W + "|$)");
      re = new RegExp(start + kw + end, "i");
      matching = re.exec(mailData);
      if (matching)
        keywordsFound.push(keywordsArray[i]);
    }
  }
  return keywordsFound;
}

onmessage = function(event)
{
  var keywordsFound = GetAttachmentKeywords(event.data[0], event.data[1]);
  postMessage(keywordsFound);
};

