---------------------------------------------------------------------------
-- Display a monthly or yearly calendar.
--
-- This module defines two widgets: a month calendar and a year calendar
--
-- The two widgets have a `date` property, in the form of
-- a table {day=[number|nil], month=[number|nil], year=[number]}.
--
-- The `year` widget displays the whole specified year, e.g. {year=2006}.
--
-- The `month` widget displays the calendar for the specified month, e.g. {month=12, year=2006},
-- highlighting the specified day if the day is provided in the date, e.g. {day=22, month=12, year=2006}.
--
-- Cell and container styles can be overridden using the `fn_embed` callback function
-- which is called before adding the widgets to the layouts. The `fn_embed` function
-- takes three arguments, the original widget, the flag (`string` used to identified the widget)
-- and the date (`table`).
-- It returns another widget, embedding (and modifying) the original widget.
--
--@DOC_wibox_widget_defaults_calendar_EXAMPLE@
--
-- @author getzze
-- @copyright 2017 getzze
-- @widgetmod wibox.widget.calendar
---------------------------------------------------------------------------


local setmetatable = setmetatable
local string = string
local gtable = require("gears.table")
local vertical = require("wibox.layout.fixed").vertical
local grid = require("wibox.layout.grid")
local textbox = require("wibox.widget.textbox")
local bgcontainer = require("wibox.container.background")
local base = require("wibox.widget.base")
local beautiful = require("beautiful")

local calendar = { mt = {} }

local properties = { "date"        , "font"         , "spacing" , "week_numbers",
                     "start_sunday", "long_weekdays", "fn_embed", "flex_height",
                   }

--- The calendar font.
-- @beautiful beautiful.calendar_font
-- @tparam string font Font of the calendar

--- The calendar spacing.
-- @beautiful beautiful.calendar_spacing
-- @tparam number spacing Spacing of the grid (twice this value for inter-month spacing)

--- Display the calendar week numbers.
-- @beautiful beautiful.calendar_week_numbers
-- @param boolean Display week numbers

--- Start the week on Sunday.
-- @beautiful beautiful.calendar_start_sunday
-- @param boolean Start the week on Sunday

--- Format the weekdays with three characters instead of two
-- @beautiful beautiful.calendar_long_weekdays
-- @param boolean Use three characters for the weekdays instead of two

--- Allow cells to have flexible height.
-- Flexible height allow cells to adapt their height to fill the empty space at the bottom of the widget.
-- @beautiful beautiful.flex_height
-- @param boolean Cells can skretch to fill the empty space.

--- The calendar date.
--
-- A table representing the date {day=[number|nil], month=[number|nil], year=[number]}.
--
-- E.g.. {day=21, month=2, year=2005}, {month=2, year=2005}, {year=2005}
-- @tparam table date Date table.
-- @tparam number date.year Date year
-- @tparam number|nil date.month Date month
-- @tparam number|nil date.day Date day
-- @property date

--- The calendar font.
--
-- Choose a monospace font for a better rendering.
--
--@DOC_wibox_widget_calendar_font_EXAMPLE@
--
-- @tparam[opt="Monospace 10"] string font Font of the calendar
-- @property font
-- @usebeautiful beautiful.calendar_font

--- The calendar spacing.
--
-- The spacing between cells in the month.
-- The spacing between months in a year calendar is twice this value.
-- @tparam[opt=5] number spacing Spacing of the grid
-- @property spacing
-- @usebeautiful beautiful.calendar_spacing

--- Display the calendar week numbers.
--
--@DOC_wibox_widget_calendar_week_numbers_EXAMPLE@
--
-- @tparam[opt=false] boolean week_numbers Display week numbers
-- @property week_numbers
-- @usebeautiful beautiful.calendar_week_numbers

--- Start the week on Sunday.
--
--@DOC_wibox_widget_calendar_start_sunday_EXAMPLE@
--
-- @tparam[opt=false] boolean start_sunday Start the week on Sunday
-- @property start_sunday
-- @usebeautiful beautiful.calendar_start_sunday

--- Format the weekdays with three characters instead of two
--
--@DOC_wibox_widget_calendar_long_weekdays_EXAMPLE@
--
-- @tparam[opt=false] boolean long_weekdays Use three characters for the weekdays instead of two
-- @property long_weekdays
-- @usebeautiful beautiful.calendar_long_weekdays

--- The widget encapsulating function.
--
-- Function that takes a widget, flag (`string`) and date (`table`) as argument
-- and returns a widget encapsulating the input widget.
--
-- Default value: function (widget, flag, date) return widget end
--
-- It is used to add a container to the grid layout and to the cells:
--
--@DOC_wibox_widget_calendar_fn_embed_cell_EXAMPLE@
-- @tparam function fn_embed Function to embed the widget depending on its flag
-- @property fn_embed

--- Allow cells to have flexible height
--
--@DOC_wibox_widget_calendar_flex_height_EXAMPLE@
--
-- @tparam[opt=false] boolean flex_height Allow flex height.
-- @property flex_height
-- @usebeautiful beautiful.flex_height

--- Make a textbox
-- @tparam string text Text of the textbox
-- @tparam string font Font of the text
-- @tparam boolean center Center the text horizontally
-- @treturn wibox.widget.textbox
local function make_cell(text, font, center)
    local w = textbox()
    w:set_markup(text)
    w:set_align(center and "center" or "right")
    w:set_valign("center")
    w:set_font(font)
    return w
end

--- Create a grid layout with the month calendar
-- @tparam table props Table of calendar properties
-- @tparam table date Date table
-- @tparam number date.year Date year
-- @tparam number date.month Date month
-- @tparam number|nil date.day Date day
-- @treturn widget Grid layout
local function create_month(props, date)
    local start_row    = 3
    local week_start   = props.start_sunday and 1 or 2
    local last_day     = os.date("*t", os.time{year=date.year, month=date.month+1, day=0})
    local month_days   = last_day.day
    local column_fday  = (last_day.wday - month_days + 1 - week_start ) % 7

    local num_columns  = props.week_numbers and 8 or 7
    local start_column = num_columns - 6

    -- Compute number of rows
    -- There are at least 4 weeks in a month
    local num_rows     = 4
    -- On every month but february on non bisextile years
    if last_day.day > 28 then
        -- The number of days span over at least 5 weeks
        num_rows = num_rows + 1

        -- On month with 30+ days add 1 week if:
        -- - if 30 days and the first day is the last day of the week
        -- - if 31 days and the first days is at least the second to last day
        if column_fday >= 5 then
            if last_day.day == 30 and column_fday == 6 or last_day.day == 31 then
                num_rows = num_rows + 1
            end
        end
    -- If the first day of february is anything but the first day of the week
    elseif column_fday > 1 then
        -- Span over 5 weeks
        num_rows = num_rows + 1
    end

    -- Create grid layout
    local layout = grid()
    if props.flex_height then
        layout:set_expand(true)
    end
    layout:set_homogeneous(true)
    layout:set_spacing(props.spacing)
    layout:set_forced_num_rows(num_rows)
    layout:set_forced_num_cols(num_columns)

    --local flags = {"header", "weekdays", "weeknumber", "normal", "focus"}
    local cell_date, t, i, j, w, flag, text

    -- Header
    flag = "header"
    t = os.time{year=date.year, month=date.month, day=1}
    if props.subtype=="monthheader" then
        flag = "monthheader"
        text = os.date("%B", t)
    else
        text = os.date("%B %Y", t)
    end
    w = props.fn_embed(make_cell(text, props.font, true), flag, date)
    layout:add_widget_at(w, 1, 1, 1, num_columns)

    -- Days
    i = start_row
    j = column_fday + start_column
    local current_week = nil
    local drawn_weekdays = 0
    for d=1, month_days do
        cell_date = {year=date.year, month=date.month, day=d}
        t = os.time(cell_date)
        -- Week number
        if props.week_numbers then
            text = os.date("%V", t)
            if tonumber(text) ~= current_week then
                flag = "weeknumber"
                w = props.fn_embed(make_cell(text, props.font), flag, cell_date)
                layout:add_widget_at(w, i, 1, 1, 1)
                current_week = tonumber(text)
            end
        end
        -- Week days
        if drawn_weekdays < 7 then
            flag = "weekday"
            text = os.date("%a", t)
            if not props.long_weekdays then
                text = string.sub(text, 1, 2)
            end
            w = props.fn_embed(make_cell(text, props.font), flag, cell_date)
            layout:add_widget_at(w, 2, j, 1, 1)
            drawn_weekdays = drawn_weekdays +1
        end
        -- Normal day
        flag = "normal"
        text = string.format("%2d", d)
        -- Focus day
        if date.day == d then
            flag = "focus"
            text = "<b>"..text.."</b>"
        end
        w = props.fn_embed(make_cell(text, props.font), flag, cell_date)
        layout:add_widget_at(w, i, j, 1, 1)

        -- find next cell
        i,j = layout:get_next_empty(i,j)
        if j < start_column then j = start_column end
    end
    return props.fn_embed(layout, "month", date)
end


--- Create a grid layout for the year calendar
-- @tparam table props Table of year calendar properties
-- @tparam number|string date Year to display.
-- @treturn widget Grid layout
local function create_year(props, date)
    -- Create a grid widget with the 12 months
    local in_layout = grid()
    in_layout:set_expand(true)
    in_layout:set_homogeneous(true)
    in_layout:set_spacing(2*props.spacing)
    in_layout:set_forced_num_cols(4)
    in_layout:set_forced_num_rows(3)

    local month_date
    local current_date = os.date("*t")

    for month=1,12 do
        if date.year == current_date.year and month == current_date.month then
            month_date = {day=current_date.day, month=current_date.month, year=current_date.year}
        else
            month_date = {month=month, year=date.year}
        end
        in_layout:add(create_month(props, month_date))
    end

    -- Create a vertical layout
    local flag, text = "yearheader", string.format("%s", date.year)
    local year_header = props.fn_embed(make_cell(text, props.font, true), flag, date)
    local out_layout = vertical()
    out_layout:set_spacing(2*props.spacing) -- separate header from calendar grid
    out_layout:add(year_header)
    out_layout:add(in_layout)
    return props.fn_embed(out_layout, "year", date)
end


--- Set the container to the current date
-- @param self Widget to update
local function fill_container(self)
    local date = self._private.date
    if date then
        -- Create calendar grid
        if self._private.type == "month" then
            self._private.container:set_widget(create_month(self._private, date))
        elseif self._private.type == "year" then
            self._private.container:set_widget(create_year(self._private, date))
        end
    else
        self._private.container:set_widget(nil)
    end
    self:emit_signal("widget::layout_changed")
end


-- Set the calendar date
function calendar:set_date(date)
    if date ~= self._private.date then
        self._private.date = date
        -- (Re)create calendar grid
        fill_container(self)
    end
end


-- Build properties function
for _, prop in ipairs(properties) do
    -- setter
    if not calendar["set_" .. prop] then
        calendar["set_" .. prop] = function(self, value)
            if (string.sub(prop,1,3)=="fn_" and type(value) == "function") or self._private[prop] ~= value then
                self._private[prop] = value
                -- (Re)create calendar grid
                fill_container(self)
            end
        end
    end
    -- getter
    if not calendar["get_" .. prop] then
        calendar["get_" .. prop] = function(self)
            return self._private[prop]
        end
    end
end


--- Return a new calendar widget by type.
--
-- @tparam string type Type of the calendar, `year` or `month`
-- @tparam table date Date of the calendar
-- @tparam number date.year Date year
-- @tparam number|nil date.month Date month
-- @tparam number|nil date.day Date day
-- @tparam[opt="Monospace 10"] string font Font of the calendar
-- @treturn widget The calendar widget
local function get_calendar(type, date, font)
    local ct = bgcontainer()
    local ret = base.make_widget(ct, "calendar", {enable_properties = true})
    gtable.crush(ret, calendar, true)

    ret._private.type = type
    ret._private.container = ct

    -- default values
    ret._private.date = date
    ret._private.font = font or beautiful.calendar_font or "Monospace 10"

    ret._private.spacing       = beautiful.calendar_spacing or 5
    ret._private.week_numbers  = beautiful.calendar_week_numbers or false
    ret._private.start_sunday  = beautiful.calendar_start_sunday or false
    ret._private.long_weekdays = beautiful.calendar_long_weekdays or false
    ret._private.flex_height   = beautiful.calendar_flex_height or false
    ret._private.fn_embed      = function (w, _) return w end

    -- header specific
    ret._private.subtype = type=="year" and "monthheader" or "fullheader"

    fill_container(ret)
    return ret
end

--- A month calendar widget.
--
-- A calendar widget is a grid containing the calendar for one month.
-- If the day is specified in the date, its cell is highlighted.
--
--@DOC_wibox_widget_calendar_month_EXAMPLE@
-- @tparam table date Date of the calendar
-- @tparam number date.year Date year
-- @tparam number date.month Date month
-- @tparam number|nil date.day Date day
-- @tparam[opt="Monospace 10"] string font Font of the calendar
-- @treturn widget The month calendar widget
-- @constructorfct wibox.widget.calendar.month
function calendar.month(date, font)
    return get_calendar("month", date, font)
end

--- A year calendar widget.
--
-- A calendar widget is a grid containing the calendar for one year.
--
--@DOC_wibox_widget_calendar_year_EXAMPLE@
-- @tparam table date Date of the calendar
-- @tparam number date.year Date year
-- @tparam number|nil date.month Date month
-- @tparam number|nil date.day Date day
-- @tparam[opt="Monospace 10"] string font Font of the calendar
-- @treturn widget The year calendar widget
-- @constructorfct wibox.widget.calendar.year
function calendar.year(date, font)
    return get_calendar("year", date, font)
end


return setmetatable(calendar, calendar.mt)

-- vim: filetype=lua:expandtab:shiftwidth=4:tabstop=8:softtabstop=4:textwidth=80
