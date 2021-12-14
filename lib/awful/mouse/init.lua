---------------------------------------------------------------------------
--- Mouse module for awful
--
-- @author Julien Danjou &lt;julien@danjou.info&gt;
-- @copyright 2008 Julien Danjou
-- @module mouse
---------------------------------------------------------------------------

-- Grab environment we need
local floating = require("awful.layout.suit.floating")
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
    snap   = require("awful.mouse.snap"),
    client = require("awful.mouse.client"),
    resize = require("awful.mouse.resize"),
    drag_to_tag = require("awful.mouse.drag_to_tag")
}

mouse.object = {}
mouse.wibox = {}

--- The default distance before snapping clients together.
--
-- @DOC_screen_client_snap_EXAMPLE@
--
-- @tfield integer awful.mouse.snap.default_distance
-- @tparam[opt=8] integer default_distance
-- @see awful.mouse.snap

--- The default distance before activating screen edge snap.
-- @tfield integer awful.mouse.snap.aerosnap_distance
-- @tparam[opt=16] integer aerosnap_distance
-- @see awful.mouse.snap

--- Enable screen edges snapping.
--
--@DOC_awful_placement_aero_snap_EXAMPLE@
--
-- @tfield[opt=true] boolean awful.mouse.snap.edge_enabled
-- @tparam boolean edge_enabled

--- Enable client to client snapping.
-- @tfield[opt=true] boolean awful.mouse.snap.client_enabled
-- @tparam boolean client_enabled

--- Enable changing tag when a client is dragged to the edge of the screen.
-- @tfield[opt=false] boolean awful.mouse.drag_to_tag.enabled
-- @tparam boolean enabled

--- The snap outline background color.
-- @beautiful beautiful.snap_bg
-- @tparam color|string|gradient|pattern color

--- The snap outline width.
-- @beautiful beautiful.snap_border_width
-- @tparam integer snap_border_width

--- The snap outline shape.
-- @beautiful beautiful.snap_shape
-- @tparam function shape A `gears.shape` compatible function

--- The gap between snapped clients.
-- @beautiful beautiful.snapper_gap
-- @tparam[opt=0] number snapper_gap

--- The resize cursor name.
-- @beautiful beautiful.cursor_mouse_resize
-- @tparam[opt=cross] string cursor

--- The move cursor name.
-- @beautiful beautiful.cursor_mouse_move
-- @tparam[opt=fleur] string cursor

--- Get the client object under the pointer.
-- @deprecated awful.mouse.client_under_pointer
-- @treturn client|nil The client object under the pointer, if one can be found.
-- @see current_client
function mouse.client_under_pointer()
    gdebug.deprecate("Use mouse.current_client instead of awful.mouse.client_under_pointer()", {deprecated_in=4})

    return mouse.object.get_current_client()
end

-- Older layouts implement their own mousegrabber.
-- @tparam client c The client
-- @tparam table args Additional arguments
-- @treturn boolean This return false when the resize need to be aborted
mouse.resize.add_enter_callback(function(c, args) --luacheck: no unused args
    if c.floating then return end

    local l = c.screen.selected_tag and c.screen.selected_tag.layout or nil
    if l == floating then return end

    if l.mouse_resize_handler then
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
        local bw = w.border_width

        local list = w:find_widgets(coords.x - geo.x - bw, coords.y - geo.y - bw)

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

--- Move the wibox under the cursor.
-- @staticfct awful.mouse.wibox.move
-- @tparam wibox w The wibox to move, or none to use that under the pointer
-- @request wibox geometry mouse.move granted Requests to move the wibox.
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

--- True if the left mouse button is pressed.
-- @property is_left_mouse_button_pressed
-- @tparam boolean is_left_mouse_button_pressed

--- True if the right mouse button is pressed.
-- @property is_right_mouse_button_pressed
-- @tparam boolean is_right_mouse_button_pressed

--- True if the middle mouse button is pressed.
-- @property is_middle_mouse_button_pressed
-- @tparam boolean is_right_mouse_button_pressed

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
-- @staticfct awful.mouse.append_global_mousebindings
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

for k, b in ipairs {"left", "middle", "right"} do
    mouse.object["is_".. b .."_mouse_button_pressed"] = function()
        return capi.mouse.coords().buttons[k]
    end
end

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
    elseif mouse.object[key] and key:sub(1, 3) == "is_" then
        return mouse.object[key]()
    elseif mouse.object[key] then
        return mouse.object[key]
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
-- @treturn table The coords. It contains the `x`, `y` and `buttons` keys.
--  `buttons` contains the button number as key and a boolean as value (if it is
--  pressed).
-- @staticfct mouse.coords
-- @see is_left_mouse_button_pressed
-- @see is_right_mouse_button_pressed
-- @see is_middle_mouse_button_pressed

capi.client.connect_signal("scanning", function()
    capi.client.emit_signal("request::default_mousebindings", "startup")
end)

-- Private function to be used by `ruled.client`.
function mouse._get_client_mousebindings()
    return default_buttons
end

mouse.resize_handler = mouse.resize._resize_handler

return mouse

-- vim: filetype=lua:expandtab:shiftwidth=4:tabstop=8:softtabstop=4:textwidth=80
