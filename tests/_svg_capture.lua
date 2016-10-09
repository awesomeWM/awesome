--- Helper to export the current display as an SVG file.
local surface   = require( "gears.surface" )
local wibox = require("wibox")
local beautiful = require("beautiful")
local timer = require("gears.timer")
local titlebar = require("awful.titlebar")

local module = {}

local function fit(self, context, width,height)
    local geo = self._private.client[1]:geometry()

    return math.min(width, geo.width), math.min(height, geo.height)
end

local function draw(self, content, cr, width, height)
    local c = self._private.client[1]
    local s = surface(c.content)

    local geo = c:geometry()

    local scale = math.min(width/geo.width, height/geo.height)

    local w, h = geo.width*scale, geo.height*scale

    cr:translate((width-w)/2, (height-h)/2)

    cr:scale(scale, scale)

    cr:set_source_surface(s)
    cr:paint()
end

local function new_client_screenshot(c)
    local ret = wibox.widget.base.make_widget(nil, nil, {
        enable_properties = true,
    })

    rawset(ret, "fit" , fit )
    rawset(ret, "draw", draw)

    ret._private.client = setmetatable({c},{__mode="v"})

    return ret
end

function module.client_to_widget(c)
    local l = wibox.layout.align.vertical()
    l.fill_space = true

    local tbs = c.titlebars

    if tbs and tbs.top then
        local f = wibox.layout.flex.horizontal()
        f:add(tbs.top.drawable.widget)
        f.forced_width = c.width
        f.forced_height = 16 --FIXME find a reliable way to get that
        l:set_first(f)
    end

    local l2 = wibox.layout.align.horizontal()
    l2.fill_space = true
    l:set_second(l2)

    if tbs and tbs.left then
        local f = wibox.layout.flex.horizontal()
        f:add(tbs.left.drawable.widget)
        f.forced_width = 2
        f.forced_height = c.height - 32
        l2:set_first(f)
    end

    l2:set_second(new_client_screenshot(c))

    if tbs and tbs.right then
        local f = wibox.layout.flex.horizontal()
        f:add(tbs.right.drawable.widget)
        f.forced_width = 2
        f.forced_height = c.height - 32
        l2:set_third(f)
    end

    if tbs and tbs.bottom then
        --FIXME wtf...
--         local f = wibox.layout.flex.horizontal()
--         f:add(tbs.bottom.drawable.widget)
--         f.forced_width = c.width
--         f.forced_height = 16
--         l:set_third(f)
    end

    l.forced_width  = c.width
    l.forced_height = c.height

    return l
end

function module.wibox_to_widget(w)
    local bg = wibox.container.background()

    bg:set_bg(w.bg or beautiful.bg_normal or "#000000")
    bg.forced_width  = w.width
    bg.forced_height = w.height
    bg:set_widget(w:get_widget())

    return bg
end

-- Full capture from a running X11 Awesome instance. It creates a vector file
-- that can be dissected to figure out exactly the Awesome visual state just
-- like some macOS tools. As far as I know, it is the only X11 based WM to be
-- able to do so.
function module.full_capture(path)
    local scr = wibox.layout.free()

    -- The wallpaper
    local wp = surface(root.wallpaper())
    local wp_wdg = wibox.widget.base.make_widget()
    function wp_wdg:draw(context, cr, w, h)
        cr:set_source_surface(wp)
        cr:paint()
    end
    function wp_wdg:fit(c, w, h)
        return w, h
    end
    scr:add(wp_wdg)

    -- Draw all normal wiboxes
    for _, d in ipairs(drawin.get()) do
        local w = d.get_wibox and d:get_wibox() or nil

        if w and not w.ontop then
            scr:add_at(module.wibox_to_widget(w), {w.x, w.y})
        end
    end

    -- Draw all clients
    for _, c in ipairs(client.get(nil, true)) do
        if c:isvisible() then
            local w = module.client_to_widget(c)
            scr:add_at(w, {c.x, c.y})
        end
    end

    -- Draw all ontop wiboes
    for _, d in ipairs(drawin.get()) do
        local w = d.get_wibox and d:get_wibox() or nil

        if w and w.ontop then
            scr:add_at(module.wibox_to_widget(w), {w.x, w.y})
        end
    end

    timer.delayed_call(function()
        local img = surface.widget_to_svg(scr,
            path, screen.primary.geometry.width, screen.primary.geometry.height
        )
        img:finish()
    end)
end

function module.wibox_to_checksum(wb, sum)
    --TODO check if the wibox SVG match a checksum. Note that this requires
    -- it to be called with the same Cairo and Lua version or it will differ.
    -- It would still be useful locally.
end

return module
