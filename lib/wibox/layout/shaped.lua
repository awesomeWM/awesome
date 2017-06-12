---------------------------------------------------------------------------
-- A specialization of the fixed, flex or ratio layout to support
-- non-rectangular widget slots.
--
--@DOC_wibox_layout_defaults_shaped_EXAMPLE@
-- @author Emmanuel Lepage Vallee &lt;elv1313@gmail.com&gt;
-- @copyright 2016 Emmanuel Lepage Vallee
-- @release @AWESOME_VERSION@
-- @classmod wibox.layout.shaped
---------------------------------------------------------------------------

local wlayout = nil
local util    = require( "awful.util"        )
local color   = require( "gears.color"       )
local shape   = require( "gears.shape"       )
local base    = require( "wibox.widget.base" )

local shaped, layout_map = {}, {}

--TODO add the shift to fit()

local function fixed_layout(self, context, width, height)
    local result, pos,spacing = {}, 0, self._private.spacing
    local bw = self._private.border_width or 0

    for k, v in pairs(self._private.widgets) do
        local x, y, w, h, _ = pos, 0, width - pos, height

        if k ~= #self._private.widgets or not self._private.fill_space then
            w, _ = base.fit_widget(self, context, v, w, h)
            w=w+h/2
        end

        -- Overlap the borders and the extremities
        pos = pos + w + spacing - h/2 - bw
        x = x + bw

        if pos-spacing > width then break end
        table.insert(result, base.place_widget_at(v, x, y, w, h))
    end

    return result
end

local function flex_layout(self, context, width, height)
    local result = {}
    local pos,spacing = 0, self._private.spacing
    local num = #self._private.widgets
    local total_spacing = (spacing*(num-1))

    local space_per_item
    if self._private.dir == "y" then
        space_per_item = height / num - total_spacing/num
    else
        space_per_item = width / num - total_spacing/num
    end

    if self._private.max_widget_size then
        space_per_item = math.min(space_per_item, self._private.max_widget_size)
    end

    for _, v in pairs(self._private.widgets) do
        local x, y, w, h
        if self._private.dir == "y" then
            x, y = 0, util.round(pos)
            w, h = width, floor(space_per_item)
        else
            x, y = util.round(pos), 0
            w, h = floor(space_per_item), height
        end

        table.insert(result, base.place_widget_at(v, x, y, w, h))

        pos = pos + space_per_item + spacing

        if (self._private.dir == "y" and pos-spacing >= height) or
            (self._private.dir ~= "y" and pos-spacing >= width) then
            break
        end
    end

    return result
end

function shaped:layout(...)
    return fixed_layout(self, ...)
end

-- Work for all parent layout types
function shaped:fit(context, width, height)
    local ret_w, ret_h = wlayout.fixed.fit(self, context, width, height)

    -- Add more space for the border subtract the shift
    if ret_w < width then
        local bw = self._private.border_width or 0
        ret_w = ret_w + #self._private.widgets*bw + bw

        -- Overlapping the widgets does same a little space
        ret_w = ret_w - ret_h/2

        ret_w = math.min(width, ret_w)
    end

    return ret_w, ret_h
end

local function get_color(self, index, type)
    if not self._private[type] then return end
    local cols = self._private[type]
    return color(cols[(index-1)%#cols+1])
end

function shaped:before_draw_child(context, index, child, cr, width, height, hi)
    local matrix     = hi:get_matrix_to_parent()
    local x, y, w, h = matrix.x0, matrix.y0, hi:get_size()

    local s = self._private.shape or shape.powerline

    local bw = self._private.border_width --TODO take bw into account in `:layout()`
    local border_color = bw and (get_color(self, index, "border_colors") or
        self._private.border_color) or nil

    local mirror = self._private.mirror_shape

    cr:translate(x, y)

    if mirror then
        cr:translate(w/2, h/2)
        cr:scale(-1,-1)
        cr:translate(-w/2, -h/2)
    end

    s(cr, w, h)

    if bw then
        hi.bg_path   = cr:copy_path()
        hi.bg_matrix = cr:get_matrix()
    end

    cr:clip()

    if mirror then
        cr:translate(w/2, h/2)
        cr:scale(-1,-1)
        cr:translate(-w/2, -h/2)
    end

    cr:translate(-x, -y)

    local bg = get_color(self, index, "bgs")

    -- Avoid changing the source color
    if bg then
        cr:save()
        cr:set_source(bg)
        cr:paint()
        cr:restore()
    end

    -- Change the foreground source color
    local fg = get_color(self, index, "fgs")
    if fg then
        cr:set_source(fg)
    end
end

function shaped:after_draw_child(context, index, child, cr, w, h, hi)
    local path = hi.bg_path

    -- Draw the border
    if path then
        local bw = self._private.border_width
        local border_color = bw and (get_color(self, index, "border_colors") or
            self._private.border_color) or nil

        if border_color then
            cr:save()
            cr:set_matrix(hi.bg_matrix)
            cr:append_path(path)
            cr:set_line_width(2*bw)
            cr:set_source(color(border_color))
            cr:stroke()
            cr:restore()
        end

        hi.bg_path = nil
        hi.matrix = nil
    end

    cr:reset_clip()
end

-- Add extra properties.
for _, prop in ipairs {"shift_start", "shift_end", "border_color", "bgs", "fgs",
    "border_width", "paddings", "shape", "border_colors", "mirror_shape" } do
    shaped["set_"..prop] = function(self, value)
        self._private[prop] = value
        self:emit_signal("property::"..prop)
        self:emit_signal("widget::redraw_needed")
    end
    shaped["get_"..prop] = function(self)
        return self._private[prop]
    end
end

local function init_mapping()
    -- Cyclic dependency... This is hard to avoid as this layout is just
    -- monkeypatching another one
    if not wlayout then
        wlayout = wlayout or require "wibox.layout"

        -- Assign the `:layout()` reverse mapping
        layout_map[ wlayout.fixed.layout ] = fixed_layout
        layout_map[ wlayout.flex.layout  ] = flex_layout
        layout_map[ wlayout.ratio.layout ] = ratio_layout
    end
end

return setmetatable(shaped, {__call = function(_,base, ...)
    -- Only horizontal layouts are currently supported --FIXME

    init_mapping()

    local t, l = type(base), nil

    if not base then
        l = wlayout.fixed.horizontal
    elseif t == "string" and wlayout[base] and wlayout[base].horizontal then
        l = wlayout[base].horizontal
    elseif t == "string" and wlayout[base] then
        l = wlayout[base]
    elseif t == "function" or t == "table" then
        l = wlayout[base]
    else
        l = wlayout.fixed.horizontal
    end

    local ret = l(...)

    return util.table.crush(ret, shaped, true)
end})
