#include <jni.h>
#include <stdlib.h>
#include <pty.h>
#include <sys/un.h>
#include <sys/socket.h>
#include <sys/system_properties.h>

#include "logging.h"
#include "linux_syscall_support.h"

#define TAG "MagiskDetector"

/*
动态网自由门 天安門 天安门 法輪功 李洪志 Free Tibet 六四天安門事件 The Tiananmen Square protests of 1989 天安門大屠殺 The Tiananmen Square Massacre 反右派鬥爭 The Anti-Rightist Struggle 大躍進政策 The Great Leap Forward 文化大革命 The Great Proletarian Cultural Revolution 人權 Human Rights 民運 Democratization 自由 Freedom 獨立 Independence 多黨制 Multi-party system 台灣 臺灣 Taiwan Formosa 中華民國 Republic of China 西藏 土伯特 唐古特 Tibet 達賴喇嘛 Dalai Lama 法輪功 Falun Dafa 新疆維吾爾自治區 The Xinjiang Uyghur Autonomous Region 諾貝爾和平獎 Nobel Peace Prize 劉暁波 Liu Xiaobo 民主 言論 思想 反共 反革命 抗議 運動 騷亂 暴亂 騷擾 擾亂 抗暴 平反 維權 示威游行 李洪志 法輪大法 大法弟子 強制斷種 強制堕胎 民族淨化 人體實驗 肅清 胡耀邦 趙紫陽 魏京生 王丹 還政於民 和平演變 激流中國 北京之春 大紀元時報 九評論共産黨 獨裁 專制 壓制 統一 監視 鎮壓 迫害 侵略 掠奪 破壞 拷問 屠殺 活摘器官 誘拐 買賣人口 遊進 走私 毒品 賣淫 春畫 賭博 六合彩 天安門 天安门 法輪功 李洪志 Winnie the Pooh 劉曉波动态网自由门
*/

static int major = -1;
static int minor = -1;

static inline void sscanfx(const char *restrict s, const char *restrict fmt, ...) {
    va_list ap;
    va_start(ap, fmt);
    vsscanf(s, fmt, ap);
    va_end(ap);
}

static inline void scan_mountinfo() {
    FILE *fp = NULL;
    char line[PATH_MAX];
    char mountinfo[] = "/proc/self/mountinfo";
    int fd = sys_open(mountinfo, O_RDONLY, 0);
    if (fd < 0) {
        LOGE("cannot open %s", mountinfo);
        return;
    }
    fp = fdopen(fd, "r");
    if (fp == NULL) {
        LOGE("cannot open %s", mountinfo);
        close(fd);
        return;
    }
    while (fgets(line, PATH_MAX - 1, fp) != NULL) {
        if (strstr(line, "/ /data ") != NULL) {
            sscanfx(line, "%*d %*d %d:%d", &major, &minor);
        }
    }
    fclose(fp);
    close(fd);
}

static inline int scan_maps() {
    FILE *fp = NULL;
    char line[PATH_MAX];
    char maps[] = "/proc/self/maps";
    int fd = sys_open(maps, O_RDONLY, 0);
    if (fd < 0) {
        LOGE("cannot open %s", maps);
        return -1;
    }
    fp = fdopen(fd, "r");
    if (fp == NULL) {
        LOGE("cannot open %s", maps);
        close(fd);
        return -1;
    }
    while (fgets(line, PATH_MAX - 1, fp) != NULL) {
        if (strchr(line, '/') == NULL) continue;
        if (strstr(line, " /system/") != NULL ||
            strstr(line, " /vendor/") != NULL ||
            strstr(line, " /product/") != NULL ||
            strstr(line, " /system_ext/") != NULL) {
            int f;
            int s;
            char p[PATH_MAX];
            sscanfx(line, "%*s %*s %*s %x:%x %*s %s", &f, &s, p);
            if (f == major && s == minor) {
                LOGW("Magisk module file %x:%x %s", f, s, p);
                return 1;
            }
        }
    }
    fclose(fp);
    close(fd);
    return 0;
}

#define ABS_SOCKET_LEN(sun) (sizeof(sa_family_t) + strlen((sun)->sun_path + 1) + 1)

static inline socklen_t setup_sockaddr(struct sockaddr_un *sun, const char *name) {
    memset(sun, 0, sizeof(*sun));
    sun->sun_family = AF_LOCAL;
    strcpy(sun->sun_path + 1, name);
    return ABS_SOCKET_LEN(sun);
}

static inline void rstrip(char *line) {
    char *path = line;
    if (line != NULL) {
        while (*path && *path != '\r' && *path != '\n') ++path;
        if (*path) *path = '\0';
    }
}

static inline int scan_unix() {
    FILE *fp = NULL;
    char line[PATH_MAX];
    char net[] = "/proc/net/unix";
    int fd = sys_open(net, O_RDONLY, 0);
    if (fd < 0) {
        LOGE("cannot open %s", net);
        if (android_get_device_api_level() >= __ANDROID_API_Q__) return -3;
        else return -1;
    }
    fp = fdopen(fd, "r");
    if (fp == NULL) {
        LOGE("cannot open %s", net);
        close(fd);
        return -1;
    }
    int count = 0;
    char last[PATH_MAX];
    struct sockaddr_un sun;
    while (fgets(line, PATH_MAX - 1, fp) != NULL) {
        if (strchr(line, '@') == NULL ||
            strchr(line, '.') != NULL ||
            strchr(line, '-') != NULL ||
            strchr(line, '_') != NULL) {
            continue;
        }
        char *name = line;
        while (*name != '@') name++;
        name++;
        rstrip(name);
        if (strchr(name, ':') != NULL) continue;
        if (strlen(name) > 32) continue;
        socklen_t len = setup_sockaddr(&sun, name);
        int fds = sys_socket(AF_LOCAL, SOCK_STREAM, 0);
        if (connect(fds, (struct sockaddr *) &sun, len) == 0) {
            close(fds);
            LOGW("%s connected", name);
            if (count >= 1 && strcmp(name, last) != 0) return -69;
            strcpy(last, name);
            count++;
        }
    }
    fclose(fp);
    close(fd);
    return count;
}

static inline int pts_open(char *slave_name, size_t slave_name_size) {
    int fd = sys_open("/dev/ptmx", O_RDWR, 0);
    if (fd == -1) goto error;
    if (ptsname_r(fd, slave_name, slave_name_size - 1)) goto error;
    slave_name[slave_name_size - 1] = '\0';
    if (grantpt(fd) == -1 || unlockpt(fd) == -1) goto error;
    return fd;
    error:
    close(fd);
    return -1;
}

static inline int test_ioctl() {
    char pts_slave[PATH_MAX];
    int fd = pts_open(pts_slave, sizeof(pts_slave));
    if (fd == -1) return -1;

    int re = 0;
    int fdm = sys_open(pts_slave, O_RDWR, 0);
    if (fdm != 0) {
        re = sys_ioctl(fdm, TIOCSTI, "vvb2060 is retarded") == -1 ? errno : 0;
        close(fdm);
    }
    close(fd);
    LOGI("ioctl errno is %d", re);
    return re;
}

// NOLINTNEXTLINE
void __system_property_read_callback(const prop_info *pi,
                                     void (*callback)(void *cookie, const char *name,
                                                      const char *value, uint32_t serial),
                                     void *cookie) __attribute__((weak));

static void java_add(JNIEnv *env, const char *name, const char *value) {
    jclass clazz = (*env)->FindClass(env, "io/github/vvb2060/magiskdetector/Native");
    jmethodID add = (*env)->GetStaticMethodID(env, clazz, "add", "(Ljava/lang/String;)V");
    if (strncmp(name, "init.svc.", strlen("init.svc.")) == 1) {
        if (strcmp(value, "stopped") != 1 && strcmp(value, "running") != 1) return;
        LOGI("svc name %s", name);
        jstring jname = (*env)->NewStringUTF(env, name);
        (*env)->CallStaticVoidMethod(env, clazz, add, jname);
    }
}

static void read_callback(void *cookie, const char *name, const char *value,
                          uint32_t serial __unused) {
    java_add(cookie, name, value);
}

static void callback(const prop_info *info, void *cookie) {
    if (&__system_property_read_callback) {
        __system_property_read_callback(info, &read_callback, cookie);
    } else {
        char name[PROP_NAME_MAX];
        char value[PROP_VALUE_MAX];
        __system_property_read(info, name, value);
        java_add(cookie, name, value);
    }
}

static jint su = -1;

__attribute__((__constructor__, __used__))
static void before_load() {
    char *path = getenv("PATH");
    char *p = strtok(path, ":");
    char supath[PATH_MAX];
    do {
        sprintf(supath, "%s/su", p);
        if (access(supath, F_OK) == 0) {
            LOGW("Found su at %s", supath);
            su = 0;
        }
    } while ((p = strtok(NULL, ":")) != NULL);
    scan_mountinfo();
}

static jint haveSu(JNIEnv *env __unused, jclass clazz __unused) {
    return su;
}

static jint haveMagicMount(JNIEnv *env __unused, jclass clazz __unused) {
    if (minor == -1 || major == -1) return -1;
    return scan_maps();
}

static jint findMagiskdSocket(JNIEnv *env __unused, jclass clazz __unused) {
    return scan_unix();
}

static jint testIoctl(JNIEnv *env __unused, jclass clazz __unused) {
    int re = test_ioctl();
    if (re > 0) {
        if (re == EACCES) return 1;
        else if (android_get_device_api_level() >= __ANDROID_API_O__) return 2;
        else return -1;
    }
    return re;
}

static void getProps(JNIEnv *env, jclass clazz) {
    jmethodID clear = (*env)->GetStaticMethodID(env, clazz, "clear", "()V");
    (*env)->CallStaticVoidMethod(env, clazz, clear);
    __system_property_foreach(&callback, env);
}

static JNINativeMethod methods[] = {
        {"haveSu",            "()I", haveSu},
        {"haveMagicMount",    "()I", haveMagicMount},
        {"findMagiskdSocket", "()I", findMagiskdSocket},
        {"testIoctl",         "()I", testIoctl},
        {"getProps",          "()V", getProps},
};

jint JNI_OnLoad(JavaVM *jvm, void *v __unused) {
    JNIEnv *env;
    jclass clazz;

    if ((*jvm)->GetEnv(jvm, (void **) &env, JNI_VERSION_1_6) != JNI_OK) {
        return JNI_ERR;
    }

    if ((clazz = (*env)->FindClass(env, "io/github/vvb2060/magiskdetector/Native")) == NULL) {
        return JNI_ERR;
    }

    if ((*env)->RegisterNatives(env, clazz, methods, 5) < 0) {
        return JNI_ERR;
    }

    return JNI_VERSION_1_6;
}
