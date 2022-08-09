#include <jni.h>
#include <stdlib.h>
#include <sys/sysmacros.h>
#include <sys/system_properties.h>

#include "logging.h"
#include "linux_syscall_support.h"

#define TAG "MagiskDetector"

static dev_t scan_mountinfo() {
    int major = 0;
    int minor = 0;
    char line[PATH_MAX];
    char mountinfo[] = "/proc/self/mountinfo";
    int fd = sys_open(mountinfo, O_RDONLY, 0);
    if (fd < 0) {
        LOGE("cannot open %s", mountinfo);
        return 0;
    }
    FILE *fp = fdopen(fd, "r");
    if (fp == NULL) {
        LOGE("cannot open %s", mountinfo);
        close(fd);
        return 0;
    }
    while (fgets(line, PATH_MAX - 1, fp) != NULL) {
        if (strstr(line, "/ /data ") != NULL) {
            sscanf(line, "%*d %*d %d:%d", &major, &minor);
        }
    }
    fclose(fp);
    return makedev(major, minor);
}

static int scan_maps(dev_t data_dev) {
    int module = 0;
    char line[PATH_MAX];
    char maps[] = "/proc/self/maps";
    int fd = sys_open(maps, O_RDONLY, 0);
    if (fd < 0) {
        LOGE("cannot open %s", maps);
        return -1;
    }
    FILE *fp = fdopen(fd, "r");
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
            sscanf(line, "%*s %*s %*s %x:%x %*s %s", &f, &s, p);
            if (makedev(f, s) == data_dev) {
                LOGW("Magisk module file %x:%x %s", f, s, p);
                module++;
            }
        }
    }
    fclose(fp);
    return module;
}

static int scan_status() {
    int pid = -1;
    char line[PATH_MAX];
    char maps[] = "/proc/self/status";
    int fd = sys_open(maps, O_RDONLY, 0);
    if (fd < 0) {
        LOGE("cannot open %s", maps);
        return -1;
    }
    FILE *fp = fdopen(fd, "r");
    if (fp == NULL) {
        LOGE("cannot open %s", maps);
        close(fd);
        return -1;
    }
    while (fgets(line, PATH_MAX - 1, fp) != NULL) {
        if (strncmp(line, "TracerPid", 9) == 0) {
            pid = atoi(&line[10]);
            break;
        }
    }
    fclose(fp);
    return pid;
}

static int scan_path() {
    char *path = getenv("PATH");
    char *p = strtok(path, ":");
    char supath[PATH_MAX];
    do {
        sprintf(supath, "%s/su", p);
        if (access(supath, F_OK) == 0) {
            LOGW("Found su at %s", supath);
            return 1;
        }
    } while ((p = strtok(NULL, ":")) != NULL);
    return 0;
}

static int su = -1;
static int magiskhide = -1;

static jint haveSu(JNIEnv *env __unused, jclass clazz __unused) {
    return su;
}

static jint haveMagiskHide(JNIEnv *env __unused, jclass clazz __unused) {
    return magiskhide;
}

static jint haveMagicMount(JNIEnv *env __unused, jclass clazz __unused) {
    dev_t data_dev = scan_mountinfo();
    if (data_dev == 0) return -1;
    return scan_maps(data_dev);
}

jint JNI_OnLoad(JavaVM *jvm, void *v __unused) {
    su = scan_path();
    magiskhide = scan_status();

    JNIEnv *env;
    jclass clazz;

    if ((*jvm)->GetEnv(jvm, (void **) &env, JNI_VERSION_1_6) != JNI_OK) {
        return JNI_ERR;
    }

    if ((clazz = (*env)->FindClass(env, "io/github/vvb2060/magiskdetector/RemoteService")) ==
        NULL) {
        return JNI_ERR;
    }

    JNINativeMethod methods[] = {
            {"haveSu",         "()I", haveSu},
            {"haveMagiskHide", "()I", haveMagiskHide},
            {"haveMagicMount", "()I", haveMagicMount},
    };

    if ((*env)->RegisterNatives(env, clazz, methods, 3) < 0) {
        return JNI_ERR;
    }

    return JNI_VERSION_1_6;
}
