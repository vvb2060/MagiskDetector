#include <string.h>
#include <stdlib.h>
#include <malloc.h>
#include <jni.h>
#include <sys/un.h>
#include <sys/socket.h>
#include <sys/system_properties.h>

#include "logging.h"
#include "linux_syscall_support.h"

#define TAG "MagiskDetector"

int major = -1;
int minor = -1;

static inline void scanMountinfo() {
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
            sscanf(line, "%*d %*d %d:%d", &major, &minor);
        }
    }
    fclose(fp);
    close(fd);
}

static inline jint scanMaps() {
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
    int f;
    int s;
    while (fgets(line, PATH_MAX - 1, fp) != NULL) {
        if (strchr(line, '/') == NULL) continue;
        if (strstr(line, "/system/") != NULL || strstr(line, "/vendor/") != NULL) {
            sscanf(line, "%*s %*s %*s %x:%x", &f, &s);
            if ((f == major && s == minor) || f == 7) return 1;
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

static inline int connectMagiskd(const char *name) {
    struct sockaddr_un sun;
    socklen_t len = setup_sockaddr(&sun, name);
    int fd = sys_socket(AF_LOCAL, (unsigned) SOCK_STREAM | (unsigned) SOCK_CLOEXEC, 0);
    return connect(fd, (struct sockaddr *) &sun, len);
}

static inline void rstrip(char *line) {
    char *path = line;
    if (line != NULL) {
        while (*path && *path != '\r' && *path != '\n') ++path;
        if (*path) *path = '\0';
    }
}

static inline jint scanNet() {
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
    while (fgets(line, PATH_MAX - 1, fp) != NULL) {
        if (strchr(line, '@') == NULL) continue;
        if (strchr(line, '.') != NULL || strchr(line, '-') != NULL) continue;
        char *name = line;
        while (*name != '@') name++;
        name++;
        rstrip(name);
        if (strchr(name, ':') != NULL) continue;
        if (connectMagiskd(name) == 0) {
            LOGW("%s connected", name);
            if (strcmp(name, "time_genoff") == 0) return -2;
            else count++;
        }
    }
    fclose(fp);
    close(fd);
    return count;
}

jint su = -1;

__attribute__((constructor))
static void beforeLoad() {
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
    scanMountinfo();
}

static jint haveSu(JNIEnv *env __unused, jclass clazz __unused) {
    return su;
}

static jint haveMagicMount(JNIEnv *env __unused, jclass clazz __unused) {
    if (minor == -1 || major == -1) return -1;
    return scanMaps();
}

static jint findMagiskdSocket(JNIEnv *env __unused, jclass clazz __unused) {
    return scanNet();
}

static JNINativeMethod methods[] = {
        {"haveSu",            "()I", haveSu},
        {"haveMagicMount",    "()I", haveMagicMount},
        {"findMagiskdSocket", "()I", findMagiskdSocket},
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

    if ((*env)->RegisterNatives(env, clazz, methods, 3) < 0) {
        return JNI_ERR;
    }

    return JNI_VERSION_1_6;
}
