package com.dmc.security;

import androidx.appcompat.app.AppCompatActivity;

import android.os.Bundle;
import android.widget.TextView;
import android.widget.Toast;

import com.dmc.security.R;
import com.jni.tool.JNIAESTool;
import com.jni.tool.JNIFridaDetectTool;
import com.security.util.SecurityCheckUtil;

public class MainActivity extends AppCompatActivity {
    boolean show = true;
    private TextView hello;

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_main);
        hello = findViewById(R.id.hello);
        String str = "12312312jlkajsdlkasjdklasjd";
        String encryptStr="AHL/+CtVvcaXf+//";
        String encrypt = JNIAESTool.encrypt(str);
//        hello.setText(String.format("原文：%s\n加密：%s \n解密:%s", str, encrypt, JNIAESTool.decrypt(encryptStr)));
        //检测FridaHook
        JNIFridaDetectTool.getInstance().fridaDetect(new JNIFridaDetectTool.OnFridaDetectListener() {
            @Override
            public int onDetected() {
                //回到UI线程操作
                runOnUiThread(new Runnable() {
                    @Override
                    public void run() {
                        if (show) {
                            Toast.makeText(MainActivity.this, "检测到有FridaHook，不安全", Toast.LENGTH_SHORT).show();

                        }
                    }
                });
                return 1;
            }
        });
        //检测root
        if (SecurityCheckUtil.getSingleInstance().isRoot()) {
            Toast.makeText(MainActivity.this, "手机root后，不安全", Toast.LENGTH_SHORT).show();
        }
        //检测Xposed
        if (SecurityCheckUtil.getSingleInstance().isXposedExistByThrow()) {
            Toast.makeText(MainActivity.this, "检测到有XposedHook，不安全", Toast.LENGTH_SHORT).show();
        }
    }
}