/*
 * A test for gravity handling.
 *
 * Copyright © 2017 Uli Schlachter <psychon@znc.in>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 *
 *
 * Loosely based on test-gravity.c from metacity, which does not have a
 * copyright or license header, but Metacity itself is licensed under GPLv2.
 */

#include <xcb/xcb.h>
#include <xcb/xcb_aux.h>
#include <xcb/xcb_icccm.h>
#include <stdbool.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/*
 * This program tests various gravity-related things. For each possible gravity,
 * this does:
 * - Create a window with this gravity.
 * [Wait for expose event]
 * - Check that the window is mapped in the correct place.
 * - Resize the window.
 * [Wait for real configure notify]
 * - Check that the window moves as expected by the gravity.
 * - Move and resize the window (go back to the original size).
 * [Wait for real configure notify]
 * - Check that the window moves as expected by the gravity.
 * - Move the window.
 * [Wait for synthetic configure notify]
 * - Check that the window moves to the expected position.
 * - Unmap the window
 * [Wait for WM_STATE property notify]
 * - Map the window
 * [Wait for expose event]
 * - Check that the window is mapped in the correct place.
 *
 * This needs a WM that supports _NET_FRAME_EXTENTS.
 * Also, this assumes that the WM allows and applies window moves and resizes by
 * the application.
 */
enum test_state {
    TEST_STATE_CREATED,
    TEST_STATE_RESIZED1,
    TEST_STATE_RESIZED2,
    TEST_STATE_MOVED,
    TEST_STATE_UNMAPPED,
    TEST_STATE_MAPPED,
    TEST_STATE_DONE,
    TEST_STATE_DONE_GOT_CONFIGURE,
};

#define WIN_POS_X1 50
#define WIN_POS_Y1 70
#define WIN_WIDTH1 30
#define WIN_HEIGHT1 40

#define WIN_POS_X2 42
#define WIN_POS_Y2 43
#define WIN_WIDTH2 12
#define WIN_HEIGHT2 16

static int32_t state_difference[][2] = {
    [TEST_STATE_CREATED] = { 0, 0 },
    [TEST_STATE_UNMAPPED] = { 0, 0 },
    [TEST_STATE_MAPPED] = { 0, 0 },
    [TEST_STATE_RESIZED1] = { WIN_WIDTH1 - WIN_WIDTH2, WIN_HEIGHT1 - WIN_HEIGHT2 },
    [TEST_STATE_RESIZED2] = { 0, 0 },
    [TEST_STATE_MOVED] = { 0, 0 },
    [TEST_STATE_DONE] = { 0, 0 },
    [TEST_STATE_DONE_GOT_CONFIGURE] = { 0, 0 },
};

struct window_state {
    xcb_window_t window;
    enum test_state state;
    xcb_gravity_t gravity;
    bool sent_delayed_proceed;
};

static xcb_connection_t *c = NULL;
static xcb_screen_t *screen;
static struct window_state window_state;
static xcb_window_t last_window = XCB_NONE;
static xcb_atom_t net_frame_extents;
static xcb_atom_t wm_state;
static bool done;
static bool had_error = false;

static void check(bool cond, const char *format, ...)
    __attribute__ ((format(printf, 2, 3)));
static void do_log(const char *format, ...)
    __attribute__ ((format(printf, 1, 2)));

static void check(bool cond, const char *format, ...)
{
    va_list ap;

    if (cond)
        return;

    va_start(ap, format);
    fprintf(stderr, "ERROR: ");
    vfprintf(stderr, format, ap);
    fprintf(stderr, "\n");
    va_end(ap);

    if (c) {
        xcb_no_operation(c);
        free(xcb_get_input_focus_reply(c, xcb_get_input_focus(c), NULL));
    }
    had_error = true;
}

static void do_log(const char *format, ...)
{
    va_list ap;

    va_start(ap, format);
    fprintf(stdout, "LOG: ");
    vfprintf(stdout, format, ap);
    fprintf(stdout, "\n");
    va_end(ap);
}

static void query_frame_extents(xcb_window_t win, uint32_t *left,
        uint32_t *right, uint32_t *top, uint32_t *bottom)
{
    xcb_get_property_reply_t *reply = xcb_get_property_reply(c,
            xcb_get_property(c, 0, win, net_frame_extents, XCB_ATOM_CARDINAL,
                0, 4), NULL);

    *left = 0;
    *right = 0;
    *top = 0;
    *bottom = 0;

    if (reply && reply->length == 4)
    {
        uint32_t *data = (uint32_t *) xcb_get_property_value(reply);
        *left = data[0];
        *right = data[1];
        *top = data[2];
        *bottom = data[3];
    }

    free(reply);
}

static void delayed_proceed_with_window(void)
{
    xcb_client_message_event_t ev;

    if (window_state.sent_delayed_proceed)
        return;

    memset(&ev, 0, sizeof(ev));
    ev.response_type = XCB_CLIENT_MESSAGE;
    ev.window = window_state.window;
    ev.format = 32;
    ev.type = XCB_ATOM_NOTICE;
    ev.data.data32[0] = (int) window_state.state;

    xcb_send_event(c, 0, window_state.window, XCB_EVENT_MASK_NO_EVENT, (char *) &ev);
    window_state.sent_delayed_proceed = true;
}

static void get_geometry(xcb_window_t win, int32_t *x, int32_t *y,
        uint32_t *width, uint32_t *height, uint32_t *left, uint32_t *right,
        uint32_t *top, uint32_t *bottom)
{
    xcb_get_geometry_cookie_t geo_c = xcb_get_geometry_unchecked(c, win);
    xcb_translate_coordinates_cookie_t coord_c =
        xcb_translate_coordinates(c, win, screen->root, 0, 0);

    query_frame_extents(win, left, right, top, bottom);

    xcb_get_geometry_reply_t *geo = xcb_get_geometry_reply(c, geo_c, NULL);
    xcb_translate_coordinates_reply_t *coord =
        xcb_translate_coordinates_reply(c, coord_c, NULL);

    *x = coord->dst_x;
    *y = coord->dst_y;
    *width = geo->width;
    *height = geo->height;

    free(coord);
    free(geo);
}

static const char *gravity_to_string(xcb_gravity_t gravity) {
    switch (gravity)
    {
    case XCB_GRAVITY_NORTH_WEST:
        return "NorthWest";
    case XCB_GRAVITY_NORTH:
        return "North";
    case XCB_GRAVITY_NORTH_EAST:
        return "NorthEast";
    case XCB_GRAVITY_WEST:
        return "West";
    case XCB_GRAVITY_CENTER:
        return "Center";
    case XCB_GRAVITY_EAST:
        return "East";
    case XCB_GRAVITY_SOUTH_WEST:
        return "SouthWest";
    case XCB_GRAVITY_SOUTH:
        return "South";
    case XCB_GRAVITY_SOUTH_EAST:
        return "SouthEast";
    case XCB_GRAVITY_STATIC:
        return "Static";
    default:
        fprintf(stderr, "Unknown gravity value %d", (int) gravity);
        exit(1);
    }
}

static const char *state_to_string(enum test_state state)
{
    switch (state)
    {
    case TEST_STATE_CREATED:
        return "CREATED";
    case TEST_STATE_UNMAPPED:
        return "UNMAPPED";
    case TEST_STATE_MAPPED:
        return "MAPPED";
    case TEST_STATE_RESIZED1:
        return "RESIZED1";
    case TEST_STATE_RESIZED2:
        return "RESIZED2";
    case TEST_STATE_MOVED:
        return "MOVED";
    case TEST_STATE_DONE:
        return "DONE";
    case TEST_STATE_DONE_GOT_CONFIGURE:
        return "DONE+c";
    default:
        return "UNKNOWN";
    }
}

static int32_t div2(int32_t value, int32_t *rounding)
{
    /* If value is odd, we could round up or down */
    if (value & 1)
        *rounding = 1;
    return value / 2;
}

static void check_geometry(int32_t expected_x, int32_t expected_y, uint32_t expected_width, uint32_t expected_height)
{
    int32_t actual_x, actual_y;
    uint32_t actual_width, actual_height, left, right, top, bottom;
    get_geometry(window_state.window, &actual_x, &actual_y, &actual_width, &actual_height,
            &left, &right, &top, &bottom);

    int32_t offset_x, offset_y;
    int32_t extra_x = 0, extra_y = 0;
    int32_t diff_x = state_difference[window_state.state][0], diff_y = state_difference[window_state.state][1];
    switch (window_state.gravity)
    {
    case XCB_GRAVITY_NORTH_WEST:
        offset_x = left;
        offset_y = top;
        break;
    case XCB_GRAVITY_NORTH:
        offset_x = div2(left - right + diff_x, &extra_x);;
        offset_y = top;
        break;
    case XCB_GRAVITY_NORTH_EAST:
        offset_x = -right + diff_x;
        offset_y = top;
        break;
    case XCB_GRAVITY_WEST:
        offset_x = left;
        offset_y = div2(top - bottom + diff_y, &extra_y);
        break;
    case XCB_GRAVITY_CENTER:
        offset_x = div2(left - right + diff_x, &extra_x);
        offset_y = div2(top - bottom + diff_y, &extra_y);
        break;
    case XCB_GRAVITY_EAST:
        offset_x = -right + diff_x;
        offset_y = div2(top - bottom + diff_y, &extra_y);
        break;
    case XCB_GRAVITY_SOUTH_WEST:
        offset_x = left;
        offset_y = -bottom + diff_y;
        break;
    case XCB_GRAVITY_SOUTH:
        offset_x = div2(left - right + diff_x, &extra_x);
        offset_y = -bottom + diff_y;
        break;
    case XCB_GRAVITY_SOUTH_EAST:
        offset_x = -right + diff_x;
        offset_y = -bottom + diff_y;
        break;
    case XCB_GRAVITY_STATIC:
        offset_x = 0;
        offset_y = 0;
        break;
    default:
        fprintf(stderr, "Unknown gravity!?\n");
        return;
    }

    do_log("Checking if position of window with gravity %s is %dx%d and size "
            "is %dx%d when frame has size left=%d, right=%d, top=%d, bottom=%d",
            gravity_to_string(window_state.gravity),
            expected_x, expected_y, expected_width, expected_height,
            left, right, top, bottom);

    check(expected_width == actual_width,
            "For window with gravity %s in state %s, expected width = %d, but got %d",
            gravity_to_string(window_state.gravity), state_to_string(window_state.state), (int) expected_width, (int) actual_width);
    check(expected_height == actual_height,
            "For window with gravity %s in state %s, expected height = %d, but got %d",
            gravity_to_string(window_state.gravity), state_to_string(window_state.state), (int) expected_height, (int) actual_height);
    check(expected_x + offset_x == actual_x || expected_x + offset_x + extra_x == actual_x,
            "For window with gravity %s in state %s, expected x = %d+%d (+%d), but got %d",
            gravity_to_string(window_state.gravity), state_to_string(window_state.state), (int) expected_x, (int) offset_x, (int) extra_x, (int) actual_x);
    check(expected_y + offset_y == actual_y || expected_y + offset_y + extra_y == actual_y,
            "For window with gravity %s in state %s, expected y = %d+%d (+%d), but got %d",
            gravity_to_string(window_state.gravity), state_to_string(window_state.state), (int) expected_y, (int) offset_y, (int) extra_y, (int) actual_y);
}

static void init(xcb_gravity_t gravity)
{
    const char *name = gravity_to_string(gravity);
    xcb_size_hints_t size_hints;

    memset(&size_hints, 0, sizeof(size_hints));
    size_hints.win_gravity = gravity;
    size_hints.flags = XCB_ICCCM_SIZE_HINT_US_POSITION
        | XCB_ICCCM_SIZE_HINT_P_WIN_GRAVITY;

    window_state.window = xcb_generate_id(c);
    window_state.state = TEST_STATE_CREATED;
    window_state.gravity = gravity;

    do_log("creating window at %dx%d with size %dx%d", WIN_POS_X1, WIN_POS_Y1, WIN_WIDTH1, WIN_HEIGHT1);
    xcb_create_window(c, screen->root_depth, window_state.window, screen->root,
            WIN_POS_X1, WIN_POS_Y1, WIN_WIDTH1, WIN_HEIGHT1, 0,
            XCB_WINDOW_CLASS_INPUT_OUTPUT, screen->root_visual,
            XCB_CW_BACK_PIXEL | XCB_CW_EVENT_MASK, (uint32_t[]) {
                screen->white_pixel,
                XCB_EVENT_MASK_STRUCTURE_NOTIFY | XCB_EVENT_MASK_EXPOSURE | XCB_EVENT_MASK_PROPERTY_CHANGE
            });
    xcb_change_property(c, XCB_PROP_MODE_REPLACE, window_state.window,
            XCB_ATOM_WM_NAME, XCB_ATOM_STRING, 8, strlen(name), name);
    xcb_icccm_set_wm_size_hints(c, window_state.window, XCB_ATOM_WM_NORMAL_HINTS, &size_hints);
    xcb_map_window(c, window_state.window);
}

static void handle_expose(xcb_expose_event_t *ev)
{
    if (window_state.window != ev->window
            || (window_state.state != TEST_STATE_CREATED && window_state.state != TEST_STATE_MAPPED))
        return;

    do_log("Window in state %s was exposed, going to next state", state_to_string(window_state.state));
    delayed_proceed_with_window();
}

static void handle_configure_notify(xcb_configure_notify_event_t *ev)
{
    bool synthetic = ev->response_type & 0x80;
    if (window_state.window != ev->window)
        return;

    switch (window_state.state)
    {
    case TEST_STATE_CREATED:
    case TEST_STATE_UNMAPPED:
    case TEST_STATE_MAPPED:
        return;
    case TEST_STATE_RESIZED1:
        if (synthetic)
            return;
        do_log("Window in state RESIZED1 got ConfigureNotify, going to next state");
        delayed_proceed_with_window();
        return;
    case TEST_STATE_RESIZED2:
        if (synthetic)
            return;
        do_log("Window in state RESIZED2 got ConfigureNotify, going to next state");
        delayed_proceed_with_window();
        return;
    case TEST_STATE_MOVED:
        if (!synthetic)
            return;
        do_log("Window in state MOVED got ConfigureNotify, going to next state");
        delayed_proceed_with_window();
        return;
    case TEST_STATE_DONE:
        if (!synthetic)
            return;
    }

    check(false, "Unexpected configure notify for window 0x%x with gravity %d, %dx%d+%d+%d%s",
            (int) window_state.window, (int) window_state.gravity, ev->width, ev->height,
            ev->x, ev->y, synthetic ? " (generated)" : "");
}

static void wait_for_wm(void)
{
    /* Window managers can be awfully slow and ICCCM does not provide much
     * possibilities to synchronise with them. We try to do this by creating a
     * window and resizing it, but without mapping it.
     */
    xcb_generic_event_t *event;
    xcb_window_t window = xcb_generate_id(c);

    do_log("waiting for WM");
    xcb_create_window(c, screen->root_depth, window, screen->root,
            0, 0, 1, 1, 0,
            XCB_WINDOW_CLASS_INPUT_OUTPUT, screen->root_visual,
            XCB_CW_EVENT_MASK, (uint32_t[]) { XCB_EVENT_MASK_STRUCTURE_NOTIFY });
    xcb_configure_window(c, window,
            XCB_CONFIG_WINDOW_X | XCB_CONFIG_WINDOW_Y | XCB_CONFIG_WINDOW_WIDTH | XCB_CONFIG_WINDOW_HEIGHT,
            (uint32_t[]) { 1, 2, 3, 4 });
    xcb_flush(c);

    while (event = xcb_wait_for_event(c)) {
        if ((event->response_type & 0x7f) == XCB_CONFIGURE_NOTIFY) {
            free(event);
            break;
        }
        free(event);
    }

    xcb_destroy_window(c, window);
}

static void handle_client_message(xcb_client_message_event_t *event)
{
    /* We want a full roundtrip between "we decide to proceed" and "we actually
     * proceed" so that any in-flight events are handled. To do this, we send
     * ourselves two client messages. (So that any events that are still in
     * flight when the first message is sent are handled)
     */
    if (event->type == XCB_ATOM_NOTICE)
    {
        wait_for_wm();

        event->response_type = XCB_CLIENT_MESSAGE;
        event->type = XCB_ATOM_WM_COMMAND;
        xcb_send_event(c, 0, event->window, XCB_EVENT_MASK_NO_EVENT, (char *) event);
        return;
    }
    if (event->type != XCB_ATOM_WM_COMMAND)
        return;

    check(window_state.window == event->window, "Got weird client message?!?");

    if (event->data.data32[0] != (int) window_state.state)
        return;

    window_state.sent_delayed_proceed = false;
    switch (window_state.state)
    {
    case TEST_STATE_CREATED:
        /* Verify the position and size set in init() */
        check_geometry(WIN_POS_X1, WIN_POS_Y1, WIN_WIDTH1, WIN_HEIGHT1);

        do_log("Resizing window to size %dx%d", WIN_WIDTH2, WIN_HEIGHT2);
        xcb_configure_window(c, window_state.window,
                XCB_CONFIG_WINDOW_WIDTH | XCB_CONFIG_WINDOW_HEIGHT,
                (uint32_t[]) { WIN_WIDTH2, WIN_HEIGHT2 });
        window_state.state = TEST_STATE_RESIZED1;
        return;
    case TEST_STATE_RESIZED1:
        check_geometry(WIN_POS_X1, WIN_POS_Y1, WIN_WIDTH2, WIN_HEIGHT2);

        do_log("Moving+resizing window to position %dx%d and size %dx%d",
                WIN_POS_X1, WIN_POS_Y1, WIN_WIDTH1, WIN_HEIGHT1);
        xcb_configure_window(c, window_state.window,
                XCB_CONFIG_WINDOW_X | XCB_CONFIG_WINDOW_Y | XCB_CONFIG_WINDOW_WIDTH | XCB_CONFIG_WINDOW_HEIGHT,
                (uint32_t[]) { WIN_POS_X1, WIN_POS_Y1, WIN_WIDTH1, WIN_HEIGHT1 });
        window_state.state = TEST_STATE_RESIZED2;
        return;
    case TEST_STATE_RESIZED2:
        check_geometry(WIN_POS_X1, WIN_POS_Y1, WIN_WIDTH1, WIN_HEIGHT1);

        do_log("Moving window to position %dx%d", WIN_POS_X2, WIN_POS_Y2);
        xcb_configure_window(c, window_state.window,
                XCB_CONFIG_WINDOW_X | XCB_CONFIG_WINDOW_Y,
                (uint32_t[]) { WIN_POS_X2, WIN_POS_Y2 });
        window_state.state = TEST_STATE_MOVED;
        return;
    case TEST_STATE_MOVED:
        check_geometry(WIN_POS_X2, WIN_POS_Y2, WIN_WIDTH1, WIN_HEIGHT1);

        do_log("Unmapping window");
        xcb_unmap_window(c, window_state.window);
        window_state.state = TEST_STATE_UNMAPPED;
        return;
    case TEST_STATE_UNMAPPED:
        do_log("Mapping window");
        xcb_map_window(c, window_state.window);
        window_state.state = TEST_STATE_MAPPED;
        return;
    case TEST_STATE_MAPPED:
        check_geometry(WIN_POS_X2, WIN_POS_Y2, WIN_WIDTH1, WIN_HEIGHT1);

        xcb_destroy_window(c, window_state.window);
        last_window = window_state.window;
        window_state.window = XCB_NONE;
        window_state.state = TEST_STATE_DONE;
        done = true;
        return;
    }
}

static void handle_property_notify(xcb_property_notify_event_t *event)
{
    check(event->window == window_state.window || event->window == last_window,
            "Got property notify on unexpected window 0x%x, expected 0x%x or 0x%x?!",
            event->window, window_state.window, last_window);
    if (window_state.state == TEST_STATE_UNMAPPED && event->atom == wm_state)
        delayed_proceed_with_window();
}

static void event_loop(void)
{
    xcb_generic_event_t *ev;

    done = false;
    xcb_flush(c);
    while (!done && (ev = xcb_wait_for_event(c)))
    {
        uint8_t type = ev->response_type & 0x7f;
        switch (type)
        {
        case XCB_EXPOSE:
            handle_expose((xcb_expose_event_t *) ev);
            break;
        case XCB_CONFIGURE_NOTIFY:
            handle_configure_notify((xcb_configure_notify_event_t *) ev);
            break;
        case XCB_CLIENT_MESSAGE:
            handle_client_message((xcb_client_message_event_t *) ev);
        case XCB_PROPERTY_NOTIFY:
            handle_property_notify((xcb_property_notify_event_t *) ev);
        case XCB_REPARENT_NOTIFY:
        case XCB_MAP_NOTIFY:
        case XCB_UNMAP_NOTIFY:
        case XCB_DESTROY_NOTIFY:
            break;
        default:
            printf("Got unexpected event of type 0x%x\n", (int) ev->response_type);
        }
        free(ev);
        xcb_flush(c);
    }
}

static xcb_atom_t intern_atom(const char *str)
{
    xcb_atom_t result;
    xcb_intern_atom_reply_t *reply = xcb_intern_atom_reply(c,
            xcb_intern_atom(c, 0, strlen(str), str), NULL);
    if (!reply)
        return XCB_NONE;
    result = reply->atom;
    free(reply);
    return result;
}

static void run_test(xcb_gravity_t gravity)
{
    do_log("Doing run for gravity %s", gravity_to_string(gravity));
    init(gravity);
    event_loop();
    do_log("Finished run for gravity %s", gravity_to_string(gravity));
}

int main()
{
    int default_screen;

    c = xcb_connect(NULL, &default_screen);
    if (xcb_connection_has_error(c))
    {
        fprintf(stderr, "Could not connect to X11 server: %d",
                xcb_connection_has_error(c));
        return 1;
    }
    screen = xcb_aux_get_screen(c, default_screen);
    net_frame_extents = intern_atom("_NET_FRAME_EXTENTS");
    wm_state = intern_atom("WM_STATE");

    run_test(XCB_GRAVITY_NORTH_WEST);
    run_test(XCB_GRAVITY_NORTH);
    run_test(XCB_GRAVITY_NORTH_EAST);
    run_test(XCB_GRAVITY_WEST);
    run_test(XCB_GRAVITY_CENTER);
    run_test(XCB_GRAVITY_EAST);
    run_test(XCB_GRAVITY_SOUTH_WEST);
    run_test(XCB_GRAVITY_SOUTH);
    run_test(XCB_GRAVITY_SOUTH_EAST);
    run_test(XCB_GRAVITY_STATIC);

    check(!xcb_connection_has_error(c),
            "X11 connection has error: %d", xcb_connection_has_error(c));
    xcb_disconnect(c);
    if (had_error)
        return 1;
    puts("SUCCESS");
    return 0;
}

// vim: filetype=c:expandtab:shiftwidth=4:tabstop=8:softtabstop=4:textwidth=80
