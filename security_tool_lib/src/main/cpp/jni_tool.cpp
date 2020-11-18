#include <jni.h>
#include <string>
#include "aes.h"
#include "md5.h"
//#include <android/log.h>

static const char *app_packageName = "com.dmc.demo";
static const int app_signature_hash_code = 1967296062;
static const uint8_t AES_KEY[] = "123";
static const uint8_t AES_IV[] = "123";
static const string PWD_MD5_KEY = "123456";

static jobject getApplication(JNIEnv *env) {
    jobject application = NULL;
    jclass activity_thread_clz = env->FindClass("android/app/ActivityThread");
    if (activity_thread_clz != NULL) {
        jmethodID currentApplication = env->GetStaticMethodID(
                activity_thread_clz, "currentApplication", "()Landroid/app/Application;");
        if (currentApplication != NULL) {
            application = env->CallStaticObjectMethod(activity_thread_clz, currentApplication);
        }
    }
    return application;
}

/**
 * 检测app签名
 * @param env
 * @return true正常，false非官方签名，说明被二次打包
 */
static bool checkSignature(JNIEnv *env) {
    jobject context = getApplication(env);
    //Context的类
    jclass context_clazz = env->GetObjectClass(context);
    // 得到 getPackageManager 方法的 ID
    jmethodID methodID_getPackageManager = env->GetMethodID(context_clazz, "getPackageManager",
                                                            "()Landroid/content/pm/PackageManager;");
    // 获得PackageManager对象
    jobject packageManager = env->CallObjectMethod(context, methodID_getPackageManager);
    // 获得 PackageManager 类
    jclass pm_clazz = env->GetObjectClass(packageManager);
    // 得到 getPackageInfo 方法的 ID
    jmethodID methodID_pm = env->GetMethodID(pm_clazz, "getPackageInfo",
                                             "(Ljava/lang/String;I)Landroid/content/pm/PackageInfo;");
    // 得到 getPackageName 方法的 ID
    jmethodID methodID_pack = env->GetMethodID(context_clazz,
                                               "getPackageName", "()Ljava/lang/String;");
    // 获得当前应用的包名
    jstring application_package = (jstring) env->CallObjectMethod(context, methodID_pack);
    const char *package_name = env->GetStringUTFChars(application_package, 0);
    // 获得PackageInfo
    jobject packageInfo = env->CallObjectMethod(packageManager, methodID_pm, application_package,
                                                64);
    jclass packageinfo_clazz = env->GetObjectClass(packageInfo);
    jfieldID fieldID_signatures = env->GetFieldID(packageinfo_clazz,
                                                  "signatures", "[Landroid/content/pm/Signature;");
    jobjectArray signature_arr = (jobjectArray) env->GetObjectField(packageInfo,
                                                                    fieldID_signatures);
    //Signature数组中取出第一个元素
    jobject signature = env->GetObjectArrayElement(signature_arr, 0);
    //读signature的hashcode
    jclass signature_clazz = env->GetObjectClass(signature);
    jmethodID methodID_hashcode = env->GetMethodID(signature_clazz, "hashCode", "()I");
    jint hashCode = env->CallIntMethod(signature, methodID_hashcode);

//    if (strcmp(package_name, app_packageName) != 0) {
//        return false;
//    }
    return hashCode == app_signature_hash_code;
}


//调用这个方法就开始判断
//jint JNI_OnLoad(JavaVM *vm, void *reserved) {
//    JNIEnv *env = NULL;
//    if (vm->GetEnv((void **) &env, JNI_VERSION_1_6) != JNI_OK) {
//        return JNI_ERR;
//    }
//    if (checkSignature(env)) {
//        return JNI_VERSION_1_6;
//    }
//    return JNI_ERR;
//}



extern "C"
JNIEXPORT jstring JNICALL
Java_com_jni_tool_JNIAESTool_jniencrypt(JNIEnv *env, jclass type, jbyteArray jbArr) {

    char *str = NULL;
    jsize alen = env->GetArrayLength(jbArr);
    jbyte *ba = env->GetByteArrayElements(jbArr, JNI_FALSE);
    str = (char *) malloc(alen + 1);
    memcpy(str, ba, alen);
    str[alen] = '\0';
    env->ReleaseByteArrayElements(jbArr, ba, 0);
    //三选一
//    char *result = AES_ECB_PKCS7_Encrypt(str, AES_KEY);//AES ECB PKCS7Padding加密
//    char *result = AES_CBC_PKCS7_Encrypt(str, AES_KEY, AES_IV);//AES CBC PKCS7Padding加密/PKCS5Padding加密
    char *result = AES_CBC_ZREO_Encrypt(str, AES_KEY, AES_IV);//AES CBC ZREOPadding加密
    return env->NewStringUTF(result);
}

extern "C"
JNIEXPORT jbyteArray JNICALL
Java_com_jni_tool_JNIAESTool_jnidecrypt(JNIEnv *env, jclass type, jstring out_str) {

    const char *str = env->GetStringUTFChars(out_str, 0);
    //三选一
//    char *result = AES_ECB_PKCS7_Decrypt(str, AES_KEY);//AES ECB PKCS7Padding解密
//    char *result = AES_CBC_PKCS7_Decrypt(str, AES_KEY, AES_IV);//AES CBC PKCS7Padding解密\PKCS5Padding解密
    char *result = AES_CBC_ZREO_Decrypt(str, AES_KEY, AES_IV);//AES CBC noPadding解密
    env->ReleaseStringUTFChars(out_str, str);

    jsize len = (jsize) strlen(result);
    jbyteArray jbArr = env->NewByteArray(len);
    env->SetByteArrayRegion(jbArr, 0, len, (jbyte *) result);
    return jbArr;
}

extern "C"
JNIEXPORT jstring JNICALL
Java_com_jni_tool_JNIAESTool_pwdMD5(JNIEnv *env, jclass type, jstring out_str) {

    const char *str = env->GetStringUTFChars(out_str, 0);
    string result = MD5(MD5(PWD_MD5_KEY + string(str)).toStr()).toStr();//加盐后进行两次md5
    env->ReleaseStringUTFChars(out_str, str);
    return env->NewStringUTF(("###" + result).data());//最后再加三个#
}
