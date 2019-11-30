---------------------------------------------------------------------------
-- Display percentage in a circle.
--
-- Note that this widget makes no attempts to prevent overlapping labels or
-- labels drawn outside of the widget boundaries.
--
--@DOC_wibox_widget_defaults_piechart_EXAMPLE@
-- @author Emmanuel Lepage Valle
-- @copyright 2012 Emmanuel Lepage Vallee
-- @widgetmod wibox.widget.piechart
---------------------------------------------------------------------------

local color     = require( "gears.color"       )
local base      = require( "wibox.widget.base" )
local beautiful = require( "beautiful"         )
local gtable    = require( "gears.table"       )
local pie       = require( "gears.shape"       ).pie
local unpack    = unpack or table.unpack -- luacheck: globals unpack (compatibility with Lua 5.1)

local module = {}

local piechart = {}

local function draw_label(cr,angle,radius,center_x,center_y,text)
    local edge_x = center_x+(radius/2)*math.cos(angle)
    local edge_y = center_y+(radius/2)*math.sin(angle)

    cr:move_to(edge_x, edge_y)

    cr:rel_line_to(radius*math.cos(angle), radius*math.sin(angle))

    local x,y = cr:get_current_point()

    cr:rel_line_to(x > center_x and radius/2 or -radius/2, 0)

    local ext = cr:text_extents(text)

    cr:rel_move_to(
        (x>center_x and radius/2.5 or (-radius/2.5 - ext.width)),
        ext.height/2
    )

    cr:show_text(text) --TODO eventually port away from the toy API
    cr:stroke()

    cr:arc(edge_x, edge_y,2,0,2*math.pi)
    cr:arc(x+(x>center_x and radius/2 or -radius/2),y,2,0,2*math.pi)

    cr:fill()
end

local function compute_sum(data)
    local ret = 0
    for _, entry in ipairs(data) do
        ret = ret + entry[2]
    end

    return ret
end

local function draw(self, _, cr, width, height)
    if not self._private.data_list then return end

    local radius = (height > width and width or height) / 4
    local sum, start, count = compute_sum(self._private.data_list),0,0
    local has_label = self._private.display_labels ~= false

    -- Labels need to be drawn later so the original source is kept
    -- use get_source() wont work are the reference cannot be set from Lua(?)
    local labels = {}

    local border_width = self:get_border_width() or 1
    local border_color = self:get_border_color()
    border_color       = border_color and color(border_color)

    -- Draw the pies
    cr:save()
    cr:set_line_width(border_width)

    -- Alternate from a given sets or colors
    local colors = self:get_colors()
    local col_count = colors and #colors or 0

    for _, entry in ipairs(self._private.data_list) do
        local k, v = entry[1], entry[2]
        local end_angle = start + 2*math.pi*(v/sum)

        local col = colors and color(colors[math.fmod(count,col_count)+1]) or nil

        pie(cr, width, height, start, end_angle, radius)

        if col then
            cr:save()
            cr:set_source(color(col))
        end

        if border_width > 0 then
            if col then
                cr:fill_preserve()
                cr:restore()
            end

            -- By default, it uses the fg color
            if border_color then
                cr:set_source(border_color)
            end
            cr:stroke()
        elseif col then
            cr:fill()
            cr:restore()
        end

        -- Store the label position for later
        if has_label then
            table.insert(labels, {
                --[[angle   ]] start+(end_angle-start)/2,
                --[[radius  ]] radius,
                --[[center_x]] width/2,
                --[[center_y]] height/2,
                --[[text    ]] k,
            })
        end
        start,count = end_angle,count+1
    end
    cr:restore()

    -- Draw the labels
    if has_label then
        for _, v in ipairs(labels) do
            draw_label(cr, unpack(v))
        end
    end
end

local function fit(_, _, width, height)
    return width, height
end

--- The pie chart data list.
--
-- @property data_list
-- @tparam table data_list Sorted table where each entry has a label as its
-- first value and a number as its second value.
-- @propemits false false

--- The pie chart data.
--
-- @property data
-- @tparam table data Labels as keys and number as value.
-- @propemits false false

--- The border color.
--
-- If none is set, it will use current foreground (text) color.
--
--@DOC_wibox_widget_piechart_border_color_EXAMPLE@
-- @property border_color
-- @tparam color border_color
-- @propemits true false
-- @propbeautiful
-- @see gears.color

--- The pie elements border width.
--
--@DOC_wibox_widget_piechart_border_width_EXAMPLE@
-- @property border_width
-- @tparam[opt=1] number border_width
-- @propemits true false
-- @propbeautiful

--- The pie chart colors.
--
-- If no color is set, only the border will be drawn. If less colors than
-- required are set, colors will be re-used in order.
--
-- @property colors
-- @tparam table colors A table of colors, one for each elements.
-- @propemits true false
-- @propbeautiful
-- @see gears.color

--- The border color.
--
-- If none is set, it will use current foreground (text) color.
-- @beautiful beautiful.piechart_border_color
-- @param color
-- @see gears.color

--- If the pie chart has labels.
--
--@DOC_wibox_widget_piechart_label_EXAMPLE@
-- @property display_labels
-- @param[opt=true] boolean

--- The pie elements border width.
--
-- @beautiful beautiful.piechart_border_width
-- @tparam[opt=1] number border_width

--- The pie chart colors.
--
-- If no color is set, only the border will be drawn. If less colors than
-- required are set, colors will be re-used in order.
-- @beautiful beautiful.piechart_colors
-- @tparam table colors A table of colors, one for each elements
-- @see gears.color

for _, prop in ipairs {"data_list", "border_color", "border_width", "colors",
    "display_labels"
  } do
    piechart["set_"..prop] = function(self, value)
        self._private[prop] = value
        self:emit_signal("property::"..prop)
        if prop == "data_list" then
            self:emit_signal("property::data")
        else
            self:emit_signal("property::"..prop, value)
        end
        self:emit_signal("widget::redraw_needed")
    end
    piechart["get_"..prop] = function(self)
        return self._private[prop] or beautiful["piechart_"..prop]
    end
end

function piechart:set_data(value)
    local list = {}
    for k, v in pairs(value) do
        table.insert(list, { k, v })
    end
    self:set_data_list(list)
end

function piechart:get_data()
    local list = {}
    for _, entry in ipairs(self:get_data_list()) do
        list[entry[1]] = entry[2]
    end
    return list
end

--- Create a new piechart.
--
-- @constructorfct wibox.widget.piechart
-- @tparam table data_list The data.

local function new(data_list)

    local ret = base.make_widget(nil, nil, {
        enable_properties = true,
    })

    gtable.crush(ret, piechart)

    rawset(ret, "fit" , fit )
    rawset(ret, "draw", draw)

    ret:set_data_list(data_list)

    return ret
end

--@DOC_widget_COMMON@

--@DOC_object_COMMON@

return setmetatable(module, { __call = function(_, ...) return new(...) end })
-- vim: filetype=lua:expandtab:shiftwidth=4:tabstop=8:softtabstop=4:textwidth=80
