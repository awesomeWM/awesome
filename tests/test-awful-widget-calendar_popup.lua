--- Test for awful.widget.calendar

local runner = require("_runner")
local awful = require("awful")
local calendar = require("awful.widget.calendar_popup")

local wa = awful.screen.focused().workarea

local cmonth = calendar.month()
local cyear  = calendar.year()
local current_date = os.date("*t")
local day, month, year = current_date.day, current_date.month, current_date.year

local steps = {
    -- Check center geometry
    function(count)
        if count == 1 then
            cmonth:call_calendar(0, 'cc')
        else
            local geo = cmonth:geometry()
            assert( math.abs((wa.x + wa.width/2.0)  - (geo.x + geo.width/2.0))  < 2 )
            assert( math.abs((wa.y + wa.height/2.0) - (geo.y + geo.height/2.0)) < 2 )
            return true
        end
    end,
    -- Check top-right geometry
    function(count)
        if count == 1 then
            cmonth:call_calendar(0, 'tr')
        else
            local geo = cmonth:geometry()
            assert(wa.x + wa.width  == geo.x + geo.width)
            assert(wa.y == geo.y)
            return true
        end
    end,
    -- Check visibility
    function()
        local v = cyear.visible
        cyear:toggle()
        assert(v == not cyear.visible)
        return true
    end,
    -- Check current date
    function()
        local mdate = cmonth:get_widget():get_date()
        assert(mdate.day==day and mdate.month==month and mdate.year==year)
        local ydate = cyear:get_widget():get_date()
        assert(ydate.year==year)
        return true
    end,
    -- Check new date
    function(count)
        if count == 1 then
            -- Increment
            cmonth:call_calendar(1)
            cyear:call_calendar(-1)
        else
            local mdate = cmonth:get_widget():get_date()
            assert( mdate.day==nil and
                    ((mdate.month==month+1 and mdate.year==year) or (mdate.month==1 and mdate.year==year+1)) )
            local ydate = cyear:get_widget():get_date()
            assert(ydate.year==year-1)
            return true
        end
    end,

}
runner.run_steps(steps)

-- vim: filetype=lua:expandtab:shiftwidth=4:tabstop=8:softtabstop=4:textwidth=80
