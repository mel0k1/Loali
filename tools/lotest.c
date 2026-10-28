#include <errno.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <wayland-client.h>

#include "ext-idle-notify-v1-client-protocol.h"
#include "wlr-output-power-management-unstable-v1-client-protocol.h"

static struct ext_idle_notifier_v1 *g_idle_mgr = NULL;
static struct zwlr_output_power_manager_v1 *g_power_mgr = NULL;
static struct wl_seat *g_seat = NULL;
static struct wl_output *g_output = NULL;
static int g_dpms_want = -1;
static int g_done = 0;

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
        fprintf(stderr, "usage: lotest idle <ms> | lotest dpms <on|off|get>\n");
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
    } else {
        fprintf(stderr, "unknown mode %s\n", argv[1]);
        return 1;
    }
    wl_display_disconnect(display);
    return 0;
}
