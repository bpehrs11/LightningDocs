/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

package org.mozilla.gecko.sync.jpake.stage;

import java.io.IOException;
import java.io.Reader;
import java.io.StringReader;
import java.io.UnsupportedEncodingException;

import org.json.simple.JSONObject;
import org.json.simple.parser.JSONParser;
import org.json.simple.parser.ParseException;
import org.mozilla.gecko.sync.ExtendedJSONObject;
import org.mozilla.gecko.sync.Logger;
import org.mozilla.gecko.sync.NonObjectJSONException;
import org.mozilla.gecko.sync.Utils;
import org.mozilla.gecko.sync.crypto.CryptoException;
import org.mozilla.gecko.sync.crypto.CryptoInfo;
import org.mozilla.gecko.sync.crypto.KeyBundle;
import org.mozilla.gecko.sync.jpake.JPakeClient;
import org.mozilla.gecko.sync.setup.Constants;

public class DecryptDataStage extends JPakeStage {

  @Override
  public void execute(JPakeClient jClient) {
    Logger.debug(LOG_TAG, "Decrypting their payload.");
    if (!(jClient.theirSignerId + "3").equals((String) jClient.jIncoming
        .get(Constants.JSON_KEY_TYPE))) {
      Logger.error(LOG_TAG, "Invalid round 3 data: " + jClient.jIncoming.toJSONString());
      jClient.abort(Constants.JPAKE_ERROR_WRONGMESSAGE);
      return;
    }

    // Decrypt payload and verify HMAC.
    Logger.debug(LOG_TAG, "Decrypting payload.");
    ExtendedJSONObject iPayload = null;
    try {
      iPayload = jClient.jIncoming.getObject(Constants.JSON_KEY_PAYLOAD);
    } catch (NonObjectJSONException e1) {
      Logger.error(LOG_TAG, "Invalid round 3 data.", e1);
      jClient.abort(Constants.JPAKE_ERROR_WRONGMESSAGE);
      return;
    }
    Logger.debug(LOG_TAG, "Decrypting data.");
    String cleartext = null;
    try {
      cleartext = new String(decryptPayload(iPayload, jClient.myKeyBundle), "UTF-8");
    } catch (UnsupportedEncodingException e) {
      Logger.error(LOG_TAG, "Failed to decrypt data.", e);
      jClient.abort(Constants.JPAKE_ERROR_INTERNAL);
      return;
    } catch (CryptoException e) {
      Logger.error(LOG_TAG, "Failed to decrypt data.", e);
      jClient.abort(Constants.JPAKE_ERROR_KEYMISMATCH);
      return;
    }
    try {
      jClient.jCreds = getJSONObject(cleartext);
    } catch (IOException e) {
      Logger.error(LOG_TAG, "I/O exception while creating JSON object.", e);
      jClient.abort(Constants.JPAKE_ERROR_INVALID);
      return;
    } catch (ParseException e) {
      Logger.error(LOG_TAG, "JSON parse error.", e);
      jClient.abort(Constants.JPAKE_ERROR_INVALID);
      return;
    }

    jClient.runNextStage();
  }

  /**
   * Helper method for doing actual decryption.
   *
   * Input: JSONObject containing a valid payload (cipherText, IV, HMAC),
   * KeyBundle with keys for decryption. Output: byte[] clearText
   *
   * @throws CryptoException
   * @throws UnsupportedEncodingException
   */
  private byte[] decryptPayload(ExtendedJSONObject payload, KeyBundle keybundle)
      throws CryptoException, UnsupportedEncodingException {

    String sCiphertext = (String) payload.get(Constants.JSON_KEY_CIPHERTEXT);
    String sIv = (String) payload.get(Constants.JSON_KEY_IV);
    String sHmac = (String) payload.get(Constants.JSON_KEY_HMAC);

    byte[] ciphertext = Utils.decodeBase64(sCiphertext);
    byte[] iv = Utils.decodeBase64(sIv);
    byte[] hmac = Utils.hex2Byte(sHmac);

    CryptoInfo decrypted = CryptoInfo.decrypt(ciphertext, iv, hmac, keybundle);
    return decrypted.getMessage();
  }

  /**
   *
   * @param jsonString
   *          String to be packaged as JSON object.
   * @return JSONObject
   * @throws ParseException
   * @throws IOException
   * @throws Exception
   */
  private JSONObject getJSONObject(String jsonString) throws IOException, ParseException{
    Reader in = new StringReader(jsonString);
    return (JSONObject) new JSONParser().parse(in);
  }
}
