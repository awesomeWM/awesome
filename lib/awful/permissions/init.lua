---------------------------------------------------------------------------
--- Default implementation of the various requests handers.
--
-- AwesomeWM has many `request::` signals across the core APIs. They help
-- decouple the default behavior from the core API. The request handlers
-- can be disconnected and replaced by module or `rc.lua` to alter
-- AwesomeWM behavior.
--
-- This module provides the default implementation of those request handlers.
-- Beside some legacy signals, most request handlers have a main object, a
-- named context and a table containing any low-level hints the core code is
-- aware of. Each default handler implement a context based filter mechanism.
-- This filter is called the "permissions". It allows requests to be denied.
-- For example, if a tiled client asks to be resized or moved, the permission
-- and deny it. In the documentation, many objects and properties have a
-- "permissions" section you can display by clicking "show more".
--
-- @author Julien Danjou &lt;julien@danjou.info&gt;
-- @copyright 2009 Julien Danjou
-- @module awful.permissions
---------------------------------------------------------------------------

local client = client
local screen = screen
local ipairs = ipairs
local timer = require("gears.timer")
local gtable = require("gears.table")
local aclient = require("awful.client")
local mresize = require("awful.mouse.resize")
local aplace = require("awful.placement")
local asuit = require("awful.layout.suit")
local beautiful = require("beautiful")
local alayout = require("awful.layout")
local atag = require("awful.tag")
local gdebug = require("gears.debug")
local wibox = require("wibox")
local pcommon = require("awful.permissions._common")

local permissions = {
    generic_activate_filters    = {},
    contextual_activate_filters = {},
}

--- Honor the screen padding when maximizing.
-- @beautiful beautiful.maximized_honor_padding
-- @tparam[opt=true] boolean maximized_honor_padding

--- Hide the border on fullscreen clients.
-- @beautiful beautiful.fullscreen_hide_border
-- @tparam[opt=true] boolean fullscreen_hide_border

--- Hide the border on maximized clients.
-- @beautiful beautiful.maximized_hide_border
-- @tparam[opt=false] boolean maximized_hide_border

--- The list of all registered generic `request::activate` (focus stealing)
-- filters. If a filter is added to only one context, it will be in
-- `permissions.contextual_activate_filters`["context_name"].
-- @table[opt={}] awful.permissions.generic_activate_filters
-- @see permissions.activate
-- @see permissions.add_activate_filter
-- @see permissions.remove_activate_filter

--- The list of all registered contextual `request::activate` (focus stealing)
-- filters. If a filter is added to only one context, it will be in
-- `permissions.generic_activate_filters`.
-- @table[opt={}] awful.permissions.contextual_activate_filters
-- @see permissions.activate
-- @see permissions.add_activate_filter
-- @see permissions.remove_activate_filter

--- Update a client's settings when its geometry changes, skipping signals
-- resulting from calls within.
local repair_geometry_lock = false

local function repair_geometry(window)
    if repair_geometry_lock then return end
    repair_geometry_lock = true

    -- Re-apply the geometry locking properties to what they should be.
    for _, prop in ipairs {
        "fullscreen", "maximized", "maximized_vertical", "maximized_horizontal"
    } do
        if window[prop] then
            window:emit_signal("request::geometry", prop, {
                store_geometry = false
            })
            break
        end
    end

    repair_geometry_lock = false
end

local function filter_sticky(c)
    return not c.sticky and aclient.focus.filter(c)
end

--- Give focus when clients appear/disappear.
--
-- @param obj An object that should have a .screen property.
local function check_focus(obj)
    if (not obj.screen) or not obj.screen.valid then return end
    -- When no visible client has the focus...
    if not client.focus or not client.focus:isvisible() then
        local c = aclient.focus.history.get(screen[obj.screen], 0, filter_sticky)
        if not c then
            c = aclient.focus.history.get(screen[obj.screen], 0, aclient.focus.filter)
        end
        if c then
            c:emit_signal(
                "request::autoactivate",
                "history",
                {raise=false}
            )
        end
    end
end

--- Check client focus (delayed).
-- @param obj An object that should have a .screen property.
local function check_focus_delayed(obj)
    timer.delayed_call(check_focus, {screen = obj.screen})
end

--- Give focus on tag selection change.
--
-- @tparam tag t A tag object
local function check_focus_tag(t)
    local s = t.screen
    if (not s) or (not s.valid) then return end
    s = screen[s]
    check_focus({ screen = s })
    if client.focus and screen[client.focus.screen] ~= s then
        local c = aclient.focus.history.get(s, 0, filter_sticky)
        if not c then
            c = aclient.focus.history.get(s, 0, aclient.focus.filter)
        end
        if c then
            c:emit_signal(
                "request::autoactivate",
                "switch_tag",
                {raise=false}
            )
        end
    end
end

--- Activate a window.
--
-- This sets the focus only if the client is visible. If `raise` is set
-- in the hints, it will also unminimize the client and move it to the top
-- of its layer.
--
-- It is the default signal handler for `request::activate` on a `client`.
--
-- @signalhandler awful.permissions.activate
-- @tparam client c A client to use
-- @tparam string context The context where this signal was used.
-- @tparam[opt] table hints A table with additional hints:
-- @tparam[opt=false] boolean hints.raise Should the client be unminimized
--  and raised?
-- @tparam[opt=false] boolean hints.switch_to_tag Should the client's first tag
--  be selected if none of the client's tags are selected?
-- @tparam[opt=false] boolean hints.switch_to_tags Select all tags associated
--  with the client.
-- @sourcesignal client request::activate
function permissions.activate(c, context, hints) -- luacheck: no unused args
    if not pcommon.check(c, "client", "activate", context) then return end

    hints = hints or  {}

    if c.focusable == false and not hints.force then
        if hints.raise then
            c:raise()
        end

        return
    end

    local found, ret = false

    -- Execute the filters until something handle the request
    for _, tab in ipairs {
        permissions.contextual_activate_filters[context] or {},
        permissions.generic_activate_filters
    } do
        for i=#tab, 1, -1 do
            ret = tab[i](c, context, hints)
            if ret ~= nil then found=true; break end
        end

        if found then break end
    end

    -- Minimized clients can be requested to have focus by, for example, 3rd
    -- party toolbars and they might not try to unminimize it first.
    if ret ~= false and hints.raise then
        c.minimized = false
    end

    if ret ~= false and c:isvisible() then
        client.focus = c
    elseif ret == false and not hints.force then
        return
    end

    if hints.raise then
        c:raise()
        if not awesome.startup and not c:isvisible() then
            c.urgent = true
        end
    end

    -- The rules use `switchtotag`. For consistency and code re-use, support it,
    -- but keep undocumented. --TODO v5 remove switchtotag
    if hints.switchtotag or hints.switch_to_tag or hints.switch_to_tags then
        atag.viewmore(c:tags(), c.screen, (not hints.switch_to_tags) and 0 or nil)
    end
end

--- Add an activate (focus stealing) filter function.
--
-- The callback takes the following parameters:
--
-- * **c** (*client*) The client requesting the activation
-- * **context** (*string*) The activation context.
-- * **hints** (*table*) Some additional hints (depending on the context)
--
-- If the callback returns `true`, the client will be activated. If the callback
-- returns `false`, the activation request is cancelled unless the `force` hint is
-- set. If the callback returns `nil`, the previous callback will be executed.
-- This will continue until either a callback handles the request or when it runs
-- out of callbacks. In that case, the request will be granted if the client is
-- visible.
--
-- For example, to block Firefox from stealing the focus, use:
--
--    awful.permissions.add_activate_filter(function(c)
--        if c.class == "Firefox" then return false end
--    end, "permissions")
--
-- @tparam function f The callback
-- @tparam[opt] string context The `request::activate` context
-- @noreturn
-- @see generic_activate_filters
-- @see contextual_activate_filters
-- @see remove_activate_filter
-- @staticfct awful.permissions.add_activate_filter
function permissions.add_activate_filter(f, context)
    if not context then
        table.insert(permissions.generic_activate_filters, f)
    else
        permissions.contextual_activate_filters[context] =
            permissions.contextual_activate_filters[context] or {}

        table.insert(permissions.contextual_activate_filters[context], f)
    end
end

--- Remove an activate (focus stealing) filter function.
-- This is an helper to avoid dealing with `permissions.add_activate_filter` directly.
-- @tparam function f The callback
-- @tparam[opt] string context The `request::activate` context
-- @treturn boolean If the callback existed
-- @see generic_activate_filters
-- @see contextual_activate_filters
-- @see add_activate_filter
-- @staticfct awful.permissions.remove_activate_filter
function permissions.remove_activate_filter(f, context)
    local tab = context and (permissions.contextual_activate_filters[context] or {})
        or permissions.generic_activate_filters

    for k, v in ipairs(tab) do
        if v == f then
            table.remove(tab, k)

            -- In case the callback is there multiple time.
            permissions.remove_activate_filter(f, context)

            return true
        end
    end

    return false
end

-- Get tags that are on the same screen as the client. This should _almost_
-- always return the same content as c:tags().
local function get_valid_tags(c, s)
    local tags, new_tags = c:tags(), {}

    for _, t in ipairs(tags) do
        if s == t.screen then
            table.insert(new_tags, t)
        end
    end

    return new_tags
end

--- Tag a window with its requested tag.
--
-- It is the default signal handler for `request::tag` on a `client`.
--
-- @signalhandler awful.permissions.tag
-- @tparam client c A client to tag
-- @tparam[opt] tag|boolean t A tag to use. If `true`, then the client is made `sticky`.
-- @tparam[opt={}] table hints Extra information.
-- @tparam[opt=nil] nil|string hints.reason Why the tag is being changed.
-- @sourcesignal client request::tag
function permissions.tag(c, t, hints) --luacheck: no unused
    -- There is nothing to do
    if not t and #get_valid_tags(c, c.screen) > 0 then return end

    if not t then
        if c.transient_for and not (hints and hints.reason == "screen") then
            c.screen = c.transient_for.screen
            if not c.sticky then
                local tags = c.transient_for:tags()
                c:tags(#tags > 0 and tags or c.transient_for.screen.selected_tags)
            end
        else
            c:to_selected_tags()
        end
    elseif type(t) == "boolean" and t then
        c.sticky = true
    else
        c.screen = t.screen
        c:tags({ t })
    end
end

--- Handle client urgent request
-- @signalhandler awful.permissions.urgent
-- @tparam client c A client
-- @tparam boolean urgent If the client should be urgent
-- @sourcesignal client request::urgent
function permissions.urgent(c, urgent)
    if c ~= client.focus and not aclient.property.get(c,"ignore_urgent") then
        c.urgent = urgent
    end
end

-- Map the state to the action name
local context_mapper = {
    maximized_vertical   = "maximize_vertically",
    maximized_horizontal = "maximize_horizontally",
    maximized            = "maximize",
    fullscreen           = "maximize"
}

--- Move and resize the client.
--
-- This is the default geometry request handler.
--
-- @signalhandler awful.permissions.geometry
-- @tparam client c The client
-- @tparam string context The context
-- @tparam[opt={}] table hints The hints to pass to the handler
-- @tparam[opt=true] boolean hints.store_geometry Create a memento of the
--  previous geometry in case it has to be restored.
-- @tparam[opt=true] boolean hints.honor_workarea Avoid overlapping the `wibar`s
--  or other desktop decoration when applying the geometry.
-- @sourcesignal client request::geometry
function permissions.geometry(c, context, hints)
    if not pcommon.check(c, "client", "geometry", context) then return end

    local layout = c.screen.selected_tag and c.screen.selected_tag.layout or nil

    context = context or ""

    -- Setting the geometry will not work unless the client is floating.
    if (
            (context ~= "fullscreen")
            and (context ~= "maximized")
            and (not c.floating)
            and (layout ~= asuit.floating)
    ) then
        return
    end

    local original_context = context

    -- Now, map it to something useful
    context = context_mapper[context] or context

    local props = gtable.clone(hints or {}, false)
    props.store_geometry = props.store_geometry==nil and true or props.store_geometry

    -- If it is a known placement function, then apply it, otherwise let
    -- other potential handler resize the client (like in-layout resize or
    -- floating client resize)
    if aplace[context] then

        -- Check if it corresponds to a boolean property.
        local state = c[original_context]

        -- If the property is boolean and it corresponds to the undo operation,
        -- restore the stored geometry.
        if state == false then
            local original = repair_geometry_lock
            repair_geometry_lock = true
            aplace.restore(c, {context=context})
            repair_geometry_lock = original
            return
        end

        local honor_default = original_context ~= "fullscreen"

        if props.honor_workarea == nil then
            props.honor_workarea = honor_default
        end

        if props.honor_padding == nil and props.honor_workarea and context:match("maximize") then
            props.honor_padding = beautiful.maximized_honor_padding ~= false
        end

        if (original_context == "fullscreen" and beautiful.fullscreen_hide_border ~= false) or
           (original_context == "maximized" and beautiful.maximized_hide_border == true) then
            props.ignore_border_width = true
            props.zap_border_width = true
        end

        local original = repair_geometry_lock
        repair_geometry_lock = true
        aplace[context](c, props)
        repair_geometry_lock = original
    end
end

--- Move and resize the wiboxes.
--
-- This is the default geometry request handler.
--
-- @signalhandler awful.permissions.wibox_geometry
-- @tparam wibox w The wibox.
-- @tparam string context The context
-- @tparam[opt={}] table hints The hints to pass to the handler
-- @tparam[opt] integer hints.x The horizontal position.
-- @tparam[opt] integer hints.y The vertical position.
-- @tparam[opt] integer hints.width The wibox width.
-- @tparam[opt] integer hints.height The wibox height.
-- @sourcesignal client request::geometry
function permissions.wibox_geometry(w, context, hints)
    if not pcommon.check(w, "wibox", "geometry", context) then return end

    w:geometry(hints)
end

--- Merge the 2 requests sent by clients wanting to be maximized.
--
-- The X clients set 2 flags (atoms) when they want to be maximized. This caused
-- 2 `request::geometry` to be sent. This code gives some time for them to arrive
-- and send a new `request::geometry` (through the property change) with the
-- combined state.
--
-- @signalhandler awful.permissions.merge_maximization
-- @tparam client c The client
-- @tparam string context The context
-- @tparam[opt={}] table hints The hints to pass to the handler.
-- @tparam[opt=false] boolean hints.toggle Toggle the maximization state rather
--  than set it to `true`.
-- @sourcesignal client request::geometry
function permissions.merge_maximization(c, context, hints)
    if not pcommon.check(c, "client", "geometry", context) then return end

    if context ~= "client_maximize_horizontal" and context ~= "client_maximize_vertical" then
        return
    end

    if not c._delay_maximization then
        c._delay_maximization = function()
            -- Computes the actual X11 atoms before/after
            local before_max_h = c.maximized or c.maximized_horizontal
            local before_max_v = c.maximized or c.maximized_vertical
            local after_max_h, after_max_v
            if c._delayed_max_h ~= nil then
                after_max_h = c._delayed_max_h
            else
                after_max_h = before_max_h
            end
            if c._delayed_max_v ~= nil then
                after_max_v = c._delayed_max_v
            else
                after_max_v = before_max_v
            end
            -- Interprets the client's intention based on the client's view
            if after_max_h and after_max_v then
                c.maximized = true
            elseif before_max_h and before_max_v then
                -- At this point, c.maximized must be true, and the client is
                -- trying to unmaximize the window, and potentially partial
                -- maximized the window
                c.maximized = false
                if after_max_h ~= after_max_v then
                    c.maximized_horizontal = after_max_h
                    c.maximized_vertical = after_max_v
                end
            else
                -- At this point, c.maximized must be false, and the client is
                -- not trying to fully maximize the window
                c.maximized_horizontal = after_max_h
                c.maximized_vertical = after_max_v
            end
        end

        timer {
            timeout     = 1/60,
            autostart   = true,
            single_shot = true,
            callback    = function()
                if not c.valid then return end

                c._delay_maximization(c)
                c._delay_maximization = nil
                c._delayed_max_h = nil
                c._delayed_max_v = nil
            end
        }
    end

    local function get_value(suffix, long_suffix)
        if hints.toggle and c["_delayed_max_"..suffix] ~= nil then
            return not c["_delayed_max_"..suffix]
        elseif hints.toggle then
            return not (c["maximized"] or c["maximized_"..long_suffix])
        else
            return hints.status
        end
    end

    if context == "client_maximize_horizontal" then
        c._delayed_max_h = get_value("h", "horizontal")
    elseif context == "client_maximize_vertical" then
        c._delayed_max_v = get_value("v", "vertical")
    end
end

--- Allow the client to move itself.
--
-- This is the default geometry request handler when the context is `permissions`.
--
-- @signalhandler awful.permissions.client_geometry_requests
-- @tparam client c The client
-- @tparam string context The context
-- @tparam[opt={}] table hints The hints to pass to the handler.
-- @tparam[opt] integer hints.x The horizontal position.
-- @tparam[opt] integer hints.y The vertical position.
-- @tparam[opt] integer hints.width The client width.
-- @tparam[opt] integer hints.height The client height.
-- @sourcesignal client request::geometry
function permissions.client_geometry_requests(c, context, hints)
    if not pcommon.check(c, "client", "geometry", context) then return end

    if context == "ewmh" and hints then
        if c.immobilized_horizontal then
            hints = gtable.clone(hints)
            hints.x = nil
            hints.width = nil
        end
        if c.immobilized_vertical then
            hints = gtable.clone(hints)
            hints.y = nil
            hints.height = nil
        end
        c:geometry(hints)
    end
end

--- Begins moving a client with the mouse.
--
-- This is the default handler for `request::mouse_move`. When a request is
-- received, it uses `awful.mouse.resize` to initiate a mouse movement transaction
-- that lasts so long as the specified mouse button is held.
--
-- @signalhandler awful.permissions.client_mouse_move
-- @tparam client c The client
-- @tparam string context The context
-- @tparam table args Additional information describing the movement.
-- @tparam number args.mouse_pos.x The x coordinate of the mouse when grabbed.
-- @tparam number args.mouse_pos.y The y coordinate of the mouse when grabbed.
-- @tparam number args.button The mouse button that initiated the movement.
-- @sourcesignal client request::mouse_move
function permissions.client_mouse_move(c, context, args)
    if not pcommon.check(c, "client", "mouse_move", context) then return end

    if not c
        or c.fullscreen
        or c.maximized
        or c.type == "desktop"
        or c.type == "splash"
        or c.type == "dock"
        or mousegrabber.isrunning() then
        return
    end

    local center_pos = aplace.centered(mouse, {parent=c, pretend=true})
    mresize(c, "mouse.move", {
        placement = aplace.under_mouse,
        offset = {
            x = center_pos.x - args.mouse_pos.x,
            y = center_pos.y - args.mouse_pos.y
        },
        mouse_buttons = {args.button}
    })
end

--- Begins resizing a client with the mouse.
--
-- This is the default handler for `request::mouse_resize`. When a request is
-- received, it uses `awful.mouse.resize` to initiate a mouse resizing transaction
-- that lasts so long as the specified mouse button is held.
--
-- @signalhandler awful.permissions.client_mouse_resize
-- @tparam client c The client
-- @tparam string context The context
-- @tparam table args Additional information describing the resizing.
-- @tparam number args.mouse_pos.x The x coordinate of the mouse when grabbed.
-- @tparam number args.mouse_pos.y The y coordinate of the mouse when grabbed.
-- @tparam number args.button The mouse button that initiated the resizing.
-- @tparam string args.corner The corner/side of the window being resized.
-- @sourcesignal client request::mouse_resize
function permissions.client_mouse_resize(c, context, args)
    if not pcommon.check(c, "client", "mouse_resize", context) then return end

    if not c
        or c.fullscreen
        or c.maximized
        or c.type == "desktop"
        or c.type == "splash"
        or c.type == "dock"
        or mousegrabber.isrunning() then
        return
    end

    local corner_pos = aplace[args.corner](mouse, {parent = c, pretend = true})
    mresize(c, "mouse.resize", {
        corner = args.corner,
        corner_lock = true,
        mouse_offset = {
            x = args.mouse_pos.x - corner_pos.x,
            y = args.mouse_pos.y - corner_pos.y
        },
        mouse_buttons = {args.button}
    })
end

--- Cancels a mouse movement/resizing operation.
--
-- This is the default handler for `request::mouse_cancel`. It simply ends any
-- ongoing `mousegrabber` transaction.
--
-- @signalhandler awful.permissions.client_mouse_cancel
-- @tparam client c The client
-- @tparam string context The context
-- @sourcesignal client request::mouse_cancel
function permissions.client_mouse_cancel(c, context)
    if not pcommon.check(c, "client", "mouse_cancel", context) then return end

    -- This will also stop other mouse grabber transactions, but that's probably fine;
    -- a well-behaved client should only raise this signal when a mouse movement or
    -- resizing operation is in progress, and so no other mouse grabber transactions
    -- should be happening at this time.
    mousegrabber.stop()
end

-- The magnifier layout doesn't work with focus follow mouse.
permissions.add_activate_filter(function(c)
    if alayout.get(c.screen) ~= alayout.suit.magnifier
      and aclient.focus.filter(c) then
        return nil
    else
        return false
    end
end, "mouse_enter")

--- The default client `request::border` handler.
--
-- To replace this handler with your own, use:
--
--    client.disconnect_signal("request::border", awful.permisions.update_border)
--
-- The default implementation chooses from dozens beautiful theme variables
-- depending if the client is tiled, floating, maximized and then from its state
-- (urgent, new, active, normal)
--
-- @signalhandler awful.permissions.update_border
-- @tparam client c The client.
-- @tparam string context Why is the border changed.
-- @sourcesignal client request::border
-- @usebeautiful beautiful.border_color_marked
-- @usebeautiful beautiful.border_color_active
-- @usebeautiful beautiful.border_color_normal
-- @usebeautiful beautiful.border_color_new
-- @usebeautiful beautiful.border_color_urgent
-- @usebeautiful beautiful.border_color_floating
-- @usebeautiful beautiful.border_color_floating_active
-- @usebeautiful beautiful.border_color_floating_normal
-- @usebeautiful beautiful.border_color_floating_new
-- @usebeautiful beautiful.border_color_floating_urgent
-- @usebeautiful beautiful.border_color_maximized
-- @usebeautiful beautiful.border_color_maximized_active
-- @usebeautiful beautiful.border_color_maximized_normal
-- @usebeautiful beautiful.border_color_maximized_new
-- @usebeautiful beautiful.border_color_maximized_urgent
-- @usebeautiful beautiful.border_color_fullscreen
-- @usebeautiful beautiful.border_color_fullscreen_active
-- @usebeautiful beautiful.border_color_fullscreen_normal
-- @usebeautiful beautiful.border_color_fullscreen_new
-- @usebeautiful beautiful.border_color_fullscreen_urgent
-- @usebeautiful beautiful.border_width_active
-- @usebeautiful beautiful.border_width_normal
-- @usebeautiful beautiful.border_width_new
-- @usebeautiful beautiful.border_width_urgent
-- @usebeautiful beautiful.border_width_floating
-- @usebeautiful beautiful.border_width_floating_active
-- @usebeautiful beautiful.border_width_floating_normal
-- @usebeautiful beautiful.border_width_floating_new
-- @usebeautiful beautiful.border_width_floating_urgent
-- @usebeautiful beautiful.border_width_maximized
-- @usebeautiful beautiful.border_width_maximized_active
-- @usebeautiful beautiful.border_width_maximized_normal
-- @usebeautiful beautiful.border_width_maximized_new
-- @usebeautiful beautiful.border_width_maximized_urgent
-- @usebeautiful beautiful.border_width_fullscreen
-- @usebeautiful beautiful.border_width_fullscreen_active
-- @usebeautiful beautiful.border_width_fullscreen_normal
-- @usebeautiful beautiful.border_width_fullscreen_new
-- @usebeautiful beautiful.border_width_fullscreen_urgent
-- @usebeautiful beautiful.opacity_floating
-- @usebeautiful beautiful.opacity_floating_active
-- @usebeautiful beautiful.opacity_floating_normal
-- @usebeautiful beautiful.opacity_floating_new
-- @usebeautiful beautiful.opacity_floating_urgent
-- @usebeautiful beautiful.opacity_maximized
-- @usebeautiful beautiful.opacity_maximized_active
-- @usebeautiful beautiful.opacity_maximized_normal
-- @usebeautiful beautiful.opacity_maximized_new
-- @usebeautiful beautiful.opacity_maximized_urgent
-- @usebeautiful beautiful.opacity_fullscreen
-- @usebeautiful beautiful.opacity_fullscreen_active
-- @usebeautiful beautiful.opacity_fullscreen_normal
-- @usebeautiful beautiful.opacity_fullscreen_new
-- @usebeautiful beautiful.opacity_fullscreen_urgent
-- @usebeautiful beautiful.opacity_active
-- @usebeautiful beautiful.opacity_normal
-- @usebeautiful beautiful.opacity_new
-- @usebeautiful beautiful.opacity_urgent
-- @see client.border_width
-- @see client.border_color

function permissions.update_border(c, context)
    if not pcommon.check(c, "client", "border", context) then return end

    local suffix, fallback1, fallback2 = "", ""

    -- Add the sub-namespace.
    if c.fullscreen then
        suffix, fallback1 = "_fullscreen", "_fullscreen"
    elseif c.maximized then
        suffix, fallback1 = "_maximized", "_maximized"
    elseif c.floating then
        suffix, fallback1 = "_floating", "_floating"
    end

    -- Add the state suffix.
    if c.urgent then
        suffix, fallback2 = suffix .. "_urgent", "_urgent"
    elseif c.active then
        suffix, fallback2 = suffix .. "_active", "_active"
    elseif context == "added" then
        suffix, fallback2 = suffix .. "_new", "_new"
    else
        suffix, fallback2 = suffix .. "_normal", "_normal"
    end

    if not c._private._user_border_width then
        local bw = beautiful["border_width"..suffix]
            or beautiful["border_width"..fallback1]
            or beautiful["border_width"..fallback2]

        -- The default `awful.permissions.geometry` handler removes the border.
        if (not bw) and (c.fullscreen or c.maximized) then
            bw = 0
        end

        c._border_width = bw or beautiful.border_width
    end

    if not c._private._user_border_color then
        -- First, check marked clients. This is a concept that should probably
        -- never have been added to the core. The documentation claims it works,
        -- even if it has been broken for 90% of AwesomeWM releases ever since
        -- it was added.
        if c.marked then
            if beautiful.border_color_marked then
                c._border_color = beautiful.border_color_marked
                return
            elseif beautiful.border_marked then
                gdebug.deprecate(
                    "Use `beautiful.border_color_marked` instead of `beautiful.border_marked`",
                    {deprecated_in=5}
                )
                c._border_color = beautiful.border_marked
                return
            end
        end

        local tv = beautiful["border_color"..suffix]

        if (not tv) and fallback1 ~= "" then
            tv = beautiful["border_color"..fallback1]
        end

        if (not tv) and (fallback2 ~= "") then
            tv = beautiful["border_color"..fallback2]
        end

        -- The old theme variable did not have "color" in its name.
        if (not tv) and beautiful.border_normal and (not c.active) then
            gdebug.deprecate(
                "Use `beautiful.border_color_normal` instead of `beautiful.border_normal`",
                {deprecated_in=5}
            )
            tv = beautiful.border_normal
        elseif (not tv) and beautiful.border_focus then
            gdebug.deprecate(
                "Use `beautiful.border_color_active` instead of `beautiful.border_focus`",
                {deprecated_in=5}
            )
            tv = beautiful.border_focus
        end

        if not tv then
            tv = beautiful.border_color
        end

        if tv then
            c._border_color = tv
        end
    end

    if not c._private._user_opacity then
        local tv = beautiful["opacity"..suffix]

        if fallback1 ~= "" and not tv then
            tv = beautiful["opacity"..fallback1]
        end

        if fallback2 ~= "" and not tv then
            tv = beautiful["opacity"..fallback2]
        end

        if tv then
            c._opacity = tv
        end
    end
end

local activate_context_map = {
    mouse_enter = "mouse.enter",
    switch_tag  = "autofocus.check_focus_tag",
    history     = "autofocus.check_focus"
}

--- Default handler for the `request::autoactivate` signal.
--
-- All it does is to emit `request::activate` with the following context
-- mapping:
--
-- * mouse_enter: *mouse.enter*
-- * switch_tag : *autofocus.check_focus_tag*
-- * history    : *autofocus.check_focus*
--
-- @signalhandler awful.permissions.autoactivate
-- @tparam client c The client.
-- @tparam string context Why is the client auto-activated.
-- @tparam[opt={}] table hints Extra arguments.
-- @sourcesignal client request::activated
-- @see activate
-- @see client:activate
function permissions.autoactivate(c, context, args)
    if not pcommon.check(c, "client", "autoactivate", context) then return end

    local ctx = activate_context_map[context] and
        activate_context_map[context] or context

    c:emit_signal("request::activate", ctx, args)
end

client.connect_signal("request::autoactivate"  , permissions.autoactivate)
client.connect_signal("request::border"        , permissions.update_border)
client.connect_signal("request::activate"      , permissions.activate)
client.connect_signal("request::tag"           , permissions.tag)
client.connect_signal("request::urgent"        , permissions.urgent)
client.connect_signal("request::geometry"      , permissions.geometry)
client.connect_signal("request::geometry"      , permissions.merge_maximization)
client.connect_signal("request::geometry"      , permissions.client_geometry_requests)
client.connect_signal("request::mouse_move"    , permissions.client_mouse_move)
client.connect_signal("request::mouse_resize"  , permissions.client_mouse_resize)
client.connect_signal("request::mouse_cancel"  , permissions.client_mouse_cancel)
client.connect_signal("property::border_width" , repair_geometry)
client.connect_signal("property::screen"       , repair_geometry)
client.connect_signal("request::unmanage"      , check_focus_delayed)
client.connect_signal("tagged"                 , check_focus_delayed)
client.connect_signal("untagged"               , check_focus_delayed)
client.connect_signal("property::hidden"       , check_focus_delayed)
client.connect_signal("property::minimized"    , check_focus_delayed)
client.connect_signal("property::sticky"       , check_focus_delayed)

wibox.connect_signal("request::geometry"       , permissions.wibox_geometry)

tag.connect_signal("property::selected", function (t)
    timer.delayed_call(check_focus_tag, t)
end)

screen.connect_signal("property::workarea", function(s)
    for _, c in pairs(client.get(s)) do
        repair_geometry(c)
    end
end)

-- Enable sloppy focus, so that focus follows mouse.
if awesome.api_level > 4 then
    --TODO v5: Remove the code from `rc.lua`. It cannot be done yet because we
    -- cannot know if the user removed it to disable sloppy focus or because
    -- they want to use the permissions to manage it.
    client.connect_signal("mouse::enter", function(c)
        c:emit_signal("request::autoactivate", "mouse_enter", {raise=false})
    end)
end

return permissions

-- vim: filetype=lua:expandtab:shiftwidth=4:tabstop=8:softtabstop=4:textwidth=80
