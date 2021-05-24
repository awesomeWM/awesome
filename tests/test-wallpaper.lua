--- Test if the wallpaper function do not cause errors

local runner = require("_runner")
local wp = require("gears.wallpaper")
local color = require("gears.color")
local cairo = require( "lgi" ).cairo
local surface = require("gears.surface")

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


runner.run_steps(steps)

-- vim: filetype=lua:expandtab:shiftwidth=4:tabstop=8:softtabstop=4:textwidth=80
