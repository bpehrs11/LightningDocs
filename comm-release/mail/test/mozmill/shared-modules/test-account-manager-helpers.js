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
 * The Original Code is Thunderbird Mail Client.
 *
 * The Initial Developer of the Original Code is
 * the Mozilla Foundation.
 * Portions created by the Initial Developer are Copyright (C) 2010
 * the Initial Developer. All Rights Reserved.
 *
 * Contributor(s):
 *   Mark Banner <bugzilla@standard8.plus.com>
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

var Ci = Components.interfaces;
var Cc = Components.classes;
var Cu = Components.utils;

var elib = {};
Cu.import('resource://mozmill/modules/elementslib.js', elib);
var mozmill = {};
Cu.import('resource://mozmill/modules/mozmill.js', mozmill);
var EventUtils = {};
Cu.import('resource://mozmill/stdlib/EventUtils.js', EventUtils);
var utils = {};
Cu.import('resource://mozmill/modules/utils.js', utils);

const MODULE_NAME = 'account-manager-helpers';
const RELATIVE_ROOT = '../shared-modules';

// we need this for the main controller
const MODULE_REQUIRES = ['folder-display-helpers', 'window-helpers'];

var wh, fdh, mc;

function setupModule() {
  fdh = collector.getModule('folder-display-helpers');
  mc = fdh.mc;
  wh = collector.getModule('window-helpers');
}

function installInto(module) {
  setupModule();

  // Now copy helper functions
  module.open_advanced_settings = open_advanced_settings;
  module.open_advanced_settings_from_account_wizard =
    open_advanced_settings_from_account_wizard;
  module.click_account_tree_row = click_account_tree_row;
}

/**
 * Opens the Account Manager.
 *
 * @param callback Callback for the modal dialog that is opened.
 */
function open_advanced_settings(aCallback, aController) {
  if (aController === undefined)
    aController = mc;

  wh.plan_for_modal_dialog("mailnews:accountmanager", aCallback);
  aController.click(mc.eid("menu_accountmgr"));
  return wh.wait_for_modal_dialog("mailnews:accountmanager");
}

/**
 * Opens the Account Manager from the mail account setup wizard.
 *
 * @param callback Callback for the modal dialog that is opened.
 */
function open_advanced_settings_from_account_wizard(aCallback, aController) {
  wh.plan_for_modal_dialog("mailnews:accountmanager", aCallback);
  aController.e("manual-edit_button").click();
  aController.e("advanced-setup_button").click();
  return wh.wait_for_modal_dialog("mailnews:accountmanager");
}

/**
 * Click a row in the account settings tree
 *
 * @param controller the Mozmill controller for the account settings dialog
 * @param rowIndex the row to click
 */
function click_account_tree_row(controller, rowIndex) {
  utils.waitFor(function () controller.window.currentAccount != null,
                "Timeout waiting for currentAccount to become non-null");

  var tree = controller.window.document.getElementById("accounttree");
  var selection = tree.view.selection;
  selection.select(rowIndex);
  tree.treeBoxObject.ensureRowIsVisible(rowIndex);

  // get cell coordinates
  var x = {}, y = {}, width = {}, height = {};
  var column = tree.columns[0];
  tree.treeBoxObject.getCoordsForCellItem(rowIndex, column, "text",
                                           x, y, width, height);

  controller.sleep(0);
  EventUtils.synthesizeMouse(tree.body, x.value + 4, y.value + 4,
                             {}, tree.ownerDocument.defaultView);
  controller.sleep(0);

  utils.waitFor(function () controller.window.pendingAccount == null,
                "Timeout waiting for pendingAccount to become null");
}
