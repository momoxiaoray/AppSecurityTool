#include <stdio.h>
#include <jni.h>
#include <string.h>
#include <fcntl.h>
#include <pthread.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <android/log.h>
#include <unistd.h>
#include <errno.h>

#define APPNAME "MoJing"
#define MAX_LINE 512
int mNeedDetach;
int mLoop = 1;
JavaVM *g_VM;
extern "C" int my_openat(int, const char *, int, int);
extern "C" int my_read(int, void *, int);

static char keyword[] = "LIBFRIDA";

int find_mem_string(unsigned long, unsigned long, char *, unsigned int);

int scan_executable_segments(char *);

int read_one_line(int fd, char *buf, unsigned int max_len);

int callBack(JNIEnv *pEnv, jobject pJobject, jmethodID pId);

int find_mem_string(unsigned long start, unsigned long end, char *bytes, unsigned int len) {
    char *pmem = (char *) start;
    int matched = 0;
    while ((unsigned long) pmem < (end - len)) {
        if (*pmem == bytes[0]) {
            matched = 1;
            char *p = pmem + 1;
            while (*p == bytes[matched] && (unsigned long) p < end) {
                matched++;
                p++;
            }
            if (matched >= len) {
                return 1;
            }
        }
        pmem++;
    }
    return 0;
}

int scan_executable_segments(char *map) {
    char buf[512];
    unsigned long start, end;
    sscanf(map, "%lx-%lx %s", &start, &end, buf);
    if (buf[2] == 'x') {
        return (find_mem_string(start, end, (char *) keyword, 8) == 1);
    } else {
        return 0;
    }
}

int read_one_line(int fd, char *buf, unsigned int max_len) {
    char b;
    ssize_t ret;
    ssize_t bytes_read = 0;
    memset(buf, 0, max_len);
    do {
        ret = my_read(fd, &b, 1);
        if (ret != 1) {
            if (bytes_read == 0) {
                // error or EOF
                return -1;
            } else {
                return bytes_read;
            }
        }
        if (b == '\n') {
            return bytes_read;
        }
        *(buf++) = b;
        bytes_read += 1;
    } while (bytes_read < max_len - 1);
    return bytes_read;
}
// Used by syscall.S
extern "C" long __set_errno_internal(int n) {
    errno = n;
    return -1;
}

/**
 * 开启一个线程，检测frida
 * @param p
 * @return
 */
void *detect_frida_loop(void *p) {
    if (p == NULL) {
        return 0;
    }
    __android_log_print(ANDROID_LOG_VERBOSE, APPNAME, "1---------------");
    JNIEnv *env;
    //获取当前native线程是否有没有被附加到jvm环境中
    int getEnvStat = (*g_VM).GetEnv((void **) &env, JNI_VERSION_1_6);
    if (getEnvStat == JNI_EDETACHED) {
        //如果没有， 主动附加到jvm环境中，获取到env
        if ((*g_VM).AttachCurrentThread(&env, NULL) != 0) {
            return 0;
        }
        mNeedDetach = JNI_TRUE;
    }
    __android_log_print(ANDROID_LOG_VERBOSE, APPNAME, "2---------------");
    //强转回来
    jobject jcallback = (jobject) p;
    //通过强转后的jcallback 获取到要回调的类
    jclass javaClass = (*env).GetObjectClass(jcallback);
    if (javaClass == 0) {
        (*g_VM).DetachCurrentThread();
        return NULL;
    }
    __android_log_print(ANDROID_LOG_VERBOSE, APPNAME, "3---------------");
    //获取要回调的方法ID
    jmethodID javaCallbackId = (*env).GetMethodID(javaClass, "onDetected", "()I");
    if (javaCallbackId == NULL) {
        return 0;
    }
    __android_log_print(ANDROID_LOG_VERBOSE, APPNAME, "4---------------");
    struct sockaddr_in sa;
    memset(&sa, 0, sizeof(sa));
    sa.sin_family = AF_INET;
    inet_aton("127.0.0.1", &(sa.sin_addr));
    int sock;
    int fd;
    char map[MAX_LINE];
    char res[7];
    int num_found;
    int ret;
    int i;
    while (mLoop) {
        /*
         * 1: Scan memory for treacherous strings!
         * We also provide our own implementations of open() and read() - see syscall.S
         */
        num_found = 0;
        fd = my_openat(AT_FDCWD, "/proc/self/maps", O_RDONLY, 0);
        __android_log_print(ANDROID_LOG_VERBOSE, APPNAME, "5---------------my_openat:%d", fd);
        if (fd >= 0) {
            while ((read_one_line(fd, map, MAX_LINE)) > 0) {
                if (scan_executable_segments(map) == 1) {
                    num_found++;
                }
            }
            if (num_found > 1) {
                __android_log_print(ANDROID_LOG_VERBOSE, APPNAME,
                                    "FRIDA DETECTED [2] - suspect string found in memory!");
                if (callBack(env, jcallback, javaCallbackId) == 1)
                    break;
            }
        } else {
            __android_log_print(ANDROID_LOG_VERBOSE, APPNAME,
                                "Error opening /proc/self/maps. That's usually a bad sign.");
        }
        /*
        * 2: Frida Server Detection.
        */
        for (i = 20000; i <= 30000; i++) {
            sock = socket(AF_INET, SOCK_STREAM, 0);
            sa.sin_port = htons(i);
            if (connect(sock, (struct sockaddr *) &sa, sizeof sa) != -1) {
                memset(res, 0, 7);
                send(sock, "\x00", 1, NULL);
                send(sock, "AUTH\r\n", 6, NULL);
                usleep(500); // Give it some time to answer
                if ((ret = recv(sock, res, 6, MSG_DONTWAIT)) != -1) {
                    if (strcmp(res, "REJECT") == 0) {
                        __android_log_print(ANDROID_LOG_VERBOSE, APPNAME,
                                            "FRIDA DETECTED [1] - frida server running on port %d!",
                                            i);
                        if (callBack(env, jcallback, javaCallbackId) == 1)
                            break;
                    }
                }
            }
            close(sock);
        }
        __android_log_print(ANDROID_LOG_VERBOSE, APPNAME, "FRIDA DETECTED CHECKED ～～");
        sleep(3);
    }
}

/**
 * 检测到有Frida，就执行回调通知java层
 * @param env
 * @param jcallback
 * @param javaCallbackId
 */
int callBack(JNIEnv *env, jobject jcallback, jmethodID javaCallbackId) {
    int ret = (*env).CallIntMethod(jcallback, javaCallbackId);/*执行回调*/
    if (ret == 1) {
        mLoop = 0;
        //释放你的全局引用的接口，生命周期自己把控
        (*env).DeleteGlobalRef(jcallback);
        //释放当前线程
        if (mNeedDetach) {
            (*g_VM).DetachCurrentThread();
        }
        env = NULL;
        jcallback = NULL;
        pthread_exit(0);
    }
    return ret;
}

/*
 * public native void init();
 */

extern "C"
JNIEXPORT void JNICALL
Java_com_jni_tool_JNIFridaDetectTool_fridaDetect(JNIEnv *env, jobject clazz, jobject listener) {
    // TODO: implement fridaDetect()
    //JavaVM是虚拟机在JNI中的表示，等下再其他线程回调java层需要用到
    (*env).GetJavaVM(&g_VM);
    //生成一个全局引用，回调的时候findclass才不会为null
    jobject callback = (*env).NewGlobalRef(listener);
    // 把接口传进去，或者保存在一个结构体里面的属性， 进行传递也可以
    pthread_t t;
    pthread_create(&t, NULL, detect_frida_loop, callback);
}