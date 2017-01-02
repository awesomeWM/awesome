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

-- Start a coroutine for nicer input handling
local coro = coroutine.wrap(function()
    while true do
        local class = coroutine.yield()
        local title = coroutine.yield()
        local snid = coroutine.yield()
        open_window(class, title, snid)
    end
end)
coro()

-- Read lines from stdin and feed them to the coroutine
local stdin = Gio.UnixInputStream.new(0, false)
stdin = Gio.DataInputStream.new(stdin)

local read_start, read_finish
read_start = function()
    stdin:read_line_async(0, nil, read_finish)
end
read_finish = function(...)
    local line, length = stdin.read_line_finish(...)
    if type(length) ~= "number" then
        error("Error reading line: " .. tostring(length))
    end

    local eof = not line -- Behaviour of up-to-date LGI
            or (tostring(line) == "" and #line ~= length) -- (Ab)use uninitialized variable
    if eof then
        Gtk.main_quit()
    else
        coro(tostring(line)) -- tostring() needed for older LGI versions
        read_start()
    end
end

read_start()
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
