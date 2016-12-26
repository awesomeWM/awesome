--- Tests for drawing geometry

local runner    = require( "_runner"   )
local awful     = require( "awful"     )
local wibox     = require( "wibox"     )
local beautiful = require( "beautiful" )

local w = nil
local w1_draw, w2_draw

-- Disable automatic placement
awful.rules.rules = {
    { rule = { }, properties = {
    border_width     = 0,
    size_hints_honor = false,
    x                = 0,
    y                = 0,
    width            = 100,
    height           = 100,
    border_color     = beautiful.border_normal
}
    }
}

local steps = {
    function()
        if #client.get() == 0 then
            return true
        end
        for _,c in ipairs(client.get()) do
            c:kill()
        end
    end,
    -- border_color should get applied via focus signal for first client on tag.
    function(count)
        if count == 1 then
            awful.spawn("xterm")
        else
            local c = client.get()[1]
            if c then
                assert(c.size_hints_honor  == false )
                assert(c.border_width      == 0     )
                assert(c:geometry().x      == 0     )
                assert(c:geometry().y      == 0     )
                assert(c:geometry().height == 100   )
                assert(c:geometry().width  == 100   )

                c:kill()

                return true
            end
        end
    end,
    function()
        awful.rules.rules = {
            -- All clients will match this rule.
            { rule = { },properties = {
            titlebars_enabled = true,
            border_width      = 10,
            border_color      = "#00ff00",
            size_hints_honor  = false,
            x                 = 0,
            y                 = 0,
            width             = 100,
            height            = 100
        }
    }
}
return true
    end,
    function(count)
        if count == 1 then
            awful.spawn("xterm")
        else
            local c = client.get()[1]
            if c then
                assert(c.border_width      == 10  )
                assert(c:geometry().x      == 0   )
                assert(c:geometry().y      == 0   )
                assert(c:geometry().height == 100 )
                assert(c:geometry().width  == 100 )

                c.border_width = 20

                return true
            end
        end
    end,
    function()
        local c = client.get()[1]
        assert(c.border_width      == 20  )
        assert(c:geometry().x      == 0   )
        assert(c:geometry().y      == 0   )
        assert(c:geometry().height == 100 )
        assert(c:geometry().width  == 100 )

        c.border_width = 0

        return true
    end,
    function()
        local c = client.get()[1]

        assert(not pcall(function() c.border_width = -2000 end))
        assert(c.border_width==0)

        c.border_width = 125

        return true
    end,
    function()
        local c = client.get()[1]

        assert(c.border_width       == 125 )
        assert(c:geometry().x       == 0   )
        assert(c:geometry().y       == 0   )
        assert(c:geometry().height  == 100 )
        assert(c:geometry().width   == 100 )

        -- So it doesn't hide the other tests
        c:kill()

        return true
    end,
    function()
        w = wibox {
            ontop        = true,
            border_width = 20,
            x            = 100,
            y            = 100,
            width        = 100,
            height       = 100,
            visible      = true,
        }

        assert(w)
        assert(w.border_width == 20  )
        assert(w.width        == 100 )
        assert(w.height       == 100 )
        assert(w.x            == 100 )
        assert(w.y            == 100 )

        w:setup {
            fit    = function(_, _, _, height)
                return height, height -- A square taking the full height
            end,
            draw   = function(_, _, cr, width, height)

                assert(width  == 100)
                assert(height == 100)

                w1_draw = true
                cr:set_source_rgb(1, 0, 0) -- Red
                cr:arc(height/2, height/2, height/2, 0, math.pi*2)
                cr:fill()
            end,
            layout = wibox.widget.base.make_widget,
        }

        return true
    end,
    function()
        w.border_width = 0
        assert(w1_draw)

        return true
    end,
    function()
        assert(w.border_width == 0   )
        assert(w.width        == 100 )
        assert(w.height       == 100 )
        assert(w.x            == 100 )
        assert(w.y            == 100 )

        w:setup {
            fit    = function(_, _, _, height)
                return height, height -- A square taking the full height
            end,
            draw   = function(_, _, cr, width, height)

                assert(width  == 100)
                assert(height == 100)

                w2_draw = true
                cr:set_source_rgb(1, 0, 0) -- Red
                cr:arc(height/2, height/2, height/2, 0, math.pi*2)
                cr:fill()
            end,
            layout = wibox.widget.base.make_widget,
        }

        w.visible = false

        return true
    end,
    function()
        -- Remove the widget before the size change to avoid the asserts
        w:setup {
            fit    = function(_, _, _, height)
                return height, height -- A square taking the full height
            end,
            draw   = function(_, _, cr, _, height)
                cr:set_source_rgb(1, 0, 1) -- Purple
                cr:arc(height/2, height/2, height/2, 0, math.pi*2)
                cr:fill()
            end,
            layout = wibox.widget.base.make_widget,
        }

        w.visible = true
        assert(w2_draw)

        assert(w.border_width == 0   )
        assert(w.width        == 100 )
        assert(w.height       == 100 )
        assert(w.x            == 100 )
        assert(w.y            == 100 )

        w.border_width = 5

        w:geometry {
            x      = 200,
            y      = 200,
            width  = 200,
            height = 200,
        }

        return true
    end,
    function()
        assert(w.border_width == 5   )
        assert(w.width        == 200 )
        assert(w.height       == 200 )
        assert(w.x            == 200 )
        assert(w.y            == 200 )

        return true
    end
}

runner.run_steps(steps)

-- vim: filetype=lua:expandtab:shiftwidth=4:tabstop=8:softtabstop=4:textwidth=80
