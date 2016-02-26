---------------------------------------------------------------------------
--- Wibox module for awful.
-- This module allows you to easily create wibox and attach them to the edge of
-- a screen.
--
-- @author Julien Danjou &lt;julien@danjou.info&gt;
-- @copyright 2009 Julien Danjou
-- @release @AWESOME_VERSION@
-- @module awful.wibox
---------------------------------------------------------------------------

-- Grab environment we need
local capi =
{
    awesome = awesome,
    screen = screen,
    client = client
}
local setmetatable = setmetatable
local tostring = tostring
local ipairs = ipairs
local table = table
local error = error
local wibox = require("wibox")
local beautiful = require("beautiful")
local round = require("awful.util").round

local function get_screen(s)
    return s and screen[s]
end

local awfulwibox = { mt = {} }

--- Array of table with wiboxes inside.
-- It's an array so it is ordered.
local wiboxes = {}

--- Get a wibox position if it has been set, or return top.
-- @param wb The wibox
-- @return The wibox position.
function awfulwibox.get_position(wb)
    for _, wprop in ipairs(wiboxes) do
        if wprop.wibox == wb then
            return wprop.position
        end
    end
    return "top"
end

--- Put a wibox on a screen at this position.
-- @param wb The wibox to attach.
-- @param position The position: top, bottom left or right.
-- @param screen If the wibox it not attached to a screen, specified on which
-- screen the position should be set.
function awfulwibox.set_position(wb, position, screen)
    local area = get_screen(screen).geometry

    -- The "length" of a wibox is always chosen to be the optimal size
    -- (non-floating).
    -- The "width" of a wibox is kept if it exists.
    if position == "right" then
        wb.x = area.x + area.width - (wb.width + 2 * wb.border_width)
    elseif position == "left" then
        wb.x = area.x
    elseif position == "bottom" then
        wb.y = (area.y + area.height) - (wb.height + 2 * wb.border_width)
    elseif position == "top" then
        wb.y = area.y
    end

    for _, wprop in ipairs(wiboxes) do
        if wprop.wibox == wb then
            wprop.position = position
            break
        end
    end
end

--- Reset all wiboxes positions.
local function update_all_wiboxes_position()
    for _, wprop in ipairs(wiboxes) do
        awfulwibox.set_position(wprop.wibox, wprop.position, wprop.screen)
    end
end

local function call_wibox_position_hook_on_prop_update()
    update_all_wiboxes_position()
end

local function wibox_update_strut(wb)
    for _, wprop in ipairs(wiboxes) do
        if wprop.wibox == wb then
            if not wb.visible then
                wb:struts { left = 0, right = 0, bottom = 0, top = 0 }
            elseif wprop.position == "top" then
                wb:struts { left = 0, right = 0, bottom = 0, top = wb.height + 2 * wb.border_width }
            elseif wprop.position == "bottom" then
                wb:struts { left = 0, right = 0, bottom = wb.height + 2 * wb.border_width, top = 0 }
            elseif wprop.position == "left" then
                wb:struts { left = wb.width + 2 * wb.border_width, right = 0, bottom = 0, top = 0 }
            elseif wprop.position == "right" then
                wb:struts { left = 0, right = wb.width + 2 * wb.border_width, bottom = 0, top = 0 }
            end
            break
        end
    end
end

--- Attach a wibox to a screen.
-- If a wibox is attached, it will be automatically be moved when other wiboxes
-- will be attached.
-- @param wb The wibox to attach.
-- @param position The position of the wibox: top, bottom, left or right.
-- @param screen The screen to attach to
function awfulwibox.attach(wb, position, screen)
    screen = get_screen(screen)
    -- Store wibox as attached in a weak-valued table
    local wibox_prop_table
    -- Start from end since we sometimes remove items
    for i = #wiboxes, 1, -1 do
        -- Since wiboxes are stored as weak value, they can disappear.
        -- If they did, remove their entries
        if wiboxes[i].wibox == nil then
            table.remove(wiboxes, i)
        elseif wiboxes[i].wibox == wb then
            wibox_prop_table = wiboxes[i]
            -- We could break here, but well, let's check if there is no other
            -- table with their wiboxes been garbage collected.
        end
    end

    if not wibox_prop_table then
        table.insert(wiboxes, setmetatable({ wibox = wb, position = position, screen = screen }, { __mode = 'v' }))
    else
        wibox_prop_table.position = position
    end

    wb:connect_signal("property::width", wibox_update_strut)
    wb:connect_signal("property::height", wibox_update_strut)
    wb:connect_signal("property::visible", wibox_update_strut)

    wb:connect_signal("property::width", call_wibox_position_hook_on_prop_update)
    wb:connect_signal("property::height", call_wibox_position_hook_on_prop_update)
    wb:connect_signal("property::visible", call_wibox_position_hook_on_prop_update)
    wb:connect_signal("property::border_width", call_wibox_position_hook_on_prop_update)
end

--- Align a wibox.
-- @param wb The wibox.
-- @param align The alignment: left, right or center.
-- @param screen If the wibox is not attached to any screen, you can specify the
-- screen where to align.
function awfulwibox.align(wb, align, screen)
    screen = get_screen(screen)
    local position = awfulwibox.get_position(wb)
    local area = screen.workarea

    if position == "right" then
        if align == "right" then
            wb.y = area.y
        elseif align == "left" then
            wb.y = area.y + area.height - (wb.height + 2 * wb.border_width)
        elseif align == "center" then
            wb.y = area.y + round((area.height - wb.height) / 2)
        end
    elseif position == "left" then
        if align == "right" then
            wb.y = (area.y + area.height) - (wb.height + 2 * wb.border_width)
        elseif align == "left" then
            wb.y = area.y
        elseif align == "center" then
            wb.y = area.y + round((area.height - wb.height) / 2)
        end
    elseif position == "bottom" then
        if align == "right" then
            wb.x = area.x + area.width - (wb.width + 2 * wb.border_width)
        elseif align == "left" then
            wb.x = area.x
        elseif align == "center" then
            wb.x = area.x + round((area.width - wb.width) / 2)
        end
    elseif position == "top" then
        if align == "right" then
            wb.x = area.x + area.width - (wb.width + 2 * wb.border_width)
        elseif align == "left" then
            wb.x = area.x
        elseif align == "center" then
            wb.x = area.x + round((area.width - wb.width) / 2)
        end
    end

    -- Update struts regardless of changes
    wibox_update_strut(wb)
end

--- Stretch a wibox so it takes all screen width or height.
-- @param wb The wibox.
-- @param screen The screen to stretch on, or the wibox screen.
function awfulwibox.stretch(wb, screen)
    if screen then
        screen = get_screen(screen)
        local position = awfulwibox.get_position(wb)
        local area = screen.workarea
        if position == "right" or position == "left" then
            wb.height = area.height - (2 * wb.border_width)
            wb.y = area.y
        else
            wb.width = area.width - (2 * wb.border_width)
            wb.x = area.x
        end
    end
end

--- Create a new wibox and attach it to a screen edge.
-- @see wibox
-- @param arg A table with standard arguments to wibox() creator.
-- You can add also position key with value top, bottom, left or right.
-- You can also use width or height in % and set align to center, right or left.
-- You can also set the screen key with a screen number to attach the wibox.
-- If not specified, 1 is assumed.
-- @return The wibox created.
function awfulwibox.new(arg)
    arg = arg or {}
    local position = arg.position or "top"
    local has_to_stretch = true
    local screen = get_screen(arg.screen or 1)

    arg.type = arg.type or "dock"

    if position ~= "top" and position ~="bottom"
            and position ~= "left" and position ~= "right" then
        error("Invalid position in awful.wibox(), you may only use"
            .. " 'top', 'bottom', 'left' and 'right'")
    end

    -- Set default size
    if position == "left" or position == "right" then
        arg.width = arg.width or round(beautiful.get_font_height(arg.font) * 1.5)
        if arg.height then
            has_to_stretch = false
            if arg.screen then
                local hp = tostring(arg.height):match("(%d+)%%")
                if hp then
                    arg.height = round(screen.geometry.height * hp / 100)
                end
            end
        end
    else
        arg.height = arg.height or round(beautiful.get_font_height(arg.font) * 1.5)
        if arg.width then
            has_to_stretch = false
            if arg.screen then
                local wp = tostring(arg.width):match("(%d+)%%")
                if wp then
                    arg.width = round(screen.geometry.width * wp / 100)
                end
            end
        end
    end

    local w = wibox(arg)

    w.visible = true

    awfulwibox.attach(w, position, screen)
    if has_to_stretch then
        awfulwibox.stretch(w, screen)
    else
        awfulwibox.align(w, arg.align, screen)
    end

    awfulwibox.set_position(w, position, screen)

    return w
end

local function update_wiboxes_on_struts(c)
    local struts = c:struts()
    if struts.left ~= 0 or struts.right ~= 0
       or struts.top ~= 0 or struts.bottom ~= 0 then
        update_all_wiboxes_position()
    end
end

-- Hook registered to reset all wiboxes position.
capi.client.connect_signal("property::struts", update_wiboxes_on_struts)
capi.client.connect_signal("unmanage", update_wiboxes_on_struts)

function awfulwibox.mt:__call(...)
    return awfulwibox.new(...)
end

return setmetatable(awfulwibox, awfulwibox.mt)

-- vim: filetype=lua:expandtab:shiftwidth=4:tabstop=8:softtabstop=4:textwidth=80
