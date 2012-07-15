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
 * The Original Code is Thunderbird Mail Client.
 *
 * The Initial Developer of the Original Code is
 * the Mozilla Foundation.
 * Portions created by the Initial Developer are Copyright (C) 2009
 * the Initial Developer. All Rights Reserved.
 *
 * Contributor(s):
 *   Mark Banner <bugzilla@standard8.pus.com>
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

/**
 * Test file to check that cookies are correctly enabled in Thunderbird.
 *
 * XXX: Still need to check remote content in messages.
 */

var MODULE_NAME = 'test-cookies';

var RELATIVE_ROOT = "../shared-modules";
var MODULE_REQUIRES = ['window-helpers', 'content-tab-helpers', 'folder-display-helpers'];

var mozmill = {}; Components.utils.import('resource://mozmill/modules/mozmill.js', mozmill);
var elementslib = {}; Components.utils.import('resource://mozmill/modules/elementslib.js', elementslib);

// RELATIVE_ROOT messes with the collector, so we have to bring the path back
// so we get the right path for the resources.
var url = collector.addHttpResource('../cookies/html', 'cookies');

function setupModule(module) {
  let fdh = collector.getModule("folder-display-helpers");
  fdh.installInto(module);
  let wh = collector.getModule('window-helpers');
  wh.installInto(module);
  let cth = collector.getModule("content-tab-helpers");
  cth.installInto(module);
}

/**
 * Test deleting junk messages with no messages marked as junk.
 */
function test_load_cookie_page() {
  open_content_tab_with_url(url + "cookietest1.html");
}

function test_load_cookie_result_page() {
  open_content_tab_with_url(url + "cookietest2.html");

  if (mc.window.content.document.title != "Cookie Test 2")
    throw new Error("The cookie test 2 page is not the selected tab or not content-primary");

  let cookie = mc.window.content.wrappedJSObject.theCookie;

  dump("Cookie is: " + cookie + "\n");

  if (!cookie)
    throw new Error("Document has no cookie :-(");

  if (cookie != "name=CookieTest")
    throw new Error("Cookie set incorrectly, expected: name=CookieTest, got: " +cookie + "\n");
}
