/* Mocks for test_fullscreen_restore.py; transition functions come from client.c. */
#include <assert.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>

enum { TILE, SCROLLER, VERTICAL_SCROLLER, MONOCLE, IPC_WATCH_ARRANGGE };
struct wlr_box { int32_t x, y, width, height; };
typedef struct { int interact; } ResizeOpts;
typedef struct { int id; } Layout;
typedef struct { Layout *ltidxs[1]; } Pertag;
struct Client;
typedef struct {
    struct wlr_box m, w;
    struct Client *sel, *prevsel;
    bool isoverview;
    Pertag *pertag;
} Monitor;
typedef struct { bool mapped; } Surface;
typedef struct Client {
    Monitor *mon;
    struct Client *swallowing, *swallowdby;
    Surface surface;
    struct wlr_box geom, float_geom;
    int32_t isfloating, isfullscreen, ismaximizescreen, isfakefullscreen;
    bool fullscreen_was_maximized, iskilling, no_border, force_fakemaximize;
    bool iscustompos, is_in_scratchpad, isnamedscratchpad, group_bar, x11;
    int need_float_size_reduce;
    uint32_t tags, bw;
    float master_inner_per, stack_inner_per, old_master_inner_per, old_stack_inner_per;
    bool protocol_fullscreen, protocol_maximized;
} Client;
static struct { Client *grab_client; Monitor *selected_monitor; int clients; } server;
static struct {
    int borderpx, gappoh, gappov, group_bar_height;
    bool monocle_no_gap;
} config = { 2, 10, 12, 24, false };
static int arrange_count;
#define wl_list_for_each(c, list, link) for ((c) = NULL; (c); (c) = NULL)
#define VISIBLEON(c, m) ((c)->mon == (m))
#define ISFULLSCREEN(c) ((c)->isfullscreen || (c)->ismaximizescreen)
static Surface *client_surface(Client *c) { return &c->surface; }
static bool client_is_x11(Client *c) { return c->x11; }
static bool is_scroller_layout(Monitor *m) {
    return m->pertag->ltidxs[0]->id == SCROLLER ||
           m->pertag->ltidxs[0]->id == VERTICAL_SCROLLER;
}
static bool is_monocle_layout(Monitor *m) { return m->pertag->ltidxs[0]->id == MONOCLE; }
static bool client_wants_group_bar(Client *c) { return c->group_bar; }
static int get_client_tag_idx(Client *c) { return 0; }
static void client_set_fullscreen(Client *c, int value) { c->protocol_fullscreen = value; }
static void client_set_maximized(Client *c, bool value) { c->protocol_maximized = value; }
static void client_pending_fullscreen_state(Client *c, int value) { c->isfullscreen = value; }
static void client_pending_maximized_state(Client *c, int value) { c->ismaximizescreen = value; }
static void resize(Client *c, struct wlr_box box, ResizeOpts opts) { c->geom = box; }
static struct wlr_box client_center_geometry(Client *c, Monitor *m,
                                            struct wlr_box box, int x, int y) { return box; }
static void arrange(Monitor *m, bool a, bool b) { arrange_count++; }
static void exit_scroller_stack(Client *c) {}
static void client_reparent_group(Client *c) {}
static void check_vrr_enable(Client *c) {}
static void check_keep_idle_inhibit(Client *c) {}
static void set_size_per(Monitor *m, Client *c) {}
static void client_sync_tiled_hint(Client *c) {}
static void client_update_border_color(Client *c) {}
static void printstatus(int value) {}
static bool client_is_parked(Client *c) { return false; }
static bool client_is_x11_popup(Client *c) { return false; }
static void reset_foreign_tolevel(Client *c, Monitor *old, Monitor *m) {}
static void client_reset_mon_tags(Client *c, Monitor *m, uint32_t tags) {}
static void check_match_tag_floating_rule(Client *c, Monitor *m) {}
static Client *client_focus_top(Monitor *m) { return m->sel; }
static void client_focus(Client *c, int value) {}
void client_set_floating(Client *c, int32_t value);
void client_apply_fullscreen(Client *c, int32_t value, bool rearrange);
void clear_fullscreen_flag(Client *c);
void client_set_maximize_screen(Client *c, int32_t value, bool rearrange);
/* TRANSITION_FUNCTIONS */

static Layout layout = { TILE };
static Pertag pertag = { { &layout } };
static Monitor monitor = {
    .m = { 0, 0, 1920, 1080 }, .w = { 0, 30, 1920, 1050 }, .pertag = &pertag
};
static Client fresh(void) {
    layout.id = TILE;
    config.monocle_no_gap = false;
    monitor.isoverview = false;
    arrange_count = 0;
    return (Client) { .mon = &monitor, .surface.mapped = true, .bw = 2,
                     .geom = { 0, 30, 960, 1050 }, .float_geom = { 100, 100, 640, 480 } };
}
static void enter_maximized_fullscreen(Client *c) {
    client_set_maximize_screen(c, 1, true);
    client_apply_fullscreen(c, 1, true);
    assert(c->isfullscreen && !c->ismaximizescreen && c->fullscreen_was_maximized);
    assert(c->protocol_fullscreen && !c->protocol_maximized && c->bw == 0);
}
static void check_restored(Client *c) {
    assert(!c->isfullscreen && c->ismaximizescreen && !c->fullscreen_was_maximized);
    assert(!c->protocol_fullscreen && c->bw == (uint32_t)(c->no_border ? 0 : config.borderpx));
    if (!c->force_fakemaximize)
        assert(c->protocol_maximized);
    if (!is_scroller_layout(c->mon) || c->isfloating) {
        int gapx = config.monocle_no_gap && is_monocle_layout(c->mon) ? 0 : config.gappoh;
        int gapy = config.monocle_no_gap && is_monocle_layout(c->mon) ? 0 : config.gappov;
        int bar = c->group_bar ? config.group_bar_height : 0;
        assert(c->geom.x == monitor.w.x + gapx);
        assert(c->geom.y == monitor.w.y + gapy + bar);
        assert(c->geom.width == monitor.w.width - 2 * gapx);
        assert(c->geom.height == monitor.w.height - 2 * gapy - bar);
    }
}
int main(void) {
    /* Repeat cycles for tiled/floating, borders, group bars and fake maximize. */
    for (int bits = 0; bits < 32; bits++) {
        Client c = fresh();
        c.isfloating = bits & 1;
        c.no_border = bits & 2;
        c.group_bar = bits & 4;
        c.force_fakemaximize = bits & 8;
        c.x11 = bits & 16;
        for (int cycle = 0; cycle < 3; cycle++) {
            enter_maximized_fullscreen(&c);
            client_apply_fullscreen(&c, 1, true); /* Duplicate entry. */
            int before = arrange_count;
            client_apply_fullscreen(&c, 0, true);
            assert(arrange_count == before + 1);
            check_restored(&c);
            client_apply_fullscreen(&c, 0, true); /* Duplicate exit. */
            check_restored(&c);
        }
    }
    for (int id = TILE; id <= MONOCLE; id++) {
        Client c = fresh();
        layout.id = id;
        config.monocle_no_gap = true;
        enter_maximized_fullscreen(&c);
        int before = arrange_count;
        client_apply_fullscreen(&c, 0, false);
        assert(arrange_count == before);
        check_restored(&c);
    }
    Client c = fresh();
    client_apply_fullscreen(&c, 1, true);
    client_apply_fullscreen(&c, 0, true);
    assert(!c.ismaximizescreen); /* Ordinary fullscreen must not maximize. */

    c = fresh();
    enter_maximized_fullscreen(&c);
    client_set_maximize_screen(&c, 0, true); /* Explicit override. */
    client_apply_fullscreen(&c, 0, true);
    assert(!c.ismaximizescreen && !c.fullscreen_was_maximized);

    c = fresh();
    enter_maximized_fullscreen(&c);
    client_set_maximize_screen(&c, 1, true); /* Explicit maximize exits fullscreen. */
    assert(!c.isfullscreen && !c.fullscreen_was_maximized);
    client_set_maximize_screen(&c, 0, true);
    client_apply_fullscreen(&c, 0, true);
    assert(!c.ismaximizescreen);

    c = fresh();
    enter_maximized_fullscreen(&c);
    client_set_floating(&c, 1);
    client_apply_fullscreen(&c, 0, true);
    assert(c.isfloating && !c.ismaximizescreen && !c.fullscreen_was_maximized);
    assert(c.geom.width == 640 && c.geom.height == 480);

    c = fresh();
    enter_maximized_fullscreen(&c);
    clear_fullscreen_flag(&c); /* Layout/focus action intentionally clears both. */
    assert(!c.isfullscreen && !c.ismaximizescreen && !c.fullscreen_was_maximized);

    c = fresh();
    enter_maximized_fullscreen(&c);
    monitor.isoverview = true;
    c.isfullscreen = 0; /* Overview temporarily clears the active flag. */
    client_apply_fullscreen(&c, 0, false);
    assert(c.fullscreen_was_maximized); /* Overview guard preserves saved state. */
    monitor.isoverview = false;
    c.isfullscreen = 1; /* overview_restore reinstates the flag before applying it. */
    client_apply_fullscreen(&c, 1, false);
    client_apply_fullscreen(&c, 0, true);
    check_restored(&c);

    c = fresh();
    enter_maximized_fullscreen(&c);
    client_set_fake_fullscreen(&c, 1);
    assert(c.ismaximizescreen && !c.isfullscreen && c.isfakefullscreen);
    for (int floating = 0; floating <= 1; floating++) {
        c = fresh();
        c.isfloating = floating;
        enter_maximized_fullscreen(&c);
        Monitor other = monitor;
        client_set_monitor(&c, &other, 1, false);
        assert(c.isfullscreen && c.fullscreen_was_maximized);
        client_apply_fullscreen(&c, 0, true);
        check_restored(&c);

        Client parent = fresh();
        parent.isfloating = floating;
        Client child = fresh();
        child.isfloating = floating;
        enter_maximized_fullscreen(&child);
        /* client_replace transfers these fields before unmap synchronizes them. */
        parent.isfullscreen = child.isfullscreen;
        parent.fullscreen_was_maximized = child.fullscreen_was_maximized;
        child.swallowing = &parent;
        sync_swallow(&child);
        assert(parent.isfullscreen && parent.fullscreen_was_maximized);
        client_apply_fullscreen(&parent, 0, true);
        check_restored(&parent);
    }
    c = fresh();
    layout.id = SCROLLER;
    enter_maximized_fullscreen(&c);
    clear_stack_head(&c);
    assert(!c.isfullscreen && !c.ismaximizescreen && !c.fullscreen_was_maximized);
    puts("Fullscreen restoration regression tests passed.");
    return 0;
}
