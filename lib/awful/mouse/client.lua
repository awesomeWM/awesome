--- Client related mouse operations.
--
-- @author Emmanuel Lepage Vallee &lt;elv1313@gmail.com&gt;
-- @copyright 2016 Emmanuel Lepage Vallee
-- @submodule mouse

local aplace = require("awful.placement")
local capi = { mouse = mouse, client = client }
local mresize = require("awful.mouse.resize")
local gdebug = require("gears.debug")

local module = {}

--- Move a client.
-- @staticfct awful.mouse.client.move
-- @param c The client to move, or the focused one if nil.
-- @param snap The pixel to snap clients.
-- @param finished_cb Deprecated, do not use
function module.move(c, snap, finished_cb) --luacheck: no unused args
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

    mresize(c, "mouse.move", {
        placement = aplace.under_mouse,
        offset    = offset,
        snap      = snap
    })
end

module.dragtotag = { }

--- Move a client to a tag by dragging it onto the left / right side of the screen.
-- @deprecated awful.mouse.client.dragtotag.border
-- @param c The client to move
function module.dragtotag.border(c)
    gdebug.deprecate("Use awful.mouse.snap.drag_to_tag_enabled = true instead "..
        "of awful.mouse.client.dragtotag.border(c). It will now be enabled.", {deprecated_in=4})

    -- Enable drag to border
    require("awful.mouse.snap").drag_to_tag_enabled = true

    return module.move(c)
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
function module.corner(c, corner)
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
function module.resize(c, corner, args)
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

    mresize(c, "mouse.resize", new_args)

    return corner
end

return module
