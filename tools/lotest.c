#define _GNU_SOURCE

#include <errno.h>
#include <fcntl.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <unistd.h>

#include <wayland-client.h>

#include "ext-idle-notify-v1-client-protocol.h"
#include "wlr-output-power-management-unstable-v1-client-protocol.h"
#include "ext-session-lock-v1-client-protocol.h"
#include "input-method-unstable-v2-client-protocol.h"
#include "virtual-keyboard-unstable-v1-client-protocol.h"

static struct ext_idle_notifier_v1 *g_idle_mgr = NULL;
static struct zwlr_output_power_manager_v1 *g_power_mgr = NULL;
static struct ext_session_lock_manager_v1 *g_lock_mgr = NULL;
static struct wl_compositor *g_compositor = NULL;
static struct wl_shm *g_shm = NULL;
static struct wl_seat *g_seat = NULL;
static struct wl_output *g_output = NULL;
static int g_dpms_want = -1;
static int g_done = 0;
static int g_locked = 0;
static int g_lock_finished = 0;
static uint32_t g_lock_color = 0xff182848u;
static int g_lock_want_unlock = 0;
static struct wl_shm_pool *g_lock_pool = NULL;
static struct wl_buffer *g_lock_buffer = NULL;
static int g_lock_fd = -1;
static struct zwp_input_method_manager_v2 *g_im_mgr = NULL;
static struct zwp_virtual_keyboard_manager_v1 *g_vk_mgr = NULL;
static int g_im_activated = 0;
static int g_im_deactivated = 0;

static void idle_handle_idled(void *data, struct ext_idle_notification_v1 *n)
{
    (void)data;
    (void)n;
    printf("IDLED\n");
    fflush(stdout);
    g_done = 1;
}

static void idle_handle_resumed(void *data, struct ext_idle_notification_v1 *n)
{
    (void)data;
    (void)n;
    printf("RESUMED\n");
    fflush(stdout);
}

static const struct ext_idle_notification_v1_listener idle_listener = {
    idle_handle_idled,
    idle_handle_resumed,
};

static void power_handle_mode(void *data, struct zwlr_output_power_v1 *p, uint32_t mode)
{
    (void)data;
    (void)p;
    printf("MODE=%s\n", mode == ZWLR_OUTPUT_POWER_V1_MODE_OFF ? "off" : "on");
    fflush(stdout);
    if (g_dpms_want >= 0) {
        zwlr_output_power_v1_set_mode(p, g_dpms_want ? ZWLR_OUTPUT_POWER_V1_MODE_ON
                                                     : ZWLR_OUTPUT_POWER_V1_MODE_OFF);
        g_dpms_want = -1;
        return;
    }
    g_done = 1;
}

static void power_handle_failed(void *data, struct zwlr_output_power_v1 *p)
{
    (void)data;
    (void)p;
    printf("FAILED\n");
    fflush(stdout);
    g_done = 1;
}

static const struct zwlr_output_power_v1_listener power_listener = {
    power_handle_mode,
    power_handle_failed,
};

static void lock_handle_locked(void *data, struct ext_session_lock_v1 *lock)
{
    (void)data;
    (void)lock;
    printf("LOCKED\n");
    fflush(stdout);
    g_locked = 1;
    g_done = 1;
}

static void lock_handle_finished(void *data, struct ext_session_lock_v1 *lock)
{
    (void)data;
    (void)lock;
    printf("FINISHED\n");
    fflush(stdout);
    g_lock_finished = 1;
    g_done = 1;
}

static const struct ext_session_lock_v1_listener lock_listener = {
    lock_handle_locked,
    lock_handle_finished,
};

static void lock_surface_configure(void *data, struct ext_session_lock_surface_v1 *surf,
                                   uint32_t serial, uint32_t width, uint32_t height)
{
    struct wl_surface *surface = (struct wl_surface *)data;
    ext_session_lock_surface_v1_ack_configure(surf, serial);
    const uint32_t w = width ? width : 1280;
    const uint32_t h = height ? height : 800;
    const size_t stride = w * 4;
    const size_t bytes = stride * h;
    int fd = memfd_create("lotest-lock", 0);
    if (fd < 0)
        fd = open("/tmp/lotest-lock-shm", O_CREAT | O_RDWR | O_CLOEXEC, 0600);
    if (fd < 0) {
        fprintf(stderr, "lotest: shm open failed\n");
        return;
    }
    if (ftruncate(fd, (off_t)bytes) != 0) {
        close(fd);
        return;
    }
    uint32_t *map = (uint32_t *)mmap(NULL, bytes, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    if (map == MAP_FAILED) {
        close(fd);
        return;
    }
    for (size_t i = 0; i < bytes / 4; ++i)
        map[i] = g_lock_color;
    munmap(map, bytes);
    struct wl_shm_pool *pool = wl_shm_create_pool(g_shm, fd, (int32_t)bytes);
    struct wl_buffer *buffer = wl_shm_pool_create_buffer(pool, 0, (int32_t)w, (int32_t)h,
                                                         (int32_t)stride, WL_SHM_FORMAT_ARGB8888);
    wl_surface_attach(surface, buffer, 0, 0);
    wl_surface_damage(surface, 0, 0, (int32_t)w, (int32_t)h);
    wl_surface_commit(surface);
    wl_display_flush(wl_proxy_get_display((struct wl_proxy *)surface));
    g_lock_pool = pool;
    g_lock_buffer = buffer;
    g_lock_fd = fd;
    printf("LOCK_SURFACE_READY %ux%u\n", w, h);
    fflush(stdout);
}

static void lock_surface_destroy(void *data, struct ext_session_lock_surface_v1 *surf)
{
    (void)data;
    (void)surf;
}

static const struct ext_session_lock_surface_v1_listener lock_surface_listener = {
    lock_surface_configure,
    lock_surface_destroy,
};

static void im_handle_activate(void *data, struct zwp_input_method_v2 *im)
{
    (void)data;
    (void)im;
    printf("ACTIVATED\n");
    fflush(stdout);
    g_im_activated = 1;
    g_done = 1;
}

static void im_handle_deactivate(void *data, struct zwp_input_method_v2 *im)
{
    (void)data;
    (void)im;
    printf("DEACTIVATED\n");
    fflush(stdout);
    g_im_deactivated = 1;
}

static void im_handle_surrounding_text(void *data, struct zwp_input_method_v2 *im,
                                       const char *text, uint32_t cursor, uint32_t anchor)
{
    (void)data; (void)im; (void)text; (void)cursor; (void)anchor;
}

static void im_handle_text_change_cause(void *data, struct zwp_input_method_v2 *im, uint32_t cause)
{
    (void)data; (void)im; (void)cause;
}

static void im_handle_content_type(void *data, struct zwp_input_method_v2 *im, uint32_t hint,
                                   uint32_t purpose)
{
    (void)data; (void)im; (void)hint; (void)purpose;
}

static void im_handle_done(void *data, struct zwp_input_method_v2 *im)
{
    (void)data;
    (void)im;
}

static void im_handle_unavailable(void *data, struct zwp_input_method_v2 *im)
{
    (void)data;
    (void)im;
    printf("UNAVAILABLE\n");
    fflush(stdout);
    g_done = 1;
}

static const struct zwp_input_method_v2_listener im_listener = {
    im_handle_activate,
    im_handle_deactivate,
    im_handle_surrounding_text,
    im_handle_text_change_cause,
    im_handle_content_type,
    im_handle_done,
    im_handle_unavailable,
};

static void output_handle_geometry(void *data, struct wl_output *o, int32_t x, int32_t y,
                                   int32_t pw, int32_t ph, int32_t sub, const char *make,
                                   const char *model, int32_t transform)
{
    (void)data; (void)o; (void)x; (void)y; (void)pw; (void)ph; (void)sub;
    (void)make; (void)model; (void)transform;
}

static void output_handle_mode(void *data, struct wl_output *o, uint32_t flags, int32_t w,
                               int32_t h, int32_t refresh)
{
    (void)data; (void)o; (void)flags; (void)w; (void)h; (void)refresh;
}

static void output_handle_done(void *data, struct wl_output *o) { (void)data; (void)o; }
static void output_handle_scale(void *data, struct wl_output *o, int32_t s) { (void)data; (void)o; (void)s; }

static const struct wl_output_listener output_listener = {
    output_handle_geometry,
    output_handle_mode,
    output_handle_done,
    output_handle_scale,
};

static void seat_handle_capabilities(void *data, struct wl_seat *s, uint32_t caps)
{
    (void)data;
    (void)s;
    (void)caps;
}

static void seat_handle_name(void *data, struct wl_seat *s, const char *name)
{
    (void)data;
    (void)s;
    (void)name;
}

static const struct wl_seat_listener seat_listener = {
    seat_handle_capabilities,
    seat_handle_name,
};

static void registry_global(void *data, struct wl_registry *reg, uint32_t name,
                            const char *iface, uint32_t version)
{
    if (strcmp(iface, ext_idle_notifier_v1_interface.name) == 0) {
        g_idle_mgr = wl_registry_bind(reg, name, &ext_idle_notifier_v1_interface, 1);
    } else if (strcmp(iface, zwlr_output_power_manager_v1_interface.name) == 0) {
        g_power_mgr = wl_registry_bind(reg, name, &zwlr_output_power_manager_v1_interface, 1);
    } else if (strcmp(iface, ext_session_lock_manager_v1_interface.name) == 0) {
        g_lock_mgr = wl_registry_bind(reg, name, &ext_session_lock_manager_v1_interface, 1);
    } else if (strcmp(iface, zwp_input_method_manager_v2_interface.name) == 0) {
        g_im_mgr = wl_registry_bind(reg, name, &zwp_input_method_manager_v2_interface, 1);
    } else if (strcmp(iface, zwp_virtual_keyboard_manager_v1_interface.name) == 0) {
        g_vk_mgr = wl_registry_bind(reg, name, &zwp_virtual_keyboard_manager_v1_interface, 1);
    } else if (strcmp(iface, wl_compositor_interface.name) == 0 && !g_compositor) {
        g_compositor = wl_registry_bind(reg, name, &wl_compositor_interface, 1);
    } else if (strcmp(iface, wl_shm_interface.name) == 0 && !g_shm) {
        g_shm = wl_registry_bind(reg, name, &wl_shm_interface, 1);
    } else if (strcmp(iface, wl_seat_interface.name) == 0 && !g_seat) {
        g_seat = wl_registry_bind(reg, name, &wl_seat_interface, version < 7 ? version : 7);
        if (g_seat)
            wl_seat_add_listener(g_seat, &seat_listener, NULL);
    } else if (strcmp(iface, wl_output_interface.name) == 0 && !g_output) {
        g_output = wl_registry_bind(reg, name, &wl_output_interface, version < 3 ? version : 3);
        if (g_output)
            wl_output_add_listener(g_output, &output_listener, NULL);
    }
}

static void registry_global_remove(void *data, struct wl_registry *reg, uint32_t name)
{
    (void)data;
    (void)reg;
    (void)name;
}

static const struct wl_registry_listener registry_listener = {
    registry_global,
    registry_global_remove,
};

int main(int argc, char **argv)
{
    if (argc < 2) {
        fprintf(stderr, "usage: lotest idle <ms> | lotest dpms <on|off|get> | lotest lock [hex] [unlock]\n");
        return 1;
    }
    struct wl_display *display = wl_display_connect(NULL);
    if (!display) {
        fprintf(stderr, "no wayland display\n");
        return 1;
    }
    struct wl_registry *reg = wl_display_get_registry(display);
    wl_registry_add_listener(reg, &registry_listener, NULL);
    wl_display_roundtrip(display);
    wl_display_roundtrip(display);

    if (!g_seat) {
        fprintf(stderr, "no wl_seat\n");
        return 1;
    }

    if (strcmp(argv[1], "idle") == 0) {
        uint32_t ms = argc > 2 ? (uint32_t)atoi(argv[2]) : 1500u;
        if (!g_idle_mgr) {
            fprintf(stderr, "no ext_idle_notifier_v1\n");
            return 1;
        }
        struct ext_idle_notification_v1 *n =
            ext_idle_notifier_v1_get_idle_notification(g_idle_mgr, ms, g_seat);
        if (!n) {
            fprintf(stderr, "get_idle_notification failed\n");
            return 1;
        }
        ext_idle_notification_v1_add_listener(n, &idle_listener, NULL);
        while (!g_done && wl_display_dispatch(display) >= 0) {
        }
    } else if (strcmp(argv[1], "dpms") == 0) {
        if (!g_power_mgr) {
            fprintf(stderr, "no zwlr_output_power_manager_v1\n");
            return 1;
        }
        if (!g_output) {
            fprintf(stderr, "no wl_output\n");
            return 1;
        }
        struct zwlr_output_power_v1 *p = zwlr_output_power_manager_v1_get_output_power(
            g_power_mgr, g_output);
        if (!p) {
            fprintf(stderr, "get_output_power failed\n");
            return 1;
        }
        zwlr_output_power_v1_add_listener(p, &power_listener, NULL);
        if (argc > 2 && strcmp(argv[2], "off") == 0)
            g_dpms_want = 0;
        else if (argc > 2 && strcmp(argv[2], "on") == 0)
            g_dpms_want = 1;
        wl_display_roundtrip(display);
        int guard = 0;
        while (!g_done && guard < 30 && wl_display_dispatch(display) >= 0)
            guard++;
        printf("DPMS_SET=%s\n", argc > 2 ? argv[2] : "get");
        fflush(stdout);
    } else if (strcmp(argv[1], "lock") == 0) {
        if (!g_lock_mgr) {
            fprintf(stderr, "no ext_session_lock_manager_v1\n");
            return 1;
        }
        for (int i = 2; i < argc; ++i) {
            if (strcmp(argv[i], "unlock") == 0)
                g_lock_want_unlock = 1;
            else if (argv[i][0] == '0' && argv[i][1] == 'x')
                g_lock_color = (uint32_t)strtoul(argv[i] + 2, NULL, 16) | 0xff000000u;
            else
                g_lock_color = (uint32_t)strtoul(argv[i], NULL, 16) | 0xff000000u;
        }
        struct ext_session_lock_v1 *lock = ext_session_lock_manager_v1_lock(g_lock_mgr);
        if (!lock) {
            fprintf(stderr, "lock failed\n");
            return 1;
        }
        ext_session_lock_v1_add_listener(lock, &lock_listener, NULL);
        struct wl_surface *surface = wl_compositor_create_surface(g_compositor);
        if (!surface) {
            fprintf(stderr, "no compositor surface\n");
            return 1;
        }
        struct ext_session_lock_surface_v1 *lsurf =
            ext_session_lock_v1_get_lock_surface(lock, surface, g_output);
        if (!lsurf) {
            fprintf(stderr, "get_lock_surface failed\n");
            return 1;
        }
        ext_session_lock_surface_v1_add_listener(lsurf, &lock_surface_listener, surface);
        wl_display_roundtrip(display);
        while (!g_locked && !g_lock_finished) {
            if (wl_display_dispatch(display) < 0)
                break;
        }
        if (g_locked && g_lock_want_unlock) {
            ext_session_lock_v1_unlock_and_destroy(lock);
            wl_display_roundtrip(display);
            printf("UNLOCKED\n");
            fflush(stdout);
        } else if (g_locked) {
            while (wl_display_dispatch(display) >= 0) {
            }
        }
        if (g_lock_buffer)
            wl_buffer_destroy(g_lock_buffer);
        if (g_lock_pool)
            wl_shm_pool_destroy(g_lock_pool);
        if (g_lock_fd >= 0)
            close(g_lock_fd);
    } else if (strcmp(argv[1], "im") == 0) {
        if (!g_im_mgr) {
            fprintf(stderr, "no zwp_input_method_manager_v2\n");
            return 1;
        }
        struct zwp_input_method_v2 *im = zwp_input_method_manager_v2_get_input_method(
            g_im_mgr, g_seat);
        if (!im) {
            fprintf(stderr, "get_input_method failed\n");
            return 1;
        }
        zwp_input_method_v2_add_listener(im, &im_listener, NULL);
        wl_display_roundtrip(display);
        int guard = 0;
        while (!g_done && guard < 60 && wl_display_dispatch(display) >= 0)
            guard++;
        if (g_im_activated) {
            zwp_input_method_v2_commit_string(im, "hi");
            zwp_input_method_v2_commit(im, 0);
            wl_display_roundtrip(display);
            printf("COMMIT_SENT\n");
            fflush(stdout);
        }
        if (!g_im_activated) {
            fprintf(stderr, "never activated\n");
            return 1;
        }
    } else if (strcmp(argv[1], "vkey") == 0) {
        if (!g_vk_mgr) {
            fprintf(stderr, "no zwp_virtual_keyboard_manager_v1\n");
            return 1;
        }
        struct zwp_virtual_keyboard_v1 *vk =
            zwp_virtual_keyboard_manager_v1_create_virtual_keyboard(g_vk_mgr, g_seat);
        if (!vk) {
            fprintf(stderr, "create_virtual_keyboard failed\n");
            return 1;
        }
        uint32_t key = argc > 2 ? (uint32_t)atoi(argv[2]) : 30u;
        zwp_virtual_keyboard_v1_key(vk, 100, key, 1);
        zwp_virtual_keyboard_v1_key(vk, 120, key, 0);
        wl_display_roundtrip(display);
        printf("VKEY_SENT=%u\n", key);
        fflush(stdout);
    } else {
        fprintf(stderr, "unknown mode %s\n", argv[1]);
        return 1;
    }
    wl_display_disconnect(display);
    return 0;
}
