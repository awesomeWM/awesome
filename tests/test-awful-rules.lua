local awful = require("awful")
local gears = require("gears")
local beautiful = require("beautiful")
local test_client = require("_client")
local unpack = unpack or table.unpack -- luacheck: globals unpack (compatibility with Lua 5.1)

local callback_called = false

-- Magic table to store tests
local tests = {}

local tb_height = gears.math.round(beautiful.get_font_height() * 1.5)
-- local border_width = beautiful.border_width

-- Detect "manage" race conditions
local real_apply = awful.rules.apply
function awful.rules.apply(c)
    assert(#c:tags() == 0)
    return real_apply(c)
end

local function test_rule(rule)
    rule.rule = rule.rule or {}

    -- Create a random class. The number need to be large as "rule1" and "rule10" would collide
    -- The math.ceil is necessary to remove the floating point, this create an invalid dbus name
    local class = string.format("rule%010d", 42+#tests)
    rule.rule.class = rule.rule.class or class

    test_client(rule.rule.class, rule.properties.name  or "Foo")
    if rule.test then
        local t = rule.test
        table.insert(tests, function() return t(rule.rule.class) end)
        rule.test = nil
    end

    table.insert(awful.rules.rules, rule)
end

-- Helper function to search clients
local function get_client_by_class(class)
    for _, c in ipairs(client.get()) do
        if class == c.class then
            return c
        end
    end
end

-- Test callback and floating
test_rule {
    properties = { floating = true },
    callback   = function(c)
        assert(type(c) == "client")
        callback_called = true
    end,
    test = function(class)
        -- Test if callbacks works
        assert(callback_called)

        -- Make sure "smart" dynamic properties are applied
        assert(get_client_by_class(class).floating)

        -- The size should not have changed
        local geo = get_client_by_class(class):geometry()
        assert(geo.width == 100 and geo.height == 100+tb_height)

        return true
    end
}

-- Test ontop
test_rule {
    properties = { ontop = true },
    test = function(class)
        -- Make sure C-API properties are applied
        assert(get_client_by_class(class).ontop)

        return true
    end
}

-- Test placement
test_rule {
    properties = {
        floating  = true,
        placement = "bottom_right"
    },
    test = function(class)
        -- Test placement
        local sgeo = mouse.screen.workarea
        local c    = get_client_by_class(class)
        local geo  = c:geometry()

        assert(geo.y+geo.height+2*c.border_width == sgeo.y+sgeo.height)
        assert(geo.x+geo.width+2*c.border_width == sgeo.x+sgeo.width)

        return true
    end
}

-- Create a tag named after the class name
test_rule {
    properties = { new_tag=true },
    test = function(class)
        local c_new_tag1 = get_client_by_class(class)

        assert(#c_new_tag1:tags()[1]:clients() == 1)
        assert(c_new_tag1:tags()[1].name == class)

        return true
    end
}

-- Create a tag named Foo Tag with a magnifier layout
test_rule {
    properties = {
        new_tag = {
            name   = "Foo Tag",
            layout = awful.layout.suit.magnifier
        }
    },
    test = function(class)
        local t_new_tag2 = get_client_by_class(class):tags()[1]

        assert( #t_new_tag2:clients()  == 1           )
        assert( t_new_tag2.name        == "Foo Tag"   )
        assert( t_new_tag2.layout.name == "magnifier" )

        return true
    end
}

-- Create a tag named "Bar Tag"
test_rule {
    properties = { new_tag= "Bar Tag" },
    test = function(class)
        local c_new_tag3 = get_client_by_class(class)
        assert(#c_new_tag3:tags()[1]:clients() == 1)
        assert(c_new_tag3:tags()[1].name == "Bar Tag")

        return true
    end
}

-- Test if setting the geometry work
test_rule {
    properties = {
        floating = true,
        x        = 200,
        y        = 200,
        width    = 200,
        height   = 200,
    },
    test = function(class)
        local c   = get_client_by_class(class)
        local geo = c:geometry()

        -- Give it some time
        if geo.y < 180 then return end

        assert(geo.x      == 200)
        assert(geo.y      == 200)
        assert(geo.width  == 200)
        assert(geo.height == 200)

        return true
    end
}

-- Test if setting a partial geometry preserve the other attributes
test_rule {
    properties = {
        floating = true,
        x        = 200,
        geometry = {
            height = 220
        }
    },
    test = function(class)
        local c   = get_client_by_class(class)
        local geo = c:geometry()

        -- Give it some time
        if geo.height < 200 then return end

        assert(geo.x      == 200)
        assert(geo.width  == 100)
        assert(geo.height == 220)

        return true
    end
}

-- Test maximized
test_rule {
    properties = { maximized = true },
    test = function(class)
        local c = get_client_by_class(class)
        -- Make sure C-API properties are applied

        assert(c.maximized)

        local geo = c:geometry()
        local sgeo = c.screen.workarea

        assert(geo.y==sgeo.y)
        assert(geo.x==sgeo.x)

        assert(geo.width+2*c.border_width==sgeo.width)
        assert(geo.height+2*c.border_width==sgeo.height)

        return true
    end
}

-- Test fullscreen
test_rule {
    properties = { fullscreen = true },
    test = function(class)
        local c = get_client_by_class(class)
        -- Make sure C-API properties are applied

        assert(c.fullscreen)

        local geo = c:geometry()
        local sgeo = c.screen.geometry

        assert(geo.x==sgeo.x)
        assert(geo.y==sgeo.y)
        assert(geo.height+2*c.border_width==sgeo.height)
        assert(geo.width+2*c.border_width==sgeo.width)

        return true
    end
}

-- Test tag and switchtotag
test_rule {
    properties = {
        tag         = "9",
        switchtotag = true
    },
    test = function(class)
        local c = get_client_by_class(class)
        -- Make sure C-API properties are applied

        assert(#c:tags() == 1)
        assert(c:tags()[1].name == "9")
        assert(c.screen.selected_tag.name == "9")

        return true
    end
}
test_rule {
    properties = {
        tag         = "8",
        switchtotag = false
    },
    test = function(class)
        local c = get_client_by_class(class)
        -- Make sure C-API properties are applied

        assert(#c:tags() == 1)
        assert(c:tags()[1].name == "8")

        assert(c.screen.selected_tag.name ~= "8")

        return true
    end
}

-- Wait until all the auto-generated clients are ready
local function spawn_clients()
    if #client.get() >= #tests then
        -- Set tiled
        awful.layout.inc(1)
        return true
    end
end

require("_runner").run_steps{spawn_clients, unpack(tests)}

-- vim: filetype=lua:expandtab:shiftwidth=4:tabstop=8:softtabstop=4:textwidth=80
