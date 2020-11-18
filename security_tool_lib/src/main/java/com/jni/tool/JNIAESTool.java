package com.jni.tool;

import android.content.Context;
import android.content.pm.PackageInfo;
import android.content.pm.PackageManager;
import android.content.pm.Signature;
import android.os.Build;

import java.io.UnsupportedEncodingException;
import java.nio.charset.StandardCharsets;

/**
 * method: 加解密
 * author: Daimc(xiaocheng.ok@qq.com)
 * date: 2020/7/14 10:14
 * description:
 */
public class JNIAESTool {
    static {
        System.loadLibrary("jni_tool");
    }

    private static native String jniencrypt(byte[] bytes);

    private static native byte[] jnidecrypt(String str);

    public static native String pwdMD5(String str);

    public static synchronized String encrypt(String str) {
        return jniencrypt(str.getBytes());
    }

    public static synchronized String decrypt(String str) {
        if (Build.VERSION.SDK_INT >= 19) {
            return new String(jnidecrypt(str), StandardCharsets.UTF_8);
        } else {
            try {
                return new String(jnidecrypt(str), "UTF-8");
            } catch (UnsupportedEncodingException var3) {
                var3.printStackTrace();
                return new String(jnidecrypt(str));
            }
        }
    }

    //获取签名
    public static int getSignature(Context context) {
        try {
            PackageInfo packageInfo = context.getPackageManager().getPackageInfo(context.getPackageName(), PackageManager.GET_SIGNATURES);
            Signature sign = packageInfo.signatures[0];
            return sign.hashCode();
        } catch (Exception e) {
            e.printStackTrace();
        }
        return -1;
    }
}
