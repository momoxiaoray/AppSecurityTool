package com.jni.tool;
/**
 * method: frida_检测
 * author: Daimc(xiaocheng.ok@qq.com)
 * date: 2020/7/14 10:18
 * description:
 */
public class JNIFridaDetectTool {
    static {
        System.loadLibrary("frida_detact");
    }

    //单例
    private static class SdkHodler {
        static JNIFridaDetectTool instance = new JNIFridaDetectTool();
    }

    public static JNIFridaDetectTool getInstance() {
        return SdkHodler.instance;
    }

    public native void fridaDetect(OnFridaDetectListener listener);

    /**
     * 检测Frida hook框架回调
     */
    public interface OnFridaDetectListener {
        public int onDetected();
    }
}
