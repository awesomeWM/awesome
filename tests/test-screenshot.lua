-- This test suite tests the various screenshot related APIs.
--
-- Credit: https://www.reddit.com/r/awesomewm/comments/i6nf7z/need_help_writing_a_color_picker_widget_using_lgi/
local wibox = require("wibox")
local spawn = require("awful.spawn")
local gsurface = require("gears.surface")
local lgi = require 'lgi'
local cairo = lgi.cairo
local gdk = lgi.Gdk

-- Dummy blue client for the client.content test
-- the lua_executable portion may need to get ironed out. I need to specify 5.3
local lua_executable = os.getenv("LUA")
if lua_executable == nil or lua_executable == "" then
    lua_executable = "lua5.3"
end
local client_dim = 250
local tiny_client_code_template = [[
local Gtk = require('lgi').Gtk
local Gdk = require('lgi').Gdk
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
    img:finish()

    return img
end

local function get_pixel(img, x, y)
    local bytes = gdk.pixbuf_get_from_surface(img, x, y, 1, 1):get_pixels()
    return "#" .. bytes:gsub('.', function(c) return ('%02x'):format(c:byte()) end)
end

local steps = {}

-- Check the whole root window.
table.insert(steps, function()
    local img = copy_to_image_surface(root.content(), root.size())
    local root_width, root_height = root.size()

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

-- Check the screen.content
table.insert(steps, function()
    for s in screen do

      local geo = s.geometry
      local img = copy_to_image_surface(s.content, geo.width, geo.height)

      assert(get_pixel(img, geo.x + 4, geo.y + 4) == "#ff0000")
      assert(get_pixel(img, geo.x + geo.width - 4, geo.y + 4) == "#ff0000")
      assert(get_pixel(img, geo.x + 4, geo.y + geo.height - 4) == "#ff0000")
      assert(get_pixel(img, geo.x + geo.width - 4, geo.y + geo.height - 4) == "#ff0000")

    end

    -- Spawn for the client.content test
    assert(#client.get() == 0)
    spawn(tiny_client)

    return true
end)

-- Check the client.content
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

require("_runner").run_steps(steps)
