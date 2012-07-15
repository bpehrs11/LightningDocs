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
 * The Original Code is SeaMonkey Project Code.
 *
 * The Initial Developer of the Original Code is
 * Bruno Escherl <aqualon@aquachan.de>.
 * Portions created by the Initial Developer are Copyright (C) 2008
 * the Initial Developer. All Rights Reserved.
 *
 * Contributor(s):
 *   Ian Neal <iann_bugzilla@blueyonder.co.uk>
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

// The content of this file is loaded into the scope of the
// prefwindow and will be available to all prefpanes!

function EnableElementById(aElementId, aEnable, aFocus)
{
  EnableElement(document.getElementById(aElementId), aEnable, aFocus);
}

function EnableElement(aElement, aEnable, aFocus)
{
  let pref = document.getElementById(aElement.getAttribute("preference"));
  let enabled = aEnable && !pref.locked;

  aElement.disabled = !enabled;

  if (enabled && aFocus)
    aElement.focus();
}

function WriteSoundField(aField, aValue)
{
  var file = GetFileFromString(aValue);
  if (file)
  {
    aField.file = file;
    aField.label = (/Mac/.test(navigator.platform)) ? file.leafName : file.path;
  }
}

function SelectSound(aSoundUrlPref)
{
  const nsIFilePicker = Components.interfaces.nsIFilePicker;
  var fp = Components.classes["@mozilla.org/filepicker;1"]
                     .createInstance(nsIFilePicker);
  var prefutilitiesBundle = document.getElementById("bundle_prefutilities");
  fp.init(window, prefutilitiesBundle.getString("choosesound"),
          nsIFilePicker.modeOpen);

  var file = GetFileFromString(aSoundUrlPref.value);
  if (file && file.parent && file.parent.exists())
    fp.displayDirectory = file.parent;

  var filterExts = "*.wav; *.wave";
  // On Mac, allow AIFF files too.
  if (/Mac/.test(navigator.platform))
    filterExts += "; *.aif; *.aiff";
  fp.appendFilter(prefutilitiesBundle.getString("SoundFiles"), filterExts);
  fp.appendFilters(nsIFilePicker.filterAll);

  if (fp.show() == nsIFilePicker.returnOK)
    aSoundUrlPref.value = fp.fileURL.spec;
}

function PlaySound(aValue, aMail)
{
  const nsISound = Components.interfaces.nsISound;
  var sound = Components.classes["@mozilla.org/sound;1"]
                        .createInstance(nsISound);

  if (aValue)
    sound.play(Services.io.newURI(aValue, null, null));
  else if (aMail && !/Mac/.test(navigator.platform))
    sound.playEventSound(nsISound.EVENT_NEW_MAIL_RECEIVED);
  else
    sound.beep();
}
