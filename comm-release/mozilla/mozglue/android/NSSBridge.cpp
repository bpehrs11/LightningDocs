/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <stdlib.h>
#include "dlfcn.h"
#include "NSSBridge.h"
#include "APKOpen.h"
#ifdef ANDROID
#include <jni.h>
#include <android/log.h>
#endif

#ifndef MOZ_OLD_LINKER
#include "ElfLoader.h"
#endif

#define DEBUG 1

#ifdef DEBUG
#define LOG(x...) __android_log_print(ANDROID_LOG_INFO, "GeckoJNI", x)
#else
#define LOG(x...) printf(x);
#endif

static bool initialized = false;

#define NSS_WRAPPER_INT(name) name ## _t f_ ## name;
NSS_WRAPPER_INT(NSS_Initialize)
NSS_WRAPPER_INT(NSS_Shutdown)
NSS_WRAPPER_INT(SECITEM_ZfreeItem)
NSS_WRAPPER_INT(PK11SDR_Encrypt)
NSS_WRAPPER_INT(PK11SDR_Decrypt)
NSS_WRAPPER_INT(PK11_GetInternalKeySlot)
NSS_WRAPPER_INT(PK11_NeedUserInit)
NSS_WRAPPER_INT(PK11_InitPin)
NSS_WRAPPER_INT(PR_ErrorToString)
NSS_WRAPPER_INT(PR_GetError)
NSS_WRAPPER_INT(PR_Free)
NSS_WRAPPER_INT(PL_Base64Encode)
NSS_WRAPPER_INT(PL_Base64Decode)
NSS_WRAPPER_INT(PL_strfree)

int
setup_nss_functions(void *nss_handle,
                        void *nspr_handle,
                        void *plc_handle)
{
  __android_log_print(ANDROID_LOG_ERROR, "GeckoLibLoad", "setup nss 1");
  if (nss_handle == NULL || nspr_handle == NULL || plc_handle == NULL) {
    LOG("missing handle\n");
    return FAILURE;
  }
#define GETFUNC(name) f_ ## name = (name ## _t) __wrap_dlsym(nss_handle, #name); \
                      if (!f_ ##name) return FAILURE;
  GETFUNC(NSS_Initialize);
  GETFUNC(NSS_Shutdown);
  GETFUNC(PK11SDR_Encrypt);
  GETFUNC(PK11SDR_Decrypt);
  GETFUNC(PK11_GetInternalKeySlot);
  GETFUNC(PK11_NeedUserInit);
  GETFUNC(PK11_InitPin);
  GETFUNC(SECITEM_ZfreeItem);
#undef GETFUNC
#define NSPRFUNC(name) f_ ## name = (name ## _t) __wrap_dlsym(nspr_handle, #name); \
                       if (!f_ ##name) return FAILURE;
  NSPRFUNC(PR_ErrorToString);
  NSPRFUNC(PR_GetError);
  NSPRFUNC(PR_Free);
#undef NSPRFUNC
#define PLCFUNC(name) f_ ## name = (name ## _t) __wrap_dlsym(plc_handle, #name); \
                      if (!f_ ##name) return FAILURE;
  PLCFUNC(PL_Base64Encode);
  PLCFUNC(PL_Base64Decode);
  PLCFUNC(PL_strfree);
#undef PLCFUNC

  return SUCCESS;
}

/* Throws the current NSS error. */
static void
throwError(JNIEnv* jenv, const char * funcString) {
    char *msg;

    PRErrorCode perr = f_PR_GetError();
    char * errString = f_PR_ErrorToString(perr, 0);
    asprintf(&msg, "%s returned error %d: %s\n", funcString, perr, errString);
    LOG("Throwing error: %s\n", msg);

    JNI_Throw(jenv, "java/lang/Exception", msg);
    free(msg);
    LOG("Error thrown\n");
}

extern "C" NS_EXPORT jstring JNICALL
Java_org_mozilla_gecko_NSSBridge_nativeEncrypt(JNIEnv* jenv, jclass,
                                               jstring jPath,
                                               jstring jValue)
{
    jstring ret = jenv->NewStringUTF("");

    const char* path;
    path = jenv->GetStringUTFChars(jPath, NULL);

    const char* value;
    value = jenv->GetStringUTFChars(jValue, NULL);

    char* result;
    SECStatus rv = doCrypto(jenv, path, value, &result, true);
    if (rv == SECSuccess) {
      ret = jenv->NewStringUTF(result);
      free(result);
    }

    jenv->ReleaseStringUTFChars(jValue, value);
    jenv->ReleaseStringUTFChars(jPath, path);

    return ret;
}

extern "C" NS_EXPORT jstring JNICALL
Java_org_mozilla_gecko_NSSBridge_nativeDecrypt(JNIEnv* jenv, jclass,
                                               jstring jPath,
                                               jstring jValue)
{
    jstring ret = jenv->NewStringUTF("");

    const char* path;
    path = jenv->GetStringUTFChars(jPath, NULL);

    const char* value;
    value = jenv->GetStringUTFChars(jValue, NULL);

    char* result;
    SECStatus rv = doCrypto(jenv, path, value, &result, false);
    if (rv == SECSuccess) {
      ret = jenv->NewStringUTF(result);
      free(result);
    }

    jenv->ReleaseStringUTFChars(jValue, value);
    jenv->ReleaseStringUTFChars(jPath, path);

    return ret;
}


/* Encrypts or decrypts a string. result should be freed with free() when done */
SECStatus
doCrypto(JNIEnv* jenv, const char *path, const char *value, char** result, bool encrypt)
{
    SECStatus rv;
    PK11SlotInfo *slot;
    if (!initialized) {
      LOG("initialize crypto %s\n", path);
      rv = f_NSS_Initialize(path, "", "", "secmod.db", NSS_INIT_NOROOTINIT);
      if (rv != SECSuccess) {
          throwError(jenv, "NSS_Initialize");
          return rv;
      }
      initialized = true;
    }

    slot = f_PK11_GetInternalKeySlot();
    if (!slot) {
      throwError(jenv, "PK11_GetInternalKeySlot");
      return SECFailure;
    }

    if (f_PK11_NeedUserInit(slot)) {
      LOG("Initializing key3.db with default blank password.");
      rv = f_PK11_InitPin(slot, NULL, NULL);
      if (rv != SECSuccess) {
        throwError(jenv, "PK11_InitPin");
        return rv;
      }
    }

    SECItem request;
    SECItem reply;

    reply.data = 0;
    reply.len = 0;

    if (encrypt) {
      LOG("encrypting %s\n", value);
      request.data = (unsigned char*)value;
      request.len = strlen(value);

      SECItem keyid;
      keyid.data = 0;
      keyid.len = 0;
      rv = f_PK11SDR_Encrypt(&keyid, &request, &reply, NULL);

      if (rv != SECSuccess) {
        throwError(jenv, "PK11SDR_Encrypt");
        goto done;
      }

      rv = encode(reply.data, reply.len, result);
      if (rv != SECSuccess) {
          throwError(jenv, "encode");
          goto done;
      }
      LOG("encrypted %s\n", *result);
    } else {
      LOG("decoding %s\n", value);
      rv = decode(value, &request.data, (PRInt32*)&request.len);
      if (rv != SECSuccess) {
          throwError(jenv, "decode");
          return rv;
      }

      rv = f_PK11SDR_Decrypt(&request, &reply, NULL);
      if (rv != SECSuccess) {
        throwError(jenv, "PK11SDR_Decrypt");
        goto done;
      }

      *result = (char *)malloc(reply.len);
      (*result)[reply.len] = '\0';
      strncpy(*result, (char *)reply.data, reply.len);
      //asprintf(result, "%s", (char *)reply.data);

      LOG("decoded %i letters %s\n", reply.len, *result);
      free(request.data);
    }

done:
    f_SECITEM_ZfreeItem(&reply, false);
    return rv;
}

/*
 * Base64 encodes the data passed in. The caller must deallocate _retval using free();
 */
SECStatus
encode(const unsigned char *data, PRInt32 dataLen, char **_retval)
{
  SECStatus rv = SECSuccess;
  char *encoded = f_PL_Base64Encode((const char *)data, dataLen, NULL);
  if (!encoded)
    rv = SECFailure;
  if (!*encoded)
    rv = SECFailure;

  if (rv == SECSuccess) {
    *_retval = (char *)malloc(strlen(encoded));
    strcpy(*_retval, encoded);
  }

  if (encoded) {
    f_PR_Free(encoded);
  }

  return rv;
}

/*
 * Base64 decodes the data passed in. The caller must deallocate result using free();
 */
SECStatus
decode(const char *data, unsigned char **result, PRInt32 *length)
{
  SECStatus rv = SECSuccess;
  PRUint32 len = strlen(data);
  int adjust = 0;

  /* Compute length adjustment */
  if (len > 0 && data[len-1] == '=') {
    adjust++;
    if (data[len-2] == '=') adjust++;
  }

  char *decoded;
  decoded = f_PL_Base64Decode(data, len, NULL);
  if (!decoded) {
    return SECFailure;
  }

  LOG("xxx Decoded: %s\n", decoded);

  if (!*decoded) {
    return SECFailure;
  }

  *length = (len*3)/4 - adjust;
  *result = (unsigned char*)malloc((size_t)len);

  if (!*result) {
    rv = SECFailure;
  } else {
    memcpy((char*)*result, decoded, len);
  }
  f_PR_Free(decoded);
  return rv;
}

