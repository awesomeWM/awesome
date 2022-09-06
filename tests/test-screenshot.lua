-- This test suite tests the various screenshot related APIs.
--
-- Credit: https://www.reddit.com/r/awesomewm/comments/i6nf7z/need_help_writing_a_color_picker_widget_using_lgi/
local awful = require("awful")
local wibox = require("wibox")
local spawn = require("awful.spawn")
local gsurface = require("gears.surface")
local lgi = require('lgi')
local cairo = lgi.cairo
local gdk = lgi.require('Gdk', '3.0')
local capi = {
    root = _G.root
}
-- Dummy blue client for the client.content test
-- the lua_executable portion may need to get ironed out. I need to specify 5.3
local lua_executable = os.getenv("LUA")
if lua_executable == nil or lua_executable == "" then
    lua_executable = "lua5.3"
end
local client_dim = 250
local tiny_client_code_template = [[
local lgi = require('lgi')
local Gdk = lgi.require('Gdk', '3.0')
local Gtk = lgi.require('Gtk', '3.0')
local class = 'client'
window = Gtk.Window {default_width=%d, default_height=%d, title='title'}
window:set_wmclass(class, class)
window:override_background_color(0,
    Gdk.RGBA({red = 0, green = 0, blue = 1, alpha = 1}))
window:show_all()
Gtk:main{...}
]]
local tiny_client = { lua_executable, "-e", string.format(
                          tiny_client_code_template, client_dim, client_dim)}

-- We need a directory to configure the screenshot library to use even through
-- we never actually write a file.
local fake_screenshot_dir = "/tmp"

-- Split in the screen into 2 distict screens.
screen[1]:split()

-- Add a green wibox on screen 1 at (100, 100)
wibox {
    bg      = "#00ff00",
    visible = true,
    width   = 100,
    height  = 100,
    x       = 100,
    y       = 100,
}

local corners = {}

-- Add red squares at 1 pixel away from each screen corners.
for s in screen do
    -- Top left
    table.insert(corners, {
        x = s.geometry.x + 1,
        y = s.geometry.y + 1,
    })

    -- Top right
    table.insert(corners, {
        x = s.geometry.x + s.geometry.width - 11,
        y = s.geometry.y + 1,
    })

    -- Bottom left
    table.insert(corners, {
        x = s.geometry.x + 1,
        y = s.geometry.y + s.geometry.height - 11,
    })

    -- Bottom right
    table.insert(corners, {
        x = s.geometry.x + s.geometry.width - 11,
        y = s.geometry.y + s.geometry.height - 11,
    })

end

-- Make 10x10 wibox in each corner to eliminate any geometry errors
for _, corner in ipairs(corners) do
    corner.bg = "#ff0000"
    corner.visible = true
    corner.width = 10
    corner.height = 10

    wibox(corner)
end

local function copy_to_image_surface(content, w, h)
    local sur = gsurface(content)

    local img = cairo.ImageSurface(cairo.Format.RGB24, w, h)
    local cr = cairo.Context(img)

    cr:set_source_surface(sur)
    cr:paint()
    img:flush()

    return img
end

local function get_pixel(img, x, y)
    local bytes = gdk.pixbuf_get_from_surface(img, x, y, 1, 1):get_pixels()
    return "#" .. bytes:gsub('.', function(c) return ('%02x'):format(c:byte()) end)
end

local snipper_success = nil
local function snipper_cb(ss)
    local img = ss.surface
    snipper_success = img and get_pixel(img, 10, 10) == "#00ff00"
end

local steps = {}

-- Check the whole root window with root.content()
table.insert(steps, function()
    local root_width, root_height = root.size()
    local img = copy_to_image_surface(capi.root.content(), root_width,
                                      root_height)

    if get_pixel(img, 100, 100) ~= "#00ff00" then return end
    if get_pixel(img, 2, 2) ~= "#ff0000" then return end

    assert(get_pixel(img, 100, 100) == "#00ff00")
    assert(get_pixel(img, 199, 199) == "#00ff00")
    assert(get_pixel(img, 201, 201) ~= "#00ff00")

    assert(get_pixel(img, 2, 2) == "#ff0000")
    assert(get_pixel(img, root_width - 2, 2) == "#ff0000")
    assert(get_pixel(img, 2, root_height - 2) == "#ff0000")
    assert(get_pixel(img, root_width - 2, root_height - 2) == "#ff0000")

    return true
end)

-- Check screen.content
table.insert(steps, function()
    for s in screen do

      local geo = s.geometry
      local img = copy_to_image_surface(s.content, geo.width, geo.height)

      assert(get_pixel(img, 4, 4) == "#ff0000")
      assert(get_pixel(img, geo.width - 4, 4) == "#ff0000")
      assert(get_pixel(img, 4, geo.height - 4) == "#ff0000")
      assert(get_pixel(img, geo.width - 4, geo.height - 4) == "#ff0000")

--[[
      assert(get_pixel(img, geo.x + 4, geo.y + 4) == "#ff0000")
      assert(get_pixel(img, geo.x + geo.width - 4, geo.y + 4) == "#ff0000")
      assert(get_pixel(img, geo.x + 4, geo.y + geo.height - 4) == "#ff0000")
      assert(get_pixel(img, geo.x + geo.width - 4, geo.y + geo.height - 4) == "#ff0000")
--]]

    end

    -- Spawn for the client.content test
    assert(#client.get() == 0)
    spawn(tiny_client)

    return true
end)

-- Check client.content
table.insert(steps, function()
    if #client.get() ~= 1 then return end
    local c = client.get()[1]
    local geo = c:geometry()
    local img = copy_to_image_surface(c.content, geo.width, geo.height)

    if get_pixel(img, math.floor(geo.width / 2), math.floor(geo.height / 2)) ~= "#0000ff" then
        return
    end

    -- Make sure the process finishes. Just `c:kill()` only
    -- closes the window. Adding some handlers to the GTK "app"
    -- created some unwanted side effects in the CI.
    awesome.kill(c.pid, 9)

    return true
end)

table.insert(steps, function()
    --Make sure client from last test is gone
    if #client.get() ~= 0 then return end

    local fake_screenshot_dir2 = string.gsub(fake_screenshot_dir, "/*$", "/", 1)

    local ss = awful.screenshot { directory = "/tmp" }
    local name_prfx = fake_screenshot_dir2 .. "Screenshot-"

    local f  = string.find(ss.file_path, name_prfx)
    if f ~= 1 then
        error("Failed autogenerate filename: " .. ss.file_path .. " : " .. name_prfx)
        return false
    end

    name_prfx = fake_screenshot_dir2 .. "MyShot.png"
    ss.file_path = name_prfx

    if ss.file_path ~= name_prfx then
        error("Failed assign filename: " .. ss.file_path .. " : " .. name_prfx)
        return false
    end

    return true

end)

--Check the root window with awful.screenshot.root() method
table.insert(steps, function()

    local root_width, root_height = root.size()
    local ss = awful.screenshot { directory = "/tmp" }
    ss:refresh()

    local img = ss.surface
    assert(img)

    assert(get_pixel(img, 100, 100) == "#00ff00")
    assert(get_pixel(img, 199, 199) == "#00ff00")
    assert(get_pixel(img, 201, 201) ~= "#00ff00")

    assert(get_pixel(img, 2, 2) == "#ff0000")
    assert(get_pixel(img, root_width - 2, 2) == "#ff0000")
    assert(get_pixel(img, 2, root_height - 2) == "#ff0000")
    assert(get_pixel(img, root_width - 2, root_height - 2) == "#ff0000")

    if ss.screen ~= nil or ss.client ~= nil then
        error("Returned non nil screen or client for root screenshot")
        return false
    end

    return true

end)

-- Check the awful.screenshot.screen() method
table.insert(steps, function()
    for s in screen do
        local geo = s.geometry
        local ss = awful.screenshot {screen = s,  directory = "/tmp" }
        ss:refresh()

        local img = ss.surface
        assert(img)

        assert(get_pixel(img, 4, 4) == "#ff0000")
        assert(get_pixel(img, geo.width - 4, 4) == "#ff0000")
        assert(get_pixel(img, 4, geo.height - 4) == "#ff0000")
        assert(get_pixel(img, geo.width - 4, geo.height - 4) == "#ff0000")
    end

    -- Spawn for the client.content test
    assert(#client.get() == 0)
    spawn(tiny_client)

    return true

end)

-- Check the awful.screenshot.client() method
table.insert(steps, function()

    if #client.get() ~= 1 then return end

    local c = client.get()[1]
    local geo = c:geometry()
    local ss = awful.screenshot {client = c,  directory = "/tmp" }
    ss:refresh()
    local img = ss.surface
    assert(img)

    if get_pixel(img, math.floor(geo.width / 2), math.floor(geo.height / 2)) ~= "#0000ff" then
        return
    end

    -- Make sure the process finishes. Just `c:kill()` only
    -- closes the window. Adding some handlers to the GTK "app"
    -- created some unwanted side effects in the CI.
    awesome.kill(c.pid, 9)

    return true

end)

--Check the snipper toop with awful.screenshot.snipper() method
table.insert(steps, function()
    --Make sure client from last test is gone
    if #client.get() ~= 0 then return end
    --Ensure mousegrabber is satisfied
    root.fake_input("button_press",1)
    return true
end)

table.insert(steps, function()
    root.fake_input("button_release",1)
    awesome.sync()
    return true
end)

table.insert(steps, function()
    local ss = awful.screenshot { interactive = true,  directory = "/tmp"  }
    ss:refresh()

    ss:connect_signal("snipping::success", snipper_cb)

    return true
end)

table.insert(steps, function()
    mouse.coords {x = 110, y = 110}
    return true
end)

table.insert(steps, function()
    root.fake_input("button_press",1)
    return true
end)

table.insert(steps, function()
    mouse.coords {x = 190, y = 190}
    awesome.sync()
    return true
end)

table.insert(steps, function()
    root.fake_input("button_release",1)
    return true
end)

table.insert(steps, function()
    if snipper_success == nil then return end

    return snipper_success
end)


--Check the snipper collapse and cancel
table.insert(steps, function()
    --Make sure client from last test is gone
    if #client.get() ~= 0 then return end
    --Ensure mousegrabber is satisfied
    root.fake_input("button_press",1)
    return true
end)

table.insert(steps, function()
    root.fake_input("button_release",1)

    awesome.sync()
    return true
end)

table.insert(steps, function()
    local ss = awful.screenshot { interactive = true, directory = "/tmp"  }
    ss:connect_signal("snipping::success", snipper_cb)
    return true
end)

table.insert(steps, function()
    mouse.coords {x = 110, y = 110}
    return true
end)

table.insert(steps, function()
    root.fake_input("button_press",1)
    return true
end)

table.insert(steps, function()
    root.fake_input("button_release",1)
    return true
end)

table.insert(steps, function()
    mouse.coords {x = 150, y = 150}
    awesome.sync()
    return true
end)

table.insert(steps, function()
    --Cause a rectangle collapse
    mouse.coords {x = 150, y = 110}
    awesome.sync()
    return true
end)

table.insert(steps, function()
    --Cancel snipper tool
    root.fake_input("button_press",3)
    return true
end)

table.insert(steps, function()
    root.fake_input("button_release",3)
    awesome.sync()
    return true
end)

table.insert(steps, function()
    local ss = awful.screenshot {
        geometry = {x = 100, y = 100, width = 100, height = 100}
    }
    ss:refresh()

    local img = ss.surface
    return get_pixel(img, 10, 10) == "#00ff00"
end)

local escape_works, enter_works = false, false

table.insert(steps, function()
    local ss = awful.screenshot { interactive = true }

    ss:connect_signal("snipping::start", function()
        ss:connect_signal("snipping::cancelled", function() enter_works = true end)
    end)

    ss:refresh()
    return true
end)

table.insert(steps, function()
    if not enter_works then
        root.fake_input("key_press","Return")
        root.fake_input("key_release","Return")
        return
    end
    local ss = awful.screenshot { interactive = true }
    ss:connect_signal("snipping::cancelled", function() escape_works = true end)
    ss:refresh()
    return true
end)

-- Test auto-save
table.insert(steps, function()
    if not escape_works then
        root.fake_input("key_press","Escape")
        root.fake_input("key_release","Escape")
        return
    end

    local called = false
    local save = awful.screenshot.save
    awful.screenshot.save = function() called = true end
    local ss = awful.screenshot { auto_save_delay = 0 }
    ss:connect_signal("snipping::cancelled", function() escape_works = true end)
    awful.screenshot.save = save

    assert(called)

    return true
end)

local timer_start, timer_tick, timer_timeout, saved = false, false, false, false

-- Test delayed auto-save
table.insert(steps, function()
    local ss = awful.screenshot { auto_save_tick_duration = 0.05 }

    ss:connect_signal("timer::started", function() timer_start = true end)
    ss:connect_signal("timer::tick", function() timer_tick = true end)
    ss:connect_signal("timer::timeout", function() timer_timeout = true end)
    ss:connect_signal("file::saved", function() saved = true end)

    ss.auto_save_delay = 1

    return true
end)

table.insert(steps, function()
    if not (timer_start and timer_tick and timer_timeout and saved) then return end
    return true
end)

require("_runner").run_steps(steps)
