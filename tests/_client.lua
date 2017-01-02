local spawn = require("awful.spawn")

-- This file provide a simple, yet flexible, test client.
-- It is used to test the `awful.rules`

local test_client_source = [[
local lgi = require 'lgi'
local Gtk = lgi.require('Gtk')
local Gio = lgi.require('Gio')
Gtk.init()

local function open_window(class, title, snid)
    local window = Gtk.Window {
        default_width  = 100,
        default_height = 100,
        title          = title
    }
    if snid ~= "" then
        window:set_startup_id(snid)
    end
    window:set_wmclass(class, class)
    window:show_all()
end

local function read_line(stream)
    local line, length = stream:async_read_line()
    local eof = not line -- Behaviour of up-to-date LGI
            or (tostring(line) == "" and #line ~= length) -- (Ab)use uninitialized variable
    if eof then
        Gtk.main_quit()
    else
        return tostring(line) -- tostring() needed for older LGI versions
    end
end

local function read_input(stream)
    while true do
        local class = read_line(stream)
        local title = read_line(stream)
        local snid = read_line(stream)
        open_window(class, title, snid)
    end
end

-- Read lines from stdin and handle them
local stdin = Gio.UnixInputStream.new(0, false)
stdin = Gio.DataInputStream.new(stdin)
Gio.Async.start(read_input)(stdin)

Gtk:main{...}
]]

local lgi = require("lgi")
local Gio = lgi.require("Gio")

local initialized = false
local pipe
local function init()
    if initialized then return end
    initialized = true
    local cmd = { "lua", "-e", test_client_source }
    local _, _, stdin, stdout, stderr = awesome.spawn(cmd, false, true, true, true)
    pipe = Gio.UnixOutputStream.new(stdin, true)
    stdout = Gio.UnixInputStream.new(stdout, true)
    stderr = Gio.UnixInputStream.new(stderr, true)
    spawn.read_lines(stdout, function(...) print("_client", ...) end)
    spawn.read_lines(stderr, function(...) print("_client", ...) end)
end

-- Hack needed for awesome's Startup Notification machinery
local function get_snid(sn_rules, callback)
    local success, snid = spawn({ "/bin/true" }, sn_rules, callback)
    assert(success)
    assert(snid)
    return snid
end

return function(class, title, sn_rules, callback)
    class = class or "test_app"
    title = title or "Awesome test client"

    init()
    local snid = (sn_rules or callback) and get_snid(sn_rules, callback) or ""
    local data = class .. "\n" .. title .. "\n" .. snid .. "\n"
    local success, msg = pipe:write_all(data)
    assert(success, tostring(msg))

    return snid
end

-- vim: filetype=lua:expandtab:shiftwidth=4:tabstop=8:softtabstop=4:textwidth=80
