/* -*- Mode: JavaScript; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
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
 * Portions created by the Initial Developer are Copyright (C) 2010
 * the Initial Developer. All Rights Reserved.
 *
 * Contributor(s):
 *   Siddharth Agarwal <sid.bugzilla@gmail.com>
 *   Mike Conley <mconley@mozilla.com>
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
 * This module handles migrating mail-specific preferences, etc. Migration has
 * traditionally been a part of msgMail3PaneWindow.js, but separating the code
 * out into a module makes unit testing much easier.
 */

var EXPORTED_SYMBOLS = ["MailMigrator"];

const Cc = Components.classes;
const Ci = Components.interfaces;
const Cu = Components.utils;

Cu.import("resource://gre/modules/XPCOMUtils.jsm");
Cu.import("resource://gre/modules/Services.jsm");

var MailMigrator = {
  /**
   * Switch the given fonts to the given encodings, but only if the current fonts
   * are defaults.
   */
  _switchDefaultFonts: function MailMigrator__switchDefaultFonts(aFonts,
                                                                 aEncodings) {
    for each (let [, encoding] in Iterator(aEncodings)) {
      let serifPref = "font.name.serif." + encoding;
      let sansPref = "font.name.sans-serif." + encoding;
      let variableSizePref = "font.size.variable." + encoding;
      // This is expected to be one of sans-serif or serif, and determines what
      // we'll link the variable font size to.
      let isSansDefault = this._prefBranch.getCharPref("font.default." + encoding) ==
                            "sans-serif";

      if (!this._prefBranch.prefHasUserValue(serifPref)) {
        this._prefBranch.setCharPref(serifPref, aFonts.serif);
        if (!isSansDefault)
          this._prefBranch.setIntPref(variableSizePref, aFonts.variableSize);
      }

      if (!this._prefBranch.prefHasUserValue(sansPref)) {
        this._prefBranch.setCharPref(sansPref, aFonts.sans);
        if (isSansDefault)
          this._prefBranch.setIntPref(variableSizePref, aFonts.variableSize);
      }

      let monospacePref = "font.name.monospace." + encoding;
      let fixedSizePref = "font.size.fixed." + encoding;
      if (!this._prefBranch.prefHasUserValue(monospacePref)) {
        this._prefBranch.setCharPref(monospacePref, aFonts.monospace);
        this._prefBranch.setIntPref(fixedSizePref, aFonts.fixedSize);
      }
    }
  },

  /**
   * Migrate to ClearType fonts (Cambria, Calibri and Consolas) on Windows Vista
   * and above.
   */
  migrateToClearTypeFonts: function MailMigrator_migrateToClearTypeFonts() {
    // Windows...
    if ("@mozilla.org/windows-registry-key;1" in Components.classes) {
      // Only migrate on Vista (Windows version 6.0) and above
      let sysInfo = Cc["@mozilla.org/system-info;1"]
                      .getService(Ci.nsIPropertyBag2);
      if (sysInfo.getPropertyAsDouble("version") >= 6.0) {
        let fontPrefVersion =
          this._prefBranch.getIntPref("mail.font.windows.version");
        if (fontPrefVersion < 2) {
          let fonts = {
            serif: "Cambria",
            sans: "Calibri",
            monospace: "Consolas",
            variableSize: 17,
            fixedSize: 14,
          };
          // Encodings to switch to the new fonts.
          let encodings = [];
          // (Thunderbird 3.1)
          if (fontPrefVersion < 1)
            encodings.push("x-unicode", "x-western");
          // (Thunderbird 3.2)
          encodings.push("x-central-euro", "x-cyrillic", "x-baltic", "el", "tr");

          this._switchDefaultFonts(fonts, encodings);

          this._prefBranch.setIntPref("mail.font.windows.version", 2);
        }
      }
    }
  },

  /**
   * Determine if the UI has been upgraded in a way that requires us to reset
   * some user configuration.  If so, performs the resets.
   */
  _migrateUI: function MailMigrator__migrateUI() {
    // The code for this was ported from
    // mozilla/browser/components/nsBrowserGlue.js
    const UI_VERSION = 4;
    const MESSENGER_DOCURL = "chrome://messenger/content/messenger.xul#";
    const UI_VERSION_PREF = "mail.ui-rdf.version";
    let currentUIVersion = 0;

    try {
      currentUIVersion = Services.prefs.getIntPref(UI_VERSION_PREF);
    } catch(ex) {}

    if (currentUIVersion >= UI_VERSION)
      return;

    this._rdf = Cc["@mozilla.org/rdf/rdf-service;1"].getService(Ci.nsIRDFService);
    this._dataSource = this._rdf.GetDataSource("rdf:local-store");
    let dirty = false;

    try {
      // Initially, we checked if currentUIVersion < 1, and stripped the
      // persisted "collapsed" property from folderPaneBox if it wasn't.
      // However, the inital implementation of migrateUI swallowed up
      // exceptions, and bumped the value of UI_VERSION_PREF regardless.
      // Now, instead, we fail to bump the UI_VERSION_PREF if something goes
      // wrong, and we've moved the folderPaneBox operation into
      // currentUIVersion < 2 just in case the operation failed for some of
      // our users the first time.
      if (currentUIVersion < 2) {
        // We want to remove old settings that collapse the folderPaneBox
        let fpbResource = this._rdf.GetResource(MESSENGER_DOCURL
                                                + "folderPaneBox");
        let collapsedResource = this._rdf.GetResource("collapsed");
        let collapsed = this._getPersist(fpbResource, collapsedResource);

        if (collapsed !== null) {
          // We want to override this, and set it to false.  We should really
          // be ignoring this persist attribute, anyhow.
          dirty = true;
          this._unAssert(fpbResource, collapsedResource);
        }

        // We want to remove the throbber from the menubar on Linux and
        // Windows, and from the mail-toolbar on OSX.
        let currentSetResource = this._rdf.GetResource("currentset");
        let barResource = null;

        if (Services.appinfo.OS == "Darwin")
          barResource = this._rdf.GetResource(MESSENGER_DOCURL + "mail-bar3");
        else
          barResource = this._rdf.GetResource(MESSENGER_DOCURL +
                                              "mail-toolbar-menubar2");

        if (barResource !== null) {
          let currentSet = this._getPersist(barResource, currentSetResource);
          if (currentSet &&
              currentSet.indexOf("throbber-box") != -1) {
            dirty = true;
            currentSet = currentSet.replace(/(^|,)throbber-box($|,)/, "$1$2");
            this._setPersist(barResource, currentSetResource, currentSet);
          }
        }
      }

      // In UI version 3, we move the QFB button from the tabbar toolbar to
      // to the mail toolbar.
      if (currentUIVersion < 3) {
        let currentSetResource = this._rdf.GetResource("currentset");
        let tbtResource = this._rdf.GetResource(MESSENGER_DOCURL
                                                + "tabbar-toolbar");
        if (tbtResource !== null) {
          let currentSet = this._getPersist(tbtResource, currentSetResource);
          if (currentSet
              && currentSet.indexOf("qfb-show-filter-bar") != -1) {
            dirty = true;
            currentSet = currentSet.replace(/(^|,)qfb-show-filter-bar($|,)/,
                                            "$1$2");
            this._setPersist(tbtResource, currentSetResource, currentSet);
          }
        }

        let barResource = this._rdf.GetResource(MESSENGER_DOCURL + "mail-bar3");
        if (barResource !== null) {
          let currentSet = this._getPersist(barResource, currentSetResource);

          if (currentSet
              && currentSet.indexOf("qfb-show-filter-bar") == -1) {

            dirty = true;
            if (currentSet.indexOf("gloda-search") != -1) {
              // Put the QFB toggle before the gloda-search and any of
              // spring / spacer / separator.
              currentSet = currentSet.replace(/(^|,)([spring,|spacer,|separator,]*)gloda-search($|,)/,
                                              "$1qfb-show-filter-bar,$2gloda-search$3");
            } else {
              // If there's no gloda-search, just put the QFB toggle at the end
              currentSet = currentSet + ",qfb-show-filter-bar";
            }
            this._setPersist(barResource, currentSetResource, currentSet);
          }
        }
      }

      // In UI version 4, we add the chat button to the mail toolbar.
      if (currentUIVersion < 4) {
        let currentSetResource = this._rdf.GetResource("currentset");
        let barResource = this._rdf.GetResource(MESSENGER_DOCURL + "mail-bar3");
        if (barResource !== null) {
          let currentSet = this._getPersist(barResource, currentSetResource);

          if (currentSet
              && currentSet.indexOf("button-chat") == -1) {

            dirty = true;
            if (currentSet.indexOf("button-newmsg") != -1) {
              // Put the chat button after the newmsg button.
              currentSet = currentSet.replace(/(^|,)button-newmsg($|,)/,
                                              "$1button-newmsg,button-chat$2");
            } else if (currentSet.indexOf("button-address") != -1) {
              // If there's no newmsg button, put the chat button before the address book button.
              currentSet = currentSet.replace(/(^|,)button-address($|,)/,
                                              "$1button-chat,button-address$2");
            } else {
              // Otherwise, just put the chat button at the end.
              currentSet = currentSet + ",button-chat";
            }
            this._setPersist(barResource, currentSetResource, currentSet);
          }
        }
      }

      // Update the migration version.
      Services.prefs.setIntPref(UI_VERSION_PREF, UI_VERSION);

    } catch(e) {
      Cu.reportError("Migrating from UI version " + currentUIVersion + " to "
                     + UI_VERSION + " failed. Error message was: " + e + " -- "
                     + "Will reattempt on next start.");
    } finally {
      if (dirty)
        this._dataSource.QueryInterface(Ci.nsIRDFRemoteDataSource).Flush();
      delete this._rdf;
      delete this._dataSource;
    }
  },

  /**
   * Perform any migration work that needs to occur after the Account Wizard
   * has had a chance to appear.
   */
  migratePostAccountWizard: function MailMigrator_migratePostAccountWizard() {
    this.migrateToClearTypeFonts();
  },

  /**
   * Perform any migration work that needs to occur once the user profile has
   * been loaded.
   */
  migrateAtProfileStartup: function MailMigrator_migrateAtProfileStartup() {
    this._migrateUI();
  },

  /**
   * A helper function to get the property for a resource in the
   * localstore.rdf file.  This function should only be called by _migrateUI.
   *
   * @param aSource the resource to get the property from
   * @param aProperty the property to get the value from
   */
  _getPersist: function MailMigrator__getPersist(aSource, aProperty) {
    // The code for this was ported from
    // mozilla/browser/components/nsBrowserGlue.js.

    let target = this._dataSource.GetTarget(aSource, aProperty, true);
    if (target instanceof Ci.nsIRDFLiteral)
      return target.Value;
    return null;
  },

  /**
   * A helper function to set the property for a resource in the localstore.rdf.
   * This function also automatically adds the property to the list of properties
   * being persisted for the aSource.
   *
   * @param aSource the resource that we want to set persistence on
   * @param aProperty the property that we're going to set the value of
   * @param aTarget the value that we're going to set the property to
   */
  _setPersist: function MailMigrator__setPersist(aSource, aProperty, aTarget) {
    try {
      let oldTarget = this._dataSource.GetTarget(aSource, aProperty, true);
      if (oldTarget) {
        if (aTarget)
          this._dataSource.Change(aSource, aProperty, oldTarget, this._rdf.GetLiteral(aTarget));
        else
          this._dataSource.Unassert(aSource, aProperty, oldTarget);
      }
      else {
        this._dataSource.Assert(aSource, aProperty, this._rdf.GetLiteral(aTarget), true);
      }

      // Add the entry to the persisted set for this document if it's not there.
      // This code is mostly borrowed from nsXULDocument::Persist.
      let docURL = aSource.ValueUTF8.split("#")[0];
      let docResource = this._rdf.GetResource(docURL);
      let persistResource = this._rdf.GetResource("http://home.netscape.com/NC-rdf#persist");
      if (!this._dataSource.HasAssertion(docResource, persistResource, aSource, true)) {
        this._dataSource.Assert(docResource, persistResource, aSource, true);
      }
    }
    catch(e) {
      // Something's gone horribly wrong - report it in the Error Console
      Cu.reportError(e);
      throw(e);
    }
  },

  /**
   * A helper function to unassert a property from a resource.  This function
   * should only be called by _migrateUI.
   *
   * @param aSource the resource to remove the property from
   * @param aProperty the property to be removed
   */
  _unAssert: function MailMigrator__unAssert(aSource, aProperty) {
    try {
      let oldTarget = this._dataSource.GetTarget(aSource, aProperty, true);
      if (oldTarget)
        this._dataSource.Unassert(aSource, aProperty, oldTarget);
    }
    catch(e) {
      // If something's gone wrong here, report it in the Error Console.
      Cu.reportError(e);
      throw(e);
    }
  },
};

XPCOMUtils.defineLazyServiceGetter(MailMigrator, "_prefBranch",
                                   "@mozilla.org/preferences-service;1",
                                   "nsIPrefBranch");
