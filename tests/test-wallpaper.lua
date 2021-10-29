--- Test if the wallpaper function do not cause errors

local runner = require("_runner")
local wp = require("gears.wallpaper")
local color = require("gears.color")
local cairo = require( "lgi" ).cairo
local surface = require("gears.surface")
local awall = require("awful.wallpaper")
local beautiful = require("beautiful")
local wibox = require("wibox")
local gdebug = require("gears.debug")

-- This test suite is for a deprecated module.
gdebug.deprecate = function() end

local steps = {}

local colors = {
    "#000030",
    {
        type  = "linear" ,
        from  = { 0, 0  },
        to    = { 0, 100 },
        stops = {
            { 0, "#ff0000" },
            { 1, "#0000ff" }
        }
    },
    color("#043000"),
    "#302E00",
    "#002C30",
    color("#300030AA"),
    "#301C00",
    "#140030",
}

-- Dummy wallpaper
local img = cairo.ImageSurface.create(cairo.Format.ARGB32, 100, 100)
local cr = cairo.Context(img)
cr:set_source(color(colors[2]))
cr:rectangle(0,0,100,100)
cr:fill()
cr:paint()

table.insert(steps, function()
    assert(img)
    assert(surface.load_uncached(img))

    local w, h = surface.get_size(surface.load_uncached(img))

    assert(w == 100)
    assert(h == 100)

    wp.fit(img, nil, nil)

    wp.fit(img, screen[1], nil)

    -- There is a delayed call for the last call, let it be processed before
    -- adding more
    return true
end)

table.insert(steps, function()
    wp.fit(img, screen[1], "#00ff00")

    -- There is a delayed call for the last call, let it be processed before
    -- adding more
    return true
end)

table.insert(steps, function()
    wp.centered(img, nil, nil)
    wp.centered(img, screen[1], nil)

    -- There is a delayed call for the last call, let it be processed before
    -- adding more
    return true
end)

table.insert(steps, function()
    wp.centered(img, screen[1], "#00ff00")

    return true
end)

table.insert(steps, function()
    wp.maximized(img, nil, nil, nil)

    -- There is a delayed call for the last call, let it be processed before
    -- adding more
    return true
end)

table.insert(steps, function()
    wp.maximized(img, screen[1], nil, nil)

    -- There is a delayed call for the last call, let it be processed before
    -- adding more
    return true
end)

table.insert(steps, function()
    wp.maximized(img, screen[1], false, nil)
    wp.maximized(img, screen[1], true, nil)

    -- There is a delayed call for the last call, let it be processed before
    -- adding more
    return true
end)

table.insert(steps, function()
    wp.maximized(img, screen[1], false, {x=10, y= 10})
    wp.maximized(img, screen[1], true, {x=10, y= 10})

    return true
end)

table.insert(steps, function()
    wp.tiled(img, nil, nil)
    wp.tiled(img, screen[1], nil)

    -- There is a delayed call for the last call, let it be processed before
    -- adding more
    return true
end)

table.insert(steps, function()
    wp.tiled(img, screen[1], {x=10, y= 10})

    return true
end)

table.insert(steps, function()
    for _, c in ipairs(colors) do
        wp.set(c)
    end

    return true
end)

-- Make sure passing `nil` doesn't crash Awesome.
table.insert(steps, function()
    root._wallpaper(nil)
    root.wallpaper(nil)

    return true
end)

local walls = setmetatable({}, {__mode = "v"})

-- Test awful.wallpaper garbage collection.
table.insert(steps, function()
    walls["first"] = awall {
        screen = screen[1],
        widget = {
            {
                image     = beautiful.wallpaper,
                upscale   = true,
                downscale = true,
                widget    = wibox.widget.imagebox,
            },
            valign = "center",
            halign = "center",
            tiled  = false,
            widget = wibox.container.tile,
        }
    }

    collectgarbage("collect")

    return true
end)

table.insert(steps, function(count)
    if count < 3 then
        collectgarbage("collect")
        return
    end

    assert(walls["first"])

    walls["second"] = awall {
        screen = screen[1],
        widget = {
            {
                image     = beautiful.wallpaper,
                upscale   = true,
                downscale = true,
                widget    = wibox.widget.imagebox,
            },
            valign = "center",
            halign = "center",
            tiled  = false,
            widget = wibox.container.tile,
        }
    }

    return true
end)

local repaint_called, paint_width, paint_height = false, nil, nil

-- Test setting "after the fact"
table.insert(steps, function(count)
    if count < 3 then
        collectgarbage("collect")
        return
    end

    assert(walls["second"])
    assert(not walls["first"])

    local real_repaint = walls["second"].repaint

    walls["second"].repaint = function(self)
        repaint_called = true
        real_repaint(self)
    end

    walls["second"].widget = wibox.widget {
        fit = function(_, w, h)
            return w, h
        end,
        draw = function(self, ctx, cr2, width, height)
            cr2:set_source_rgba(1, 0, 0, 0.5)
            cr2:rectangle(0, 0, width/2, height/2)
            cr2:fill()
            paint_width, paint_height = width, height
            assert((not self.dpi) and  ctx.dpi)
        end
    }

    walls["second"].bg = "#0000ff"
    walls["second"].fg = "#00ff00"

    assert(repaint_called)

    -- This needs to happen after this event loop to avoid
    -- painting the wallpaper many time in a row.
    assert(not paint_width)

    return true
end)

table.insert(steps, function()
    assert(walls["second"])

    assert(paint_width == screen[1].geometry.width)
    assert(paint_height == screen[1].geometry.height)

    repaint_called = false
    paint_width, paint_height = nil, nil

    -- Disable new wallpaper creation to prevent request::wallpaper from
    -- replacing the wall.
    local constructor = getmetatable(awall).__call
    setmetatable(awall, {__call = function() end})

    screen[1]:fake_resize(
        10, 10, math.floor(screen[1].geometry.width/2), math.floor(screen[1].geometry.height/2)
    )

    assert(paint_height ~= screen[1].geometry.height)
    assert(repaint_called)
    setmetatable(awall, {__call = constructor})

    return true
end)

table.insert(steps, function(count)
    if count == 1 then return end

    assert(paint_width == screen[1].geometry.width)
    assert(paint_height == screen[1].geometry.height)

    walls["second"].dpi = 123
    assert(walls["second"].dpi == 123)
    walls["second"]:detach()

    return true
end)

table.insert(steps, function(count)
    if count < 3 then
        collectgarbage("collect")
        return
    end

    assert(not walls["second"])

    return true
end)

runner.run_steps(steps)

-- vim: filetype=lua:expandtab:shiftwidth=4:tabstop=8:softtabstop=4:textwidth=80
