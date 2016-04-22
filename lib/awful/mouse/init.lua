---------------------------------------------------------------------------
--- Mouse module for awful
--
-- @author Julien Danjou &lt;julien@danjou.info&gt;
-- @copyright 2008 Julien Danjou
-- @release @AWESOME_VERSION@
-- @module awful.mouse
---------------------------------------------------------------------------

-- Grab environment we need
local layout = require("awful.layout")
local tag = require("awful.tag")
local aplace = require("awful.placement")
local awibox = require("awful.wibox")
local util = require("awful.util")
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
}

mouse.client = {}
mouse.wibox = {}

--- Get the client object under the pointer.
-- @return The client object under the pointer, if one can be found.
function mouse.client_under_pointer()
    local obj = capi.mouse.object_under_pointer()
    if type(obj) == "client" then
        return obj
    end
end

--- Get the drawin object under the pointer.
-- @return The drawin object under the pointer, if one can be found.
function mouse.drawin_under_pointer()
    local obj = capi.mouse.object_under_pointer()
    if type(obj) == "drawin" then
        return obj
    end
end

--- Move a client.
-- @param c The client to move, or the focused one if nil.
-- @param snap The pixel to snap clients.
-- @param finished_cb Deprecated, do not use
function mouse.client.move(c, snap, finished_cb) --luacheck: no unused args
    if finished_cb then
        util.deprecated("The mouse.client.move `finished_cb` argument is no longer"..
            " used, please use awful.mouse.resize.add_leave_callback(f, 'mouse.move')")
    end

    c = c or capi.client.focus

    if not c
        or c.fullscreen
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

    mouse.resize(c, "mouse.move", {placement=aplace.under_mouse, offset=offset})
end

mouse.client.dragtotag = { }

--- Move a client to a tag by dragging it onto the left / right side of the screen
-- @param c The client to move
function mouse.client.dragtotag.border(c)
    capi.mousegrabber.run(function (_mouse)
                                if not c.valid then return false end

                                local button_down = false
                                for _, v in ipairs(_mouse.buttons) do
                                    if v then button_down = true end
                                end
                                local wa = capi.screen[c.screen].workarea
                                if _mouse.x >= wa.x + wa.width then
                                    capi.mouse.coords({ x = wa.x + wa.width - 1 })
                                elseif _mouse.x <= wa.x then
                                    capi.mouse.coords({ x = wa.x + 1 })
                                end
                                if not button_down then
                                    local tags = c.screen.tags
                                    local t = c.screen.selected_tag
                                    local idx
                                    for i, v in ipairs(tags) do
                                        if v == t then
                                            idx = i
                                        end
                                    end
                                    if _mouse.x > wa.x + wa.width - 10 then
                                        local newtag = tags[util.cycle(#tags, idx + 1)]
                                        c:move_to_tag(newtag)
                                        tag.viewnext()
                                    elseif _mouse.x < wa.x + 10 then
                                        local newtag = tags[util.cycle(#tags, idx - 1)]
                                        c:move_to_tag(newtag)
                                        tag.viewprev()
                                    end
                                    return false
                                end
                                return true
                            end, "fleur")
end

--- Move the wibox under the cursor
--@param w The wibox to move, or none to use that under the pointer
function mouse.wibox.move(w)
    w = w or mouse.wibox_under_pointer()
    if not w then return end

    local offset = {
        x = w.x - capi.mouse.coords().x,
        y = w.y - capi.mouse.coords().y
    }

    capi.mousegrabber.run(function (_mouse)
        local button_down = false
        if awibox.get_position(w) == "floating" then
            w.x = capi.mouse.coords().x + offset.x
            w.y = capi.mouse.coords().y + offset.y
        else
            local wa = capi.screen[capi.mouse.screen].workarea

            if capi.mouse.coords()["y"] > wa.y + wa.height - 10 then
                awibox.set_position(w, "bottom", w.screen)
            elseif capi.mouse.coords()["y"] < wa.y + 10 then
                awibox.set_position(w, "top", w.screen)
            elseif capi.mouse.coords()["x"] > wa.x + wa.width - 10 then
                awibox.set_position(w, "right", w.screen)
            elseif capi.mouse.coords()["x"] < wa.x + 10 then
                awibox.set_position(w, "left", w.screen)
            end
            w.screen = capi.mouse.screen
        end
        for _, v in ipairs(_mouse.buttons) do
            if v then button_down = true end
        end
        if not button_down then
            return false
        end
        return true
    end, "fleur")
end

--- Get a client corner coordinates.
-- @tparam[opt=client.focus] client c The client to get corner from, focused one by default.
-- @tparam string corner The corner to use: auto, top_left, top_right, bottom_left,
-- bottom_right, left, right, top bottom. Default is auto, and auto find the
-- nearest corner.
-- @treturn string The corner name
-- @treturn number x The horizontal position
-- @treturn number y The vertical position
function mouse.client.corner(c, corner)
    util.deprecated(
        "Use awful.placement.closest_corner(mouse) or awful.placement[corner](mouse)"..
        " instead of awful.mouse.client.corner"
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
-- @param c The client to resize, or the focused one by default.
-- @tparam string corner The corner to grab on resize. Auto detected by default.
-- @tparam[opt={}] table args A set of `awful.placement` arguments
function mouse.client.resize(c, corner, args)
    c = c or capi.client.focus

    if not c then return end

    if c.fullscreen
        or c.type == "desktop"
        or c.type == "splash"
        or c.type == "dock" then
        return
    end

    -- Move the mouse to the corner
    if corner and aplace[corner] then
        aplace[corner](capi.mouse, {parent=c})
    end

    mouse.resize(c, "mouse.resize", args or {include_sides=true})
end

--- Default handler for `request::geometry` signals with `mouse.resize` context.
-- @tparam client c The client
-- @tparam string context The context
-- @tparam[opt={}] table hints The hints to pass to the handler
function mouse.resize_handler(c, context, hints)
    if hints and context and context:find("mouse.*") then
        -- This handler only handle the floating clients. If the client is tiled,
        -- then it let the layouts handle it.
        local lay = c.screen.selected_tag.layout

        if lay == layout.suit.floating or c.floating then
            local offset = hints and hints.offset or {}

            if type(offset) == "number" then
                offset = {
                    x      = offset,
                    y      = offset,
                    width  = offset,
                    height = offset,
                }
            end

            c:geometry {
                x      = hints.x      + (offset.x      or 0 ),
                y      = hints.y      + (offset.y      or 0 ),
                width  = hints.width  + (offset.width  or 0 ),
                height = hints.height + (offset.height or 0 ),
            }
        elseif lay.resize_handler then
            lay.resize_handler(c, context, hints)
        end
    end
end

capi.client.connect_signal("request::geometry", mouse.resize_handler)

-- Set the cursor at startup
capi.root.cursor("left_ptr")

return mouse

-- vim: filetype=lua:expandtab:shiftwidth=4:tabstop=8:softtabstop=4:textwidth=80
