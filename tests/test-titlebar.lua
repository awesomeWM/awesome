local runner = require("_runner")
local titlebar = require("awful.titlebar")
local rules = require("ruled.client")
local spawn = require("awful.spawn")
local gdebug = require("gears.debug")

local tiny_client_code_template = [[
pcall(require, 'luarocks.loader')
local Gtk, class = require('lgi').require('Gtk'), 'client'
Gtk.init()
window = Gtk.Window {default_width=100, default_height=100, title='title'}
%s
window:set_wmclass(class, class)
local app = Gtk.Application {}
function app:on_activate()
    window.application = self
    window:show_all()
end
app:run {''}
]]
local tiny_client = {"lua", "-e", string.format(tiny_client_code_template, "")}
local tiny_client_undecorated = {"lua", "-e",
    string.format(tiny_client_code_template, [[
window.decorated = false
]])
}

-- Use the test client props
local dep = gdebug.deprecate
gdebug.deprecate = function() end
rules.rules = {}
gdebug.deprecate = dep

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

        c:kill()

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

        c:kill()

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

        c:kill()

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

        c:kill()

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

        c:kill()

        return true
    end,
    function()
        if #client.get() ~= 0 then return end
        spawn(tiny_client, { titlebars_enabled = true })
        return true
    end,
    function()
        if #client.get() ~= 1 then return end

        local c = client.get()[1]

        -- Save current client geometries
        local x, y, h, w = c.x, c.y, c.height, c.width

        -- First show all titlebars, then hide them
        for _,method in pairs { 'show', 'hide' } do
            for _,position in pairs { 'top', 'bottom', 'left', 'right' } do
                titlebar[method] {
                    client = c,
                    position = position,
                    resize_client = true
                }

                -- client geometries should be unmodified
                assert(x == c.x and y == c.y and h == c.height and w == c.width)
            end
        end

        c:kill()

        return true
    end,
}

runner.run_steps(steps)

-- vim: filetype=lua:expandtab:shiftwidth=4:tabstop=8:softtabstop=4:textwidth=80
