/* -*- Mode: Java; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
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
 * Ben Bucksch <ben.bucksch beonex.com>
 * Portions created by the Initial Developer are Copyright (C) 2008-2009
 * the Initial Developer. All Rights Reserved.
 *
 * Contributor(s):
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

/**
 * This is a generic input validation lib. Use it when you process
 * data from the network.
 *
 * Just a few functions which verify, for security purposes, that the
 * input variables (strings, if nothing else is noted) are of the expected
 * type and syntax.
 *
 * The functions take a string (unless noted otherwise) and return
 * the expected datatype in JS types. If the value is not as expected,
 * they throw exceptions.
 */

var sanitize =
{
  integer : function(unchecked)
  {
    if (typeof(unchecked) == "number" && !isNaN(unchecked))
      return unchecked;

    var r = parseInt(unchecked);
    if (isNaN(r))
      throw new MalformedException("no_number.error", unchecked);

    return r;
  },

  integerRange : function(unchecked, min, max)
  {
    var int = this.integer(unchecked);
    if (int < min)
      throw new MalformedException("number_too_small.error", unchecked);

    if (int > max)
      throw new MalformedException("number_too_large.error", unchecked);

    return int;
  },

  boolean : function(unchecked)
  {
    if (typeof(unchecked) == "boolean")
      return unchecked;

    if (unchecked == "true")
      return true;

    if (unchecked == "false")
      return false;

    throw new MalformedException("boolean.error", unchecked);
  },

  string : function(unchecked)
  {
    return String(unchecked);
  },

  nonemptystring : function(unchecked)
  {
    if (!unchecked)
      throw new MalformedException("string_empty.error", unchecked);

    return this.string(unchecked);
  },

  /**
   * Allow only letters, numbers, "-" and "_".
   *
   * Empty strings not allowed (good idea?).
   */
  alphanumdash : function(unchecked)
  {
    var str = this.nonemptystring(unchecked);
    if (!/^[a-zA-Z0-9\-\_]*$/.test(str))
      throw new MalformedException("alphanumdash.error", unchecked);

    return str;
  },

  /**
   * DNS hostnames like foo.bar.example.com
   * Allow only letters, numbers, "-" and "."
   * Empty strings not allowed.
   * Currently does not support IDN (international domain names).
   */
  hostname : function(unchecked)
  {
    var str = this.nonemptystring(unchecked);

    // Allow placeholders. TODO move to a new hostnameOrPlaceholder()
    // The regex is "anything, followed by one or more (placeholders than
    // anything)".  This doesn't catch the non-placeholder case, but that's
    // handled down below.
    if (/^[a-zA-Z0-9\-\.]*(%[A-Z0-9]+%[a-zA-Z0-9\-\.]*)+$/.test(str))
      return str;

    if (!/^[a-zA-Z0-9\-\.]*$/.test(str))
      throw new MalformedException("hostname_syntax.error", unchecked);

    return str.toLowerCase();
  },
  /**
   * A non-chrome URL that's safe to request.
   */
  url : function (unchecked)
  {
    var str =  this.string(unchecked);
    if (str.substr(0, 4) != "http" && str.substr(0, 5) != "https")
      throw new MalformedException("url_scheme.error", unchecked);

    var uri;
    try {
      uri = ioService().newURI(str, null, null);
      uri = uri.QueryInterface(Ci.nsIURL);
    } catch (e) {
      throw new MalformedException("url_parsing.error", unchecked);
    }

    if (uri.scheme != "http" && uri.scheme != "https")
      throw new MalformedException("url_scheme.error", unchecked);

    return uri.spec;
  },

  /**
   * A value which should be shown to the user in the UI as label
   */
  label : function(unchecked)
  {
    return this.string(unchecked);
  },

  /**
   * Allows only certain values as input, otherwise throw.
   *
   * @param unchecked {Any} The value to check
   * @param allowedValues {Array} List of values that |unchecked| may have.
   * @param defaultValue {Any} (Optional) If |unchecked| does not match
   *       anything in |mapping|, a |defaultValue| can be returned instead of
   *       throwing an exception. The latter is the default and happens when
   *       no |defaultValue| is passed.
   * @throws MalformedException
   */
  enum : function(unchecked, allowedValues, defaultValue)
  {
    for each (var allowedValue in allowedValues)
    {
      if (allowedValue == unchecked)
        return allowedValue;
    }
    // value is bad
    if (typeof(defaultValue) == "undefined")
      throw new MalformedException("allowed_value.error", unchecked);
    return defaultValue;
  },

  /**
   * Like enum, allows only certain (string) values as input, but allows the
   * caller to specify another value to return instead of the input value. E.g.,
   * if unchecked == "foo", return 1, if unchecked == "bar", return 2,
   * otherwise throw. This allows to translate string enums into integer enums.
   *
   * @param unchecked {Any} The value to check
   * @param mapping {Object} Associative array. property name is the input
   *       value, property value is the output value. E.g. the example above
   *       would be: { foo: 1, bar : 2 }.
   *       Use quotes when you need freaky characters: "baz-" : 3.
   * @param defaultValue {Any} (Optional) If |unchecked| does not match
   *       anything in |mapping|, a |defaultValue| can be returned instead of
   *       throwing an exception. The latter is the default and happens when
   *       no |defaultValue| is passed.
   * @throws MalformedException
   */
  translate : function(unchecked, mapping, defaultValue)
  {
    for (var inputValue in mapping)
    {
      if (inputValue == unchecked)
        return mapping[inputValue];
    }
    // value is bad
    if (typeof(defaultValue) == "undefined")
      throw new MalformedException("allowed_value.error", unchecked);
    return defaultValue;
  }
};

function MalformedException(msgID, uncheckedBadValue)
{
  var stringBundle = getStringBundle(
      "chrome://messenger/locale/accountCreationUtil.properties");
  var msg = stringBundle.GetStringFromName(msgID);
  if (kDebug)
    msg += " (bad value: " + new String(uncheckedBadValue) + ")";
  Exception.call(this, msg);
}
extend(MalformedException, Exception);
