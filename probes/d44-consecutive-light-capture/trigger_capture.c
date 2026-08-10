#define _GNU_SOURCE

#include <dlfcn.h>
#include <errno.h>
#include <pthread.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <time.h>
#include <unistd.h>

#include "renderdoc_app.h"

#define DEFAULT_FRAME_COUNT 3u
#define POLL_NANOSECONDS 100000000L
#define DEFAULT_TARGET_COMM "IL2Series.exe"

static uint32_t frame_count(void)
{
    const char *value = getenv("IL2_RENDERDOC_FRAME_COUNT");
    char *end = NULL;
    unsigned long parsed;

    if (!value || !*value)
        return DEFAULT_FRAME_COUNT;

    errno = 0;
    parsed = strtoul(value, &end, 10);
    if (errno || !end || *end || parsed < 2 || parsed > 8)
        return DEFAULT_FRAME_COUNT;

    return (uint32_t)parsed;
}

static int wait_for_trigger(const char *path)
{
    struct timespec delay = {0, POLL_NANOSECONDS};

    while (access(path, F_OK) != 0)
        nanosleep(&delay, NULL);

    return unlink(path);
}

static void wait_for_target_process(void)
{
    const char *target = getenv("IL2_RENDERDOC_TARGET_COMM");
    struct timespec delay = {0, POLL_NANOSECONDS};
    char comm[64];
    FILE *file;

    if (!target || !*target)
        target = DEFAULT_TARGET_COMM;

    for (;;)
    {
        file = fopen("/proc/self/comm", "r");
        if (file)
        {
            if (fgets(comm, sizeof(comm), file))
            {
                comm[strcspn(comm, "\r\n")] = '\0';
                if (strcmp(comm, target) == 0)
                {
                    fclose(file);
                    return;
                }
            }
            fclose(file);
        }
        nanosleep(&delay, NULL);
    }
}

static void *trigger_capture(void *unused)
{
    RENDERDOC_API_1_7_0 *api = NULL;
    pRENDERDOC_GetAPI get_api;
    const char *capture_path;
    const char *trigger_path;
    void *module;

    (void)unused;
    trigger_path = getenv("IL2_RENDERDOC_TRIGGER_FILE");
    if (!trigger_path || !*trigger_path)
        return NULL;

    wait_for_target_process();
    if (wait_for_trigger(trigger_path) != 0)
        return NULL;

    module = dlopen("librenderdoc.so", RTLD_NOW | RTLD_NOLOAD);
    if (!module)
        return NULL;

    get_api = (pRENDERDOC_GetAPI)dlsym(module, "RENDERDOC_GetAPI");
    if (!get_api || !get_api(eRENDERDOC_API_Version_1_7_0, (void **)&api))
        return NULL;

    capture_path = getenv("IL2_RENDERDOC_CAPTURE_PATH");
    if (capture_path && *capture_path)
        api->SetCaptureFilePathTemplate(capture_path);

    api->TriggerMultiFrameCapture(frame_count());
    return NULL;
}

__attribute__((constructor)) static void start_trigger_thread(void)
{
    pthread_t thread;

    if (pthread_create(&thread, NULL, trigger_capture, NULL) == 0)
        pthread_detach(thread);
}
