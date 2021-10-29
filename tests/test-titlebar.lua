local runner = require("_runner")
local titlebar = require("awful.titlebar")
local rules = require("ruled.client")
local spawn = require("awful.spawn")
local gdebug = require("gears.debug")
local textbox = require("wibox.widget.textbox")

local lua_executable = os.getenv("LUA")
if lua_executable == nil or lua_executable == "" then
    lua_executable = "lua"
end

local tiny_client_code_template = [[
pcall(require, 'luarocks.loader')
local Gtk, class = require('lgi').require('Gtk'), 'client'
Gtk.init()
window = Gtk.Window {default_width=100, default_height=100, title='title'}
%s
window:set_wmclass(class, class)
local app = Gtk.Application {}
window:show_all()
Gtk:main{...}
app:run {''}
]]
local tiny_client = { lua_executable, "-e", string.format(tiny_client_code_template, "") }
local tiny_client_undecorated = { lua_executable, "-e",
    string.format(tiny_client_code_template, [[
window.decorated = false
]])
}

local found_font, events, next_pos = nil, {}, {}

-- Use the test client props
local dep = gdebug.deprecate
gdebug.deprecate = function() end
rules.rules = {}
gdebug.deprecate = dep

local function kill_client(c)
    -- Make sure the process finishes. Just `c:kill()` only
    -- closes the window. Adding some handlers to the GTK "app"
    -- created some unwanted side effects in the CI.
    awesome.kill(c.pid, 9)
end

local function click()
    local x, y= next_pos.x, next_pos.y
    mouse.coords{x=x, y=y}
    assert(mouse.coords().x == x and mouse.coords().y == y)
    root.fake_input("button_press", 1)
    awesome.sync()
    root.fake_input("button_release", 1)
    awesome.sync()
    next_pos = nil

    return true
end


-- Too bad there's no way to disconnect the rc.lua request::titlebars function

local steps = {
    function()
        assert(#client.get() == 0)
        spawn(tiny_client)
        return true
    end,
    function()
        if #client.get() ~= 1 then return end

        local c = client.get()[1]

        -- The rules don't set any borders nor enable the titlebar
        assert(not c._request_titlebars_called)
        assert(c.width == 100 and c.height == 100)

        -- Should create the top titlebar
        titlebar.toggle(c, "top")

        assert(c._request_titlebars_called)

        local h = c.height
        assert(h > 100)

        -- Should do nothing, there is no titlebar at the bottom by default
        titlebar.toggle(c, "bottom")
        assert(h == c.height)

        -- Should hide the titlebar
        titlebar.toggle(c, "top")
        assert(c.height == 100)

        kill_client(c)

        return true
    end,
    function()
        if #client.get() ~= 0 then return end

        spawn(tiny_client, {titlebars_enabled=true})

        return true
    end,
    function()
        if #client.get() ~= 1 then return end

        local c = client.get()[1]

        local h = c.height
        assert(c.width == 100 and h > 100)
        assert(c._request_titlebars_called)

        titlebar.hide(c, "top")

        assert(c.width == 100 and c.height == 100)

        titlebar.hide(c, "top")

        assert(c.width == 100 and c.height == 100)
        titlebar.show(c, "top")

        assert(c.width == 100 and c.height == h)

        kill_client(c)

        return true
    end,
    function()
        if #client.get() ~= 0 then return end

        spawn(tiny_client_undecorated, {titlebars_enabled=true})

        return true
    end,
    function()
        if #client.get() ~= 1 then return end

        local c = client.get()[1]

        assert(c.width == 100 and c.height > 100)
        assert(c._request_titlebars_called)

        kill_client(c)

        return true
    end,
    function()
        if #client.get() ~= 0 then return end

        spawn(tiny_client, {titlebars_enabled=function(c)
            return not c.requests_no_titlebar
        end})

        return true
    end,
    function()
        if #client.get() ~= 1 then return end

        local c = client.get()[1]

        assert(c.width == 100 and c.height > 100)
        assert(c._request_titlebars_called)

        kill_client(c)

        return true
    end,
    function()
        if #client.get() ~= 0 then return end

        spawn(tiny_client_undecorated, {titlebars_enabled=function(c)
            return not c.requests_no_titlebar
        end})

        return true
    end,
    function()
        if #client.get() ~= 1 then return end

        local c = client.get()[1]

        assert(not c._request_titlebars_called)
        assert(c.width == 100 and c.height == 100)

        function textbox:set_font(value)
            found_font = value
        end

        local args = {size = 40, font = "sans 10", position = "bottom"}
        titlebar(c, args).widget = titlebar.widget.titlewidget(c)

        return true
    end,
    function()
        local c = client.get()[1]

        assert(found_font == "sans 10")

        -- Test the events.
        for _, event in ipairs { "button::press", "button::release", "mouse::move" } do
            c:connect_signal(event, function()
                table.insert(events, event)
            end)
        end

        next_pos = { x = c:geometry().x + 5, y = c:geometry().y + 5 }

        return true
    end,
    click,
    function()
        if #events == 0 then return end

        events = {}

        local c = client.get()[1]

        next_pos = { x = c:geometry().x + 5, y = c:geometry().y + c:geometry().height - 5 }

        return true
    end,
    click,
    function()
        if #events == 0 then return end

        local c = client.get()[1]

        next_pos = {
            x = c:geometry().x + c:geometry().width/2,
            y = c:geometry().y + c:geometry().height/2
        }

        return true
    end,
    click,
    function()
        if #events == 0 then return end

        local c = client.get()[1]

        kill_client(c)

        return true
    end,
    function()
        if #client.get() > 0 then return end

        return true
    end,
}

runner.run_steps(steps)

-- vim: filetype=lua:expandtab:shiftwidth=4:tabstop=8:softtabstop=4:textwidth=80
