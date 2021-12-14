---------------------------------------------------------------------------
-- A calendar popup wibox.
--
-- Display a month or year calendar popup using `calendar_popup.month` or `calendar_popup.year`.
-- The calendar style can be tweaked by providing tables of style properties at creation:
-- `style_year`, `style_month`, `style_yearheader`, `style_header`,
-- `style_weekday`, `style_weeknumber`, `style_normal`, `style_focus` (see `cell_properties`).
--
-- The wibox accepts arguments for the calendar widget: `font`, `spacing`, `week_numbers`,
-- `start_sunday`, `long_weekdays`.
-- It also accepts the extra arguments `opacity`, `bg`, `screen` and `position`.
-- `opacity` and `bg` apply to the wibox itself, they are mainly useful to manage opacity
-- by setting `opacity` for the false opacity or setting `bg="#00000000"` for compositor opacity.
-- The `screen` argument forces the display of the wibox to this screen (instead of the focused screen by default).
-- The `position` argument is a two-characters string describing the screen alignment "[vertical][horizontal]",
-- e.g. "cc", "tr", "bl", ...
--
-- The wibox visibility can be changed calling the `toggle` method.
-- The `attach` method adds mouse bindings to an existing widget in order to toggle the display of the wibox.
--
--@DOC_awful_widget_calendar_popup_default_EXAMPLE@
--
-- @author getzze
-- @copyright 2017 getzze
-- @popupmod awful.widget.calendar_popup
---------------------------------------------------------------------------

local setmetatable = setmetatable
local string = string
local gears = require("gears")
local wibox = require("wibox")
local base = require("wibox.widget.base")
local ascreen = require("awful.screen")
local abutton = require("awful.button")
local beautiful = require("beautiful")

local calendar_popup = { offset = 0, mt = {} }

local properties = { "markup", "fg_color", "bg_color", "shape", "padding", "border_width", "border_color", "opacity" }
local styles = { "year", "month", "yearheader", "monthheader", "header", "weekday", "weeknumber", "normal", "focus" }

--- The generic calendar style table.
--
-- Each table property can also be defined by `beautiful.calendar_[flag]_[property]=val`.
-- @beautiful beautiful.calendar_style
-- @tparam cell_properties table Table of cell style properties

--- Cell properties.
--
-- @DOC_awful_widget_calendar_popup_cell_properties_EXAMPLE@
--
-- @field markup Markup function or format string
-- @field fg_color Text foreground color
-- @field bg_color Text background color
-- @field shape Cell shape
-- @field padding Cell padding
-- @field border_width Calendar border width
-- @field border_color Calendar border color
-- @field opacity Cell opacity
-- @table cell_properties

--- Cell types (flags).
-- @field year Year calendar grid properties table
-- @field month Month calendar grid properties table
-- @field yearheader Year header cell properties table
-- @field header Month header cell properties table (called `monthheader` for a year calendar)
-- @field weekday Weekday cell properties table
-- @field weeknumber Weeknumber cell properties table
-- @field normal Normal day cell properties table
-- @field focus Current day cell properties table
-- @table cell_flags

--- Create a container for the grid layout
-- @tparam table tprops Table of calendar container properties.
-- @treturn function Embedding function widget,flag,date -> widget
local function embed(tprops)
    local function fn (widget, flag, _)
        if flag == "monthheader" and not tprops.monthheader then
            flag = "header"
        end
        local props = tprops[flag]
        -- Markup
        if flag ~= "year" and flag ~= "month" then
            local markup = widget:get_text()
            local m = props.markup
            if type(m) == "function" then
                markup = m(markup)
            elseif type(m) == "string" and string.find(m, "%s", 1, true) then
                markup = string.format(m, markup)
            end
            widget:set_markup(markup)
        end

        local out = base.make_widget_declarative {
            {
                widget,
                margins = props.padding + props.border_width,
                widget  = wibox.container.margin
            },
            shape        = props.shape or gears.shape.rectangle,
            border_color = props.border_color,
            border_width = props.border_width,
            fg           = props.fg_color,
            bg           = props.bg_color,
            opacity      = props.opacity,
            widget       = wibox.container.background
        }
        return out
    end
    return fn
end

--- Parse the properties of the cell type and set default values
-- @tparam string cell The cell type
-- @tparam table args Table of properties to enforce
-- @treturn table The properties table
local function parse_cell_options(cell, args)
    args = args or {}
    local props = {}
    local bl_style = beautiful.calendar_style or {}

    for _, prop in ipairs(properties) do
        local default
        if prop == 'fg_color' then
            default = cell == "focus" and beautiful.fg_focus or beautiful.fg_normal
        elseif prop == 'bg_color' then
            default = cell == "focus" and beautiful.bg_focus or beautiful.bg_normal
        elseif prop == 'padding' then
            default = 2
        elseif prop == 'opacity' then
            default = 1
        elseif prop == 'shape' then
            default = nil
        elseif prop == 'border_width' then
            default = beautiful.border_width or 0
        elseif prop == 'border_color' then
            default = beautiful.border_color_normal or beautiful.fg_normal
        end

        -- Get default
        props[prop] = args[prop] or beautiful["calendar_" .. cell .. "_" .. prop] or bl_style[prop] or default
    end
    if cell == "focus" and props.markup == nil then
        local fg = props.fg_color
            and string.format('foreground="%s"', gears.color.to_rgba_string(props.fg_color)) or ""
        local bg = props.bg_color
            and string.format('background="%s"', gears.color.to_rgba_string(props.bg_color)) or ""
        props.markup = string.format(
            '<span %s %s><b>%s</b></span>',
            fg, bg, "%s"
        )
    end
    return props
end

--- Parse the properties
-- @tparam table args Table of properties
-- @treturn table The properties table
local function parse_all_options(args)
    args = args or {}
    local props = {}

    for _, cell in pairs(styles) do
        if cell~="monthheader" or args.style_monthheader then
            props[cell] = parse_cell_options(cell, args["style_" .. cell])
        end
    end
    return props
end

--- Make the geometry of a wibox
-- @tparam widget widget Calendar widget
-- @tparam object screen Screen where to display the calendar (default to focused)
-- @tparam string position Two characters position of the calendar (default "cc")
-- @treturn number,number,number,number Geometry of the calendar, list of x, y, width, height
local function get_geometry(widget, screen, position)
    local pos = position or "cc"
    local s = screen or ascreen.focused()
    local margin = widget._calendar_margin or 0
    local wa = s.workarea
    local width, height = widget:fit({screen=s, dpi=s.dpi}, wa.width, wa.height)

    width  = width  < wa.width  and width  or wa.width
    height = height < wa.height and height or wa.height


    -- Set to position: pos = tl, tc, tr
    --                        cl, cc, cr
    --                        bl, bc, br
    local x,y
    if pos:sub(1,1) == "t" then
        y = wa.y + margin
    elseif pos:sub(1,1) == "b" then
        y = wa.y + wa.height - (height + margin)
    else  --if pos:sub(1,1) == "c" then
        y = wa.y + math.floor((wa.height - height) / 2)
    end
    if pos:sub(2,2) == "l" then
        x = wa.x + margin
    elseif pos:sub(2,2) == "r" then
        x = wa.x + wa.width - (width + margin)
    else  --if pos:sub(2,2) == "c" then
        x = wa.x + math.floor((wa.width - width) / 2)
    end

    return {x=x, y=y, width=width, height=height}
end

--- Call the calendar with offset
-- @tparam number offset Offset with respect to current month or year
-- @tparam string position Two-character position of the calendar in the screen
-- @tparam screen screen Screen where to display the calendar
-- @treturn wibox The wibox calendar
-- @method call_calendar
function calendar_popup:call_calendar(offset, position, screen)
    local inc_offset = offset or 0
    local pos = position or self.position
    local s = screen or self.screen or ascreen.focused()
    self.position = pos  -- remember last position when changing offset

    self.offset = inc_offset ~= 0 and self.offset + inc_offset or 0

    local widget = self:get_widget()
    local raw_date = os.date("*t")
    local date = {day=raw_date.day, month=raw_date.month, year=raw_date.year}
    if widget._private.type == "month" and self.offset ~= 0 then
        local month_offset = (raw_date.month + self.offset - 1) % 12 + 1
        local year_offset = raw_date.year + math.floor((raw_date.month + self.offset - 1) / 12)
        date = {month=month_offset, year=year_offset }
    elseif widget._private.type == "year" then
        date = {year=raw_date.year + self.offset}
    end

    -- set date and screen before updating geometry
    widget:set_date(date)
    self:set_screen(s)
    -- update geometry (depends on date and screen)
    self:geometry(get_geometry(widget, s, pos))
    return self
end

--- Toggle calendar visibility.
-- @method toggle
function calendar_popup:toggle()
    self:call_calendar(0)
    self.visible = not self.visible
end

--- Attach the calendar to a widget to display at a specific position.
--
--    local mytextclock = wibox.widget.textclock()
--    local month_calendar = awful.widget.calendar_popup.month()
--    month_calendar:attach(mytextclock, 'tr')
--
-- @param widget Widget to attach the calendar
-- @tparam[opt="tr"] string position Two characters string defining the position on the screen
-- @tparam[opt={}] table args Additional options
-- @tparam[opt=true] bool args.on_hover Show popup during mouse hover
-- @treturn wibox The wibox calendar
-- @method attach
function calendar_popup:attach(widget, position, args)
    position = position or "tr"
    args = args or {}
    if args.on_hover == nil then args.on_hover=true end

    widget.buttons = {
        abutton({ }, 1, function ()
                              if not self.visible or self._calendar_clicked_on then
                                  self:call_calendar(0, position)
                                  self.visible = not self.visible
                              end
                              self._calendar_clicked_on = self.visible
                        end),
        abutton({ }, 4, function () self:call_calendar(-1) end),
        abutton({ }, 5, function () self:call_calendar( 1) end)
    }

    if args.on_hover then
        widget:connect_signal("mouse::enter", function ()
            if not self._calendar_clicked_on then
                self:call_calendar(0, position)
                self.visible = true
            end
        end)
        widget:connect_signal("mouse::leave", function ()
            if not self._calendar_clicked_on then
                self.visible = false
            end
        end)
    end
    return self
end

--- Return a new calendar wibox by type.
--
-- A calendar widget displaying a `month` or a `year`
-- @tparam string caltype Type of calendar `month` or `year`
-- @tparam table args Properties of the widget
-- @tparam string args.position Two-character position of the calendar in the screen
-- @tparam screen args.screen Screen where to display the calendar
-- @tparam number args.opacity Wibox opacity
-- @tparam string args.bg Wibox background color
-- @tparam string args.font Calendar font
-- @tparam number args.spacing Calendar spacing
-- @tparam number args.margin Margin around calendar widget
-- @tparam boolean args.week_numbers Show weeknumbers
-- @tparam boolean args.start_sunday Start week on Sunday
-- @tparam boolean args.long_weekdays Format the weekdays with three characters instead of two
-- @tparam table args.style_year Container style for the year calendar (see `cell_properties`)
-- @tparam table args.style_month Container style for the month calendar (see `cell_properties`)
-- @tparam table args.style_yearheader Cell style for the year calendar header (see `cell_properties`)
-- @tparam table args.style_header Cell style for the month calendar header (see `cell_properties`)
-- @tparam table args.style_weekday Cell style for the weekday cells (see `cell_properties`)
-- @tparam table args.style_weeknumber Cell style for the weeknumber cells (see `cell_properties`)
-- @tparam table args.style_normal Cell style for the normal day cells (see `cell_properties`)
-- @tparam table args.style_focus Cell style for the current day cell (see `cell_properties`)
-- @treturn wibox A wibox containing the calendar
local function get_cal_wibox(caltype, args)
    args = args or {}

    local ret = wibox{ ontop   = true,
                       opacity = args.opacity or 1,
                       bg      = args.bg or gears.color.transparent
    }
    gears.table.crush(ret, calendar_popup, false)

    ret.offset   = 0
    ret.position = args.position  or "cc"
    ret.screen   = args.screen

    local widget = wibox.widget {
        font          = args.font or beautiful.font,
        spacing       = args.spacing,
        week_numbers  = args.week_numbers,
        start_sunday  = args.start_sunday,
        long_weekdays = args.long_weekdays,
        fn_embed      = embed(parse_all_options(args)),
        _calendar_margin = args.margin,
        widget = caltype == "year" and wibox.widget.calendar.year or wibox.widget.calendar.month
    }
    ret:set_widget(widget)

    ret.buttons = {
        abutton({ }, 1, function ()
            ret.visible=false
            ret._calendar_clicked_on=false
        end),
        abutton({ }, 3, function ()
            ret.visible=false
            ret._calendar_clicked_on=false
        end),
        abutton({ }, 4, function () ret:call_calendar(-1) end),
        abutton({ }, 5, function () ret:call_calendar( 1) end)
    }

    return ret
end

--- A month calendar wibox.
--
-- It is highly customizable using the same options as for the widgets.
-- The options are set once and for all at creation, though.
--
--@DOC_wibox_awidget_calendar_month_wibox_EXAMPLE@
--
--    local mytextclock = wibox.widget.textclock()
--    local month_calendar = awful.widget.calendar_popup.month()
--    month_calendar:attach( mytextclock, "tr" )
--
-- @tparam table args Properties of the widget
-- @tparam string args.position Two-character position of the calendar in the screen
-- @tparam screen args.screen Screen where to display the calendar
-- @tparam number args.opacity Wibox opacity
-- @tparam string args.bg Wibox background color
-- @tparam string args.font Calendar font
-- @tparam number args.spacing Calendar spacing
-- @tparam number args.margin Margin around calendar widget
-- @tparam boolean args.week_numbers Show weeknumbers
-- @tparam boolean args.start_sunday Start week on Sunday
-- @tparam boolean args.long_weekdays Format the weekdays with three characters instead of two
-- @tparam table args.style_month Container style for the month calendar (see `cell_properties`)
-- @tparam table args.style_header Cell style for the month calendar header (see `cell_properties`)
-- @tparam table args.style_weekday Cell style for the weekday cells (see `cell_properties`)
-- @tparam table args.style_weeknumber Cell style for the weeknumber cells (see `cell_properties`)
-- @tparam table args.style_normal Cell style for the normal day cells (see `cell_properties`)
-- @tparam table args.style_focus Cell style for the current day cell (see `cell_properties`)
-- @treturn wibox A wibox containing the calendar
-- @constructorfct awful.widget.calendar_popup.month
function calendar_popup.month(args)
    return get_cal_wibox("month", args)
end

--- A year calendar wibox.
--
-- It is highly customizable using the same options as for the widgets.
-- The options are set once and for all at creation, though.
--
--@DOC_wibox_awidget_calendar_year_wibox_EXAMPLE@
--
--    globalkeys = gears.table.join(globalkeys, awful.key(
--            { modkey, "Control" }, "c",  function () year_calendar:toggle() end))
--
-- @tparam table args Properties of the widget
-- @tparam string args.position Two-character position of the calendar in the screen
-- @tparam screen args.screen Screen where to display the calendar
-- @tparam number args.opacity Wibox opacity
-- @tparam string args.bg Wibox background color
-- @tparam string args.font Calendar font
-- @tparam number args.spacing Calendar spacing
-- @tparam number args.margin Margin around calendar widget
-- @tparam boolean args.week_numbers Show weeknumbers
-- @tparam boolean args.start_sunday Start week on Sunday
-- @tparam boolean args.long_weekdays Format the weekdays with three characters instead of two
-- @tparam table args.style_year Container style for the year calendar (see `cell_properties`)
-- @tparam table args.style_month Container style for the month calendar (see `cell_properties`).
-- This field can also be called `style_monthheader`.
-- @tparam table args.style_yearheader Cell style for the year calendar header (see `cell_properties`)
-- @tparam table args.style_header Cell style for the month calendar header (see `cell_properties`)
-- @tparam table args.style_weekday Cell style for the weekday cells (see `cell_properties`)
-- @tparam table args.style_weeknumber Cell style for the weeknumber cells (see `cell_properties`)
-- @tparam table args.style_normal Cell style for the normal day cells (see `cell_properties`)
-- @tparam table args.style_focus Cell style for the current day cell (see `cell_properties`)
-- @treturn wibox A wibox containing the calendar
-- @constructorfct awful.widget.calendar_popup.year
function calendar_popup.year(args)
    return get_cal_wibox("year", args)
end

return setmetatable(calendar_popup, calendar_popup.mt)

-- vim: filetype=lua:expandtab:shiftwidth=4:tabstop=8:softtabstop=4:textwidth=80
