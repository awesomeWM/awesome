---------------------------------------------------------------------------
--- Mouse module for awful
--
-- @author Julien Danjou &lt;julien@danjou.info&gt;
-- @copyright 2008 Julien Danjou
-- @module mouse
---------------------------------------------------------------------------

-- Grab environment we need
local layout = require("awful.layout")
local aplace = require("awful.placement")
local gdebug = require("gears.debug")
local type = type
local ipairs = ipairs
local capi =
{
    root = root,
    mouse = mouse,
    screen = screen,
    client = client,
    mousegrabber = mousegrabber,
}

local mouse = {
    resize = require("awful.mouse.resize"),
    snap   = require("awful.mouse.snap"),
    drag_to_tag = require("awful.mouse.drag_to_tag")
}

mouse.object = {}
mouse.client = {}
mouse.wibox = {}

--- The default snap distance.
-- @tfield integer awful.mouse.snap.default_distance
-- @tparam[opt=8] integer default_distance
-- @see awful.mouse.snap

--- Enable screen edges snapping.
-- @tfield[opt=true] boolean awful.mouse.snap.edge_enabled

--- Enable client to client snapping.
-- @tfield[opt=true] boolean awful.mouse.snap.client_enabled

--- Enable changing tag when a client is dragged to the edge of the screen.
-- @tfield[opt=false] integer awful.mouse.drag_to_tag.enabled

--- The snap outline background color.
-- @beautiful beautiful.snap_bg
-- @tparam color|string|gradient|pattern color

--- The snap outline width.
-- @beautiful beautiful.snap_border_width
-- @param integer

--- The snap outline shape.
-- @beautiful beautiful.snap_shape
-- @tparam function shape A `gears.shape` compatible function

--- The gap between snapped contents.
-- @beautiful beautiful.snapper_gap
-- @tparam number (default: 0)

--- Get the client object under the pointer.
-- @deprecated awful.mouse.client_under_pointer
-- @return The client object under the pointer, if one can be found.
-- @see current_client
function mouse.client_under_pointer()
    gdebug.deprecate("Use mouse.current_client instead of awful.mouse.client_under_pointer()", {deprecated_in=4})

    return mouse.object.get_current_client()
end

--- Move a client.
-- @staticfct awful.mouse.client.move
-- @param c The client to move, or the focused one if nil.
-- @param snap The pixel to snap clients.
-- @param finished_cb Deprecated, do not use
function mouse.client.move(c, snap, finished_cb) --luacheck: no unused args
    if finished_cb then
        gdebug.deprecate("The mouse.client.move `finished_cb` argument is no longer"..
            " used, please use awful.mouse.resize.add_leave_callback(f, 'mouse.move')", {deprecated_in=4})
    end

    c = c or capi.client.focus

    if not c
        or c.fullscreen
        or c.maximized
        or c.type == "desktop"
        or c.type == "splash"
        or c.type == "dock" then
        return
    end

    -- Compute the offset
    local coords = capi.mouse.coords()
    local geo    = aplace.centered(capi.mouse,{parent=c, pretend=true})

    local offset = {
        x = geo.x - coords.x,
        y = geo.y - coords.y,
    }

    mouse.resize(c, "mouse.move", {
        placement = aplace.under_mouse,
        offset    = offset,
        snap      = snap
    })
end

mouse.client.dragtotag = { }

--- Move a client to a tag by dragging it onto the left / right side of the screen.
-- @deprecated awful.mouse.client.dragtotag.border
-- @param c The client to move
function mouse.client.dragtotag.border(c)
    gdebug.deprecate("Use awful.mouse.snap.drag_to_tag_enabled = true instead "..
        "of awful.mouse.client.dragtotag.border(c). It will now be enabled.", {deprecated_in=4})

    -- Enable drag to border
    mouse.snap.drag_to_tag_enabled = true

    return mouse.client.move(c)
end

--- Move the wibox under the cursor.
-- @staticfct awful.mouse.wibox.move
--@tparam wibox w The wibox to move, or none to use that under the pointer
function mouse.wibox.move(w)
    w = w or mouse.current_wibox
    if not w then return end

    if not w
        or w.type == "desktop"
        or w.type == "splash"
        or w.type == "dock" then
        return
    end

    -- Compute the offset
    local coords = capi.mouse.coords()
    local geo    = aplace.centered(capi.mouse,{parent=w, pretend=true})

    local offset = {
        x = geo.x - coords.x,
        y = geo.y - coords.y,
    }

    mouse.resize(w, "mouse.move", {
        placement = aplace.under_mouse,
        offset    = offset
    })
end

--- Get a client corner coordinates.
-- @deprecated awful.mouse.client.corner
-- @tparam[opt=client.focus] client c The client to get corner from, focused one by default.
-- @tparam string corner The corner to use: auto, top_left, top_right, bottom_left,
-- bottom_right, left, right, top bottom. Default is auto, and auto find the
-- nearest corner.
-- @treturn string The corner name
-- @treturn number x The horizontal position
-- @treturn number y The vertical position
function mouse.client.corner(c, corner)
    gdebug.deprecate(
        "Use awful.placement.closest_corner(mouse) or awful.placement[corner](mouse)"..
        " instead of awful.mouse.client.corner", {deprecated_in=4}
    )

    c = c or capi.client.focus
    if not c then return end

    local ngeo = nil

    if (not corner) or corner == "auto" then
        ngeo, corner = aplace.closest_corner(mouse, {parent = c})
    elseif corner and aplace[corner] then
        ngeo = aplace[corner](mouse, {parent = c})
    end

    return corner, ngeo and ngeo.x or nil, ngeo and ngeo.y or nil
end

--- Resize a client.
-- @staticfct awful.mouse.client.resize
-- @param c The client to resize, or the focused one by default.
-- @tparam string corner The corner to grab on resize. Auto detected by default.
-- @tparam[opt={}] table args A set of `awful.placement` arguments
-- @treturn string The corner (or side) name
function mouse.client.resize(c, corner, args)
    c = c or capi.client.focus

    if not c then return end

    if c.fullscreen
        or c.maximized
        or c.type == "desktop"
        or c.type == "splash"
        or c.type == "dock" then
        return
    end

    -- Set some default arguments
    local new_args = setmetatable(
        {
            include_sides = (not args) or args.include_sides ~= false
        },
        {
            __index = args or {}
        }
    )

    -- Move the mouse to the corner
    if corner and aplace[corner] then
        aplace[corner](capi.mouse, {parent=c})
    else
        local _
        _, corner = aplace.closest_corner(capi.mouse, {
            parent        = c,
            include_sides = new_args.include_sides ~= false,
        })
    end

    new_args.corner = corner

    mouse.resize(c, "mouse.resize", new_args)

    return corner
end

--- Default handler for `request::geometry` signals with "mouse.resize" context.
-- @signalhandler awful.mouse.resize_handler
-- @tparam client c The client
-- @tparam string context The context
-- @tparam[opt={}] table hints The hints to pass to the handler
function mouse.resize_handler(c, context, hints)
    local is_accepted = hints and context
        and context:find("mouse.*")
        and c:check_allowed_requests("request::geometry", context, hints) ~= false

    if not is_accepted then return end

    -- This handler only handle the floating clients. If the client is tiled,
    -- then it let the layouts handle it.
    local t = c.screen.selected_tag
    local lay = t and t.layout or nil

    if (lay and lay == layout.suit.floating) or c.floating then
        c:geometry {
            x      = hints.x,
            y      = hints.y,
            width  = hints.width,
            height = hints.height,
        }
    elseif lay and lay.resize_handler then
        lay.resize_handler(c, context, hints)
    end
end

-- Older layouts implement their own mousegrabber.
-- @tparam client c The client
-- @tparam table args Additional arguments
-- @treturn boolean This return false when the resize need to be aborted
mouse.resize.add_enter_callback(function(c, args) --luacheck: no unused args
    if c.floating then return end

    local l = c.screen.selected_tag and c.screen.selected_tag.layout or nil
    if l == layout.suit.floating then return end

    if l ~= layout.suit.floating and l.mouse_resize_handler then
        capi.mousegrabber.stop()

        local geo, corner = aplace.closest_corner(capi.mouse, {parent=c})

        l.mouse_resize_handler(c, corner, geo.x, geo.y)

        return false
    end
end, "mouse.resize")

--- Get the client currently under the mouse cursor.
-- @property current_client
-- @tparam client|nil The client

function mouse.object.get_current_client()
    local obj = capi.mouse.object_under_pointer()
    if type(obj) == "client" then
        return obj
    end
end

--- Get the wibox currently under the mouse cursor.
-- @property current_wibox
-- @tparam wibox|nil The wibox

function mouse.object.get_current_wibox()
    local obj = capi.mouse.object_under_pointer()
    if type(obj) == "drawin" and obj.get_wibox then
        return obj:get_wibox()
    end
end

--- Get the widgets currently under the mouse cursor.
--
-- @property current_widgets
-- @tparam nil|table list The widget list
-- @treturn table The list of widgets.The first element is the biggest
-- container while the last is the topmost widget. The table contains *x*, *y*,
-- *width*, *height* and *widget*.

function mouse.object.get_current_widgets()
    local w = mouse.object.get_current_wibox()
    if w then
        local geo, coords = w:geometry(), capi.mouse:coords()

        local list = w:find_widgets(coords.x - geo.x, coords.y - geo.y)

        local ret = {}

        for k, v in ipairs(list) do
            ret[k] = v.widget
        end

        return ret, list
    end
end

--- Get the topmost widget currently under the mouse cursor.
-- @property current_widget
-- @tparam widget|nil widget The widget
-- @treturn ?widget The widget
-- @see current_widget_geometry

function mouse.object.get_current_widget()
    local wdgs, geos = mouse.object.get_current_widgets()

    if wdgs then
        return wdgs[#wdgs], geos[#geos]
    end
end

--- Get the current widget geometry.
-- @property current_widget_geometry
-- @tparam ?table The geometry.
-- @see current_widget

function mouse.object.get_current_widget_geometry()
    local _, ret = mouse.object.get_current_widget()

    return ret
end

--- Get the current widget geometries.
-- @property current_widget_geometries
-- @tparam ?table A list of geometry tables.
-- @see current_widgets

function mouse.object.get_current_widget_geometries()
    local _, ret = mouse.object.get_current_widgets()

    return ret
end

--- True if the left mouse button is pressed.
-- @property is_left_mouse_button_pressed
-- @param boolean

--- True if the right mouse button is pressed.
-- @property is_right_mouse_button_pressed
-- @param boolean

--- True if the middle mouse button is pressed.
-- @property is_middle_mouse_button_pressed
-- @param boolean

--- Add an `awful.button` based mousebinding to the global set.
--
-- A **global** mousebinding is one which is always present, even when there is
-- no focused client. If your intent is too add a mousebinding which acts on
-- the focused client do **not** use this.
--
-- @staticfct awful.mouse.append_global_mousebinding
-- @tparam awful.button button The button object.
-- @see awful.button

function mouse.append_global_mousebinding(button)
    capi.root._append_button(button)
end

--- Add multiple `awful.button` based mousebindings to the global set.
--
-- A **global** mousebinding is one which is always present, even when there is
-- no focused client. If your intent is too add a mousebinding which acts on
-- the focused client do **not** use this
--
-- @tparam table buttons A table of `awful.button` objects. Optionally, it can have
--  a `group` entry. If set, the `group` property will be set on all `awful.buttons`
--  objects.
-- @see awful.button

function mouse.append_global_mousebindings(buttons)
    local g = buttons.group
    buttons.group = nil

    -- Avoid the boilerplate. If the user is adding multiple buttons at once, then
    -- they are probably related.
    if g then
        for _, k in ipairs(buttons) do
            k.group = g
        end
    end

    capi.root._append_buttons(buttons)
    buttons.group = g
end

--- Remove a mousebinding from the global set.
--
-- @staticfct awful.mouse.remove_global_mousebinding
-- @tparam awful.button button The button object.
-- @see awful.button

function mouse.remove_global_mousebinding(button)
    capi.root._remove_button(button)
end

local default_buttons = {}

--- Add an `awful.button` to the default client buttons.
--
-- @staticfct awful.mouse.append_client_mousebinding
-- @tparam awful.button button The button.
-- @emits client_mousebinding::added
-- @emitstparam client_mousebinding::added awful.button button The button.
-- @see awful.button
-- @see awful.keyboard.append_client_keybinding

function mouse.append_client_mousebinding(button)
    table.insert(default_buttons, button)

    for _, c in ipairs(capi.client.get(nil, false)) do
        c:append_mousebinding(button)
    end

    capi.client.emit_signal("client_mousebinding::added", button)
end

--- Add a `awful.button`s to the default client buttons.
--
-- @staticfct awful.mouse.append_client_mousebindings
-- @tparam table buttons A table containing `awful.button` objects.
-- @see awful.button
-- @see awful.keyboard.append_client_keybinding
-- @see awful.mouse.append_client_mousebinding
-- @see awful.keyboard.append_client_keybindings

function mouse.append_client_mousebindings(buttons)
    for _, button in ipairs(buttons) do
        mouse.append_client_mousebinding(button)
    end
end

--- Remove a mousebinding from the default client buttons.
--
-- @staticfct awful.mouse.remove_client_mousebinding
-- @tparam awful.button button The button.
-- @treturn boolean True if the button was removed and false if it wasn't found.
-- @see awful.keyboard.append_client_keybinding

function mouse.remove_client_mousebinding(button)
    for k, v in ipairs(default_buttons) do
        if button == v then
            table.remove(default_buttons, k)

            for _, c in ipairs(capi.client.get(nil, false)) do
                c:remove_mousebinding(button)
            end

            return true
        end
    end

    return false
end

for _, b in ipairs {"left", "right", "middle"} do
    mouse.object["is_".. b .."_mouse_button_pressed"] = function()
        return capi.mouse.coords().buttons[1]
    end
end

capi.client.connect_signal("request::geometry", mouse.resize_handler)

-- Set the cursor at startup
capi.root.cursor("left_ptr")

-- Implement the custom property handler
local props = {}

capi.mouse.set_newindex_miss_handler(function(_,key,value)
    if mouse.object["set_"..key] then
        mouse.object["set_"..key](value)
    elseif not mouse.object["get_"..key] then
        props[key] = value
    else
        -- If there is a getter, but no setter, then the property is read-only
        error("Cannot set '" .. tostring(key) .. " because it is read-only")
    end
end)

capi.mouse.set_index_miss_handler(function(_,key)
    if mouse.object["get_"..key] then
        return mouse.object["get_"..key]()
    else
        return props[key]
    end
end)

--- Get or set the mouse coords.
--
--@DOC_awful_mouse_coords_EXAMPLE@
--
-- @tparam[opt=nil] table coords_table None or a table with x and y keys as mouse
--  coordinates.
-- @tparam[opt=nil] integer coords_table.x The mouse horizontal position
-- @tparam[opt=nil] integer coords_table.y The mouse vertical position
-- @tparam[opt=false] boolean silent Disable mouse::enter or mouse::leave events that
--  could be triggered by the pointer when moving.
-- @treturn integer table.x The horizontal position
-- @treturn integer table.y The vertical position
-- @treturn table table.buttons Table containing the status of buttons, e.g. field [1] is true
--  when button 1 is pressed.
-- @staticfct mouse.coords

capi.client.connect_signal("scanning", function()
    capi.client.emit_signal("request::default_mousebindings", "startup")
end)

-- Private function to be used by `ruled.client`.
function mouse._get_client_mousebindings()
    return default_buttons
end

return mouse

-- vim: filetype=lua:expandtab:shiftwidth=4:tabstop=8:softtabstop=4:textwidth=80
