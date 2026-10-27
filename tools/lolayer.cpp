#include <cerrno>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <fcntl.h>
#include <sys/mman.h>
#include <unistd.h>

#include <wayland-client.h>

#include "layer-shell-client-protocol.h"

static wl_compositor *g_compositor = nullptr;
static wl_shm *g_shm = nullptr;
static wl_output *g_output = nullptr;
static zwlr_layer_shell_v1 *g_layerShell = nullptr;

static bool g_configured = false;
static bool g_running = true;
static uint32_t g_confWidth = 0;
static uint32_t g_confHeight = 0;

static void registryAdd(void *data, wl_registry *reg, uint32_t name, const char *iface,
                        uint32_t version)
{
    Q_UNUSED(data)
    if (strcmp(iface, wl_compositor_interface.name) == 0) {
        g_compositor = static_cast<wl_compositor *>(
            wl_registry_bind(reg, name, &wl_compositor_interface, 1));
    } else if (strcmp(iface, wl_shm_interface.name) == 0) {
        g_shm = static_cast<wl_shm *>(wl_registry_bind(reg, name, &wl_shm_interface, 1));
    } else if (strcmp(iface, wl_output_interface.name) == 0) {
        if (!g_output)
            g_output = static_cast<wl_output *>(
                wl_registry_bind(reg, name, &wl_output_interface, version < 2 ? version : 2));
    } else if (strcmp(iface, zwlr_layer_shell_v1_interface.name) == 0) {
        g_layerShell = static_cast<zwlr_layer_shell_v1 *>(wl_registry_bind(
            reg, name, &zwlr_layer_shell_v1_interface, version < 4 ? version : 4));
    }
}

static void registryRemove(void *data, wl_registry *reg, uint32_t name)
{
    Q_UNUSED(data)
    Q_UNUSED(reg)
    Q_UNUSED(name)
}

static const wl_registry_listener g_registryListener = { registryAdd, registryRemove };

static void layerConfigure(void *data, zwlr_layer_surface_v1 *surf, uint32_t serial, uint32_t w,
                           uint32_t h)
{
    Q_UNUSED(data)
    g_confWidth = w;
    g_confHeight = h;
    zwlr_layer_surface_v1_ack_configure(surf, serial);
    g_configured = true;
}

static void layerClosed(void *data, zwlr_layer_surface_v1 *surf)
{
    Q_UNUSED(data)
    Q_UNUSED(surf)
    g_running = false;
}

static const zwlr_layer_surface_v1_listener g_layerListener = { layerConfigure, layerClosed };

static uint32_t blend(uint32_t a, uint32_t b, int t, int maxT)
{
    const int r0 = (a >> 16) & 0xff;
    const int g0 = (a >> 8) & 0xff;
    const int b0 = a & 0xff;
    const int r1 = (b >> 16) & 0xff;
    const int g1 = (b >> 8) & 0xff;
    const int b1 = b & 0xff;
    const uint32_t r = uint32_t(r0 + (r1 - r0) * t / maxT);
    const uint32_t g = uint32_t(g0 + (g1 - g0) * t / maxT);
    const uint32_t bl = uint32_t(b0 + (b1 - b0) * t / maxT);
    return 0xff000000u | (r << 16) | (g << 8) | bl;
}

static void drawPanel(uint32_t *px, uint32_t w, uint32_t h)
{
    const uint32_t top = 0xff241a33;
    const uint32_t bottom = 0xff2f2145;
    const uint32_t accent = 0xffd0bcff;
    const uint32_t chip = 0xff4a4458;
    for (uint32_t y = 0; y < h; ++y) {
        for (uint32_t x = 0; x < w; ++x)
            px[y * w + x] = blend(top, bottom, int(y), int(h));
    }
    for (uint32_t x = 0; x < w; ++x) {
        px[(h - 1) * w + x] = accent;
    }
    for (int c = 0; c < 4; ++c) {
        const uint32_t cx = 46 + uint32_t(c) * 54;
        const uint32_t cy = h / 2;
        for (uint32_t y = 0; y < h; ++y) {
            for (uint32_t x = 0; x < w; ++x) {
                const int dx = int(x) - int(cx);
                const int dy = int(y) - int(cy);
                if (dx * dx + dy * dy <= 100)
                    px[y * w + x] = chip;
                else if (dx * dx + dy * dy <= 121 && px[y * w + x] != chip)
                    px[y * w + x] = 0x40d0bcff | (px[y * w + x] & 0x00ffffff);
            }
        }
    }
    for (uint32_t y = 4; y + 4 < h; ++y) {
        for (uint32_t x = w - 150; x < w - 12; ++x) {
            const int dx = int(x) - int(w - 81);
            const int dy = int(y) - int(h / 2);
            if (dx * dx * 100 + dy * dy * 1600 <= 810000)
                px[y * w + x] = 0xff36343b;
        }
    }
}

int main(int argc, char **argv)
{
    Q_UNUSED(argc)
    Q_UNUSED(argv)
    wl_display *display = wl_display_connect(nullptr);
    if (!display) {
        fprintf(stderr, "lolayer: cannot connect to display\n");
        return 1;
    }
    wl_registry *registry = wl_display_get_registry(display);
    wl_registry_add_listener(registry, &g_registryListener, nullptr);
    wl_display_roundtrip(display);
    wl_display_roundtrip(display);

    if (!g_compositor || !g_shm || !g_layerShell) {
        fprintf(stderr, "lolayer: missing globals (compositor=%p shm=%p layershell=%p)\n",
                (void *)g_compositor, (void *)g_shm, (void *)g_layerShell);
        return 1;
    }

    wl_surface *surface = wl_compositor_create_surface(g_compositor);
    zwlr_layer_surface_v1 *lsurf = zwlr_layer_shell_v1_get_layer_surface(
        g_layerShell, surface, g_output, ZWLR_LAYER_SHELL_V1_LAYER_TOP, "loali-panel");
    zwlr_layer_surface_v1_add_listener(lsurf, &g_layerListener, nullptr);
    zwlr_layer_surface_v1_set_anchor(lsurf, ZWLR_LAYER_SURFACE_V1_ANCHOR_TOP
                                              | ZWLR_LAYER_SURFACE_V1_ANCHOR_LEFT
                                              | ZWLR_LAYER_SURFACE_V1_ANCHOR_RIGHT);
    zwlr_layer_surface_v1_set_size(lsurf, 0, 34);
    zwlr_layer_surface_v1_set_exclusive_zone(lsurf, 34);
    wl_surface_commit(surface);
    wl_display_flush(display);

    while (!g_configured && g_running) {
        if (wl_display_dispatch(display) < 0)
            break;
    }
    if (!g_configured) {
        fprintf(stderr, "lolayer: never configured\n");
        return 1;
    }

    const uint32_t w = g_confWidth ? g_confWidth : 1280;
    const uint32_t h = g_confHeight ? g_confHeight : 34;
    const size_t stride = w * 4;
    const size_t size = stride * h;

    int fd = memfd_create("loali-panel", 0);
    if (fd < 0) {
        const char *path = "/tmp/loali-panel-shm";
        fd = open(path, O_CREAT | O_RDWR | O_CLOEXEC, 0600);
        if (fd >= 0)
            unlink(path);
    }
    if (fd < 0) {
        fprintf(stderr, "lolayer: shm open failed\n");
        return 1;
    }
    if (ftruncate(fd, off_t(size)) != 0) {
        fprintf(stderr, "lolayer: ftruncate failed\n");
        return 1;
    }
    auto *map = static_cast<uint32_t *>(mmap(nullptr, size, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0));
    if (map == MAP_FAILED) {
        fprintf(stderr, "lolayer: mmap failed\n");
        return 1;
    }

    wl_shm_pool *pool = wl_shm_create_pool(g_shm, fd, int32_t(size));
    wl_buffer *buffer = wl_shm_pool_create_buffer(pool, 0, int32_t(w), int32_t(h), int32_t(stride),
                                                  WL_SHM_FORMAT_ARGB8888);
    drawPanel(map, w, h);
    munmap(map, size);

    wl_surface_attach(surface, buffer, 0, 0);
    wl_surface_damage(surface, 0, 0, int32_t(w), int32_t(h));
    wl_surface_commit(surface);
    wl_display_flush(display);

    while (g_running) {
        if (wl_display_dispatch(display) < 0)
            break;
    }

    zwlr_layer_surface_v1_destroy(lsurf);
    wl_surface_destroy(surface);
    wl_buffer_destroy(buffer);
    wl_shm_pool_destroy(pool);
    close(fd);
    wl_display_disconnect(display);
    return 0;
}
