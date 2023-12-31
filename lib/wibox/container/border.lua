---------------------------------------------------------------------------
-- Place widgets or images on the sides, corner and back of another widget.
--
-- This widget main use case is to surround another widget with an uniform
-- border. It can also be used to create soft shadows. It support CSS style
-- image slicing or widgets within the border. The `awful.layout.align` layout
-- an also be used to achieve border, but make it very hard to make them
-- symmetric on both axis.
--
-- Note that because of legacy reasons, `wibox.container.background` also has
-- good support for borders. If you only need simple shaped strokes, the
-- `background` container is a much better choice. This module is better
-- suited for background images and border widgets.
--
-- Advanced usage
-- ==============
--
-- In this first example, 2 `wibox.container.borders` are combined in a
-- `wibox.layout.stack`. The bottom one is the "real" border and the top
-- layer is used to place some widgets. This is useful to built titlebars.
--
-- @DOC_wibox_container_border_dual1_EXAMPLE@
--
-- This example demonstrates how to use this module to create a client
-- border with a top titlebar and borders. It does so by embedding a
-- `wibox.container.border` into another `wibox.container.border`. The outer
-- container acts as the border around the central area while the inner one
-- spans the top area and contains the titlebar itself. The outer border
-- widgets area can be used to implement border resize.
--
-- @DOC_wibox_container_border_titlebar1_EXAMPLE@
--
-- This container is like every other widgets. It is possible to override the
-- Cairo drawing functions. This is very useful here to create custom borders
-- using cairo while still re-using the internal geometry handling code.
--
-- @DOC_wibox_container_border_custom_draw1_EXAMPLE@
--
--@DOC_wibox_container_defaults_border_EXAMPLE@
--
-- @author Emmanuel Lepage-Vallee
-- @copyright 2021 Emmanuel Lepage-Vallee
-- @containermod wibox.container.border
-- @supermodule wibox.widget.base
local gtable   = require("gears.table")
local imagebox = require("wibox.widget.imagebox")
local base     = require("wibox.widget.base")
local gsurface = require("gears.surface")
local cairo    = require("lgi").cairo

local components = {
    "top_left", "top", "top_right", "right", "bottom_right", "bottom",
    "bottom_left", "left", "fill"
}

local slice_modes = {
    top_left     = "corners",
    top          = "sides",
    top_right    = "corners",
    right        = "sides",
    bottom_right = "corners",
    bottom       = "sides",
    bottom_left  = "corners",
    left         = "sides",
    fill         = "filling"
}

local fit_types = { "corners", "sides", "filling" }

local module = {}

local function imagebox_fit(self, _, w, h)
    return math.min(w, self.source_width or 0), math.min(h, self.source_height or 0)
end

local function fit_common(widget, ctx, max_w, max_h)
    if not widget then return 0, 0 end

    local w, h = widget:fit(ctx, max_w or math.huge, max_h or math.huge)

    return w, h
end

local function uses_slice(self)
    return not (self._private.border_widgets or self._private.border_image_widgets)
end

local function get_widget(self, ctx, component)
    local wids, imgs = self._private.border_widgets or {}, self._private.border_image_widgets or {}
    local slices = self._private.slice_cache and self._private.slice_cache[ctx.dpi] or {}

    return wids  [component]
        or imgs  [component]
        or slices[component]
end

local function get_all_widgets(self, partial, ctx)
    for _, position in ipairs(components) do
        partial[position] = partial[position] or get_widget(self, ctx, position)
    end

    return partial
end

--- Common code to load the `border_image`.
local function setup_origin_common(self, ctx)
    if self._private.original_md then return self._private.original_md end

    local origin_w, origin_h, origin

    local img = self._private.border_image

    -- Try SVG first.
    local style  = imagebox._get_stylesheet(self, self._private.border_image_stylesheet)
    local handle = type(img) == "string" and imagebox._load_rsvg_handle(img, style) or nil

    if handle then
        if style then
            handle:set_stylesheet(style)
        end
        handle:set_dpi(self.border_image_dpi or ctx.dpi)
        local dim = handle:get_dimensions()

        if dim.width > 0 and dim.height > 0 then
            origin_w, origin_h = dim.width, dim.height

            origin = cairo.ImageSurface(cairo.Format.ARGB32, origin_w, origin_h)
            local cr = cairo.Context(origin)
            handle:render_cairo(cr)
        end
    end

    if not origin then
        origin = gsurface(self._private.border_image)

        local err1, _origin_w = pcall(function() return origin:get_width() end)
        local err2, _origin_h = pcall(function() return origin:get_height() end)

        if err1 and err2 then
            origin_w, origin_h = _origin_w, _origin_h
        end
    end

    return {
        width   = origin_w,
        height  = origin_h,
        surface = origin,
        handle  = handle
    }
end

local function compute_side_borders(self, ctx, max_w, max_h)
    local override = self._private.borders or {}
    local mer      = self._private.border_merging or {}

    self._private.original_md = setup_origin_common(self, ctx)

    local m = {
        left   = override.left,
        right  = override.right,
        top    = override.top,
        bottom = override.bottom,
    }

    -- Limit the border to what the surface can provide.
    if uses_slice(self) and (m.left or 0) + (m.right or 0) > self._private.original_md.width then
        local max = math.floor((self._private.original_md.width-1)/2)
        m.left  = math.min(m.left, max)
        m.right = math.min(m.right, max)
    end

    -- Limit the border to what the surface can provide.
    if uses_slice(self) and (m.top or 0) + (m.bottom or 0) > self._private.original_md.height then
        local max = math.floor((self._private.original_md.height-1)/2)
        m.top    = math.min(m.top, max)
        m.bottom = math.min(m.bottom, max)
    end

    local wdgs = {
        left   = get_widget(self, ctx, "left"  ) or false,
        right  = get_widget(self, ctx, "right" ) or false,
        top    = get_widget(self, ctx, "top"   ) or false,
        bottom = get_widget(self, ctx, "bottom") or false,
    }

    -- Call `:fit()` on the sides.
    local l_w, l_h = fit_common(wdgs.left  , ctx, max_w, max_h)
    local r_w, r_h = fit_common(wdgs.right , ctx, max_w, max_h)
    local t_w, t_h = fit_common(wdgs.top   , ctx, max_w, max_h)
    local b_w, b_h = fit_common(wdgs.bottom, ctx, max_w, max_h)

    -- Either use the provided borders of the `:fit()` results.
    m.left   = m.left   or l_w
    m.right  = m.right  or r_w
    m.top    = m.top    or t_h
    m.bottom = m.bottom or b_h

    -- Allow the corner widgets to affect the border size.
    if self._private.expand_corners then
        wdgs.top_left     = get_widget(self, ctx, "top_left"    ) or false
        wdgs.top_right    = get_widget(self, ctx, "top_right"   ) or false
        wdgs.bottom_left  = get_widget(self, ctx, "bottom_left" ) or false
        wdgs.bottom_right = get_widget(self, ctx, "bottom_right") or false

        local tl_w, tl_h = fit_common(wdgs.top_left    , ctx, max_w, max_h)
        local tr_w, tr_h = fit_common(wdgs.top_right   , ctx, max_w, max_h)
        local bl_w, bl_h = fit_common(wdgs.bottom_left , ctx, max_w, max_h)
        local br_w, br_h = fit_common(wdgs.bottom_right, ctx, max_w, max_h)

        m.left   = math.max(m.left  , tl_w, bl_w)
        m.right  = math.max(m.right , tr_w, br_w)
        m.top    = math.max(m.top   , tl_h, tr_h)
        m.bottom = math.max(m.bottom, bl_h, br_h)
    end

    -- The sides should have matching size unless merging is enabled.
    local minimums = {
        left   = mer.right  and l_h or math.max(l_h, r_h),
        right  = mer.left   and r_h or math.max(l_h, r_h),
        top    = mer.bottom and t_w or math.max(t_w, b_w),
        bottom = mer.top    and b_w or math.max(t_w, b_w),
    }

    return m, wdgs, minimums
end

local function compute_borders(self, ctx, width, height, fit_width, fit_height)
    local m, widgets, mins = compute_side_borders(self, ctx, width, height)
    local ret = {}

    -- Add some fallback values.
    m.left   = m.left   or 0
    m.right  = m.right  or 0
    m.top    = m.top    or 0
    m.bottom = m.bottom or 0

    -- Take the center widget size into account.
    width  = math.min(width , (fit_width  or 9999) + m.left + m.right)
    height = math.min(height, (fit_height or 9999) + m.top  + m.right)

    -- Corners (w,h,x,y).
    ret.top_left     = {m.left , m.top   , 0              , 0                }
    ret.top_right    = {m.right, m.top   , width - m.right, 0                }
    ret.bottom_left  = {m.left , m.bottom, 0              , height - m.bottom}
    ret.bottom_right = {m.right, m.bottom, width - m.right, height - m.bottom}

    -- Sides (w,h,x,y).
    ret.left   = {m.left                  , height - m.top - m.bottom, 0            , m.top            }
    ret.right  = {m.right                 , height - m.top - m.bottom, width-m.right, m.top            }
    ret.top    = {width - m.left - m.right, m.top                    , m.left       , 0                }
    ret.bottom = {width - m.left - m.right, m.bottom                 , m.left       , height - m.bottom}

    -- Honor the border_widgets `:fit()`
    ret.left  [2] = math.max(mins.left  , ret.left  [2])
    ret.right [2] = math.max(mins.right , ret.right [2])
    ret.top   [1] = math.max(mins.top   , ret.top   [1])
    ret.bottom[1] = math.max(mins.bottom, ret.bottom[1])

    -- Center / fill
    ret.fill = {width - m.left - m.right, height - m.top - m.bottom, m.left, m.top}

    -- Undo what we just did to merge the widgets.
    if self._private.border_merging then
        local to_del = {}

        for _, side in ipairs { "top", "bottom" } do
            if self._private.border_merging[side] then
                ret[side][1] = width
                ret[side][3] = 0

                to_del[side.."_left" ] = true
                to_del[side.."_right"] = true
            end
        end

        for _, side in ipairs { "left", "right" } do
            if self._private.border_merging[side] then
                ret[side][2] = height
                ret[side][4] = 0

                to_del["top_"   ..side] = true
                to_del["bottom_"..side] = true
            end
        end

        for corner in pairs(to_del) do
            ret[corner], widgets[corner] = nil, nil
        end
    end

    return ret, widgets
end

local function init_imagebox(self, ib, border_type)
    ib.horizontal_fit_policy = border_type
    ib.vertical_fit_policy   = border_type
    ib.upscale               = true
    ib.valign                = "center"
    ib.halign                = "center"
    ib.resize                = true
    ib.scaling_quality       = self._private.image_scaling_quality or "nearest"
end

local function slice(self, ctx, borders)
    if not self._private.border_image then
        self._private.sliced_surface = nil
        return
    end

    if not self._private.slice then return end

    -- key: dpi, value: widgets
    self._private.slice_cache = self._private.slice_cache or {}

    if self._private.slice_cache[ctx.dpi] then
        return self._private.slice_cache[ctx.dpi]
    end

    self._private.slice_cache[ctx.dpi] = {}

    local ibs, crs = {}, {}

    local md = setup_origin_common(self, ctx)
    self._private.original_md = md

    if not md.surface then return end

    local scale_w, scale_h = 1, 1

    -- This feature allows to provide very large texture while still sharing
    -- the same assets between standard DPI and HiDPI computers.
    if self._private.border_image_dpi and not md.handle then
        local scale = ctx.dpi/self._private.border_image_dpi
        scale_w, scale_h = scale, scale
    end

    local ARGB32 = cairo.Format.ARGB32

    ibs.top_left     = cairo.ImageSurface(ARGB32, borders.left[1], borders.top[2])
    ibs.top          = cairo.ImageSurface(ARGB32, md.width - borders.left[1] - borders.right[1], borders.top[2])
    ibs.top_right    = cairo.ImageSurface(ARGB32, borders.right[1], borders.top[2])
    ibs.right        = cairo.ImageSurface(ARGB32, borders.right[1], md.height - borders.top[2] - borders.bottom[2])
    ibs.bottom_right = cairo.ImageSurface(ARGB32, borders.right[1], borders.bottom[2])
    ibs.bottom_left  = cairo.ImageSurface(ARGB32, borders.left[1], borders.bottom[2])
    ibs.left         = cairo.ImageSurface(ARGB32, borders.left[1], md.height - borders.top[2] - borders.bottom[2])
    ibs.bottom = cairo.ImageSurface(
        ARGB32,
        md.width - borders.left[1] - borders.right[1],
        borders.bottom[2]
    )
    ibs.fill = cairo.ImageSurface(
        ARGB32,
        md.width - borders.left[1] - borders.right[1],
        md.height - borders.top[2]  - borders.bottom[2]
    )

    for _, position in ipairs(components) do
        crs[position] = cairo.Context(ibs[position])
    end

    if scale_w > 1 or scale_h > 1 then
        md.width, md.height = math.ceil(md.width * scale_w), math.ceil(md.height * scale_h)

        local new_origin = cairo.ImageSurface(ARGB32, md.width, md.height)
        local slice_cr = cairo.Context(new_origin)

        slice_cr:set_source_surface(md.surface)
        slice_cr:scale(scale_w, scale_h)
        slice_cr:paint()

        md.surface = new_origin
    end

    -- Top
    crs.top:translate(-borders.left[1], 0)

    -- Top right
    crs.top_right:translate(-md.width + borders.right[1], 0)

    -- Right
    crs.right:translate(-md.width + borders.right[1], - borders.right[4])

    -- Bottom right
    crs.bottom_right:translate(-md.width + borders.right[1], -md.height + borders.bottom[2])

    -- Bottom
    crs.bottom:translate(-borders.left[1], -md.height + borders.bottom[2])

    -- Bottom left
    crs.bottom_left:translate(0, -md.height + borders.bottom[2])

    -- Left
    crs.left:translate(0, - borders.top_left[2])

    -- Center
    crs.fill:translate(-borders.left[1], -borders.top_left[2])

    for _, position in ipairs(components) do
        crs[position]:set_source_surface(md.surface)
        crs[position]:paint()

        -- Scaling was attempted, but there is still some corner cases like the
        -- `fill` :fit()` causing a negative texture size.
        local ib = imagebox(ibs[position])
        init_imagebox(self, ib, self._private[slice_modes[position].."_fit_policy"])
        self._private.slice_cache[ctx.dpi][position] = ib
    end
end

local function setup_background(self, ctx)
    if not self._private.border_image then
        self._private.sliced_surface = nil
        return
    end

    if self._private.background_widget then
        return self._private.background_widget
    end

    self._private.original_md = setup_origin_common(self, ctx)

    if not self._private.original_md.surface then return end

    local ib = imagebox(self._private.original_md.surface)
    self._private.background_widget = ib
    self._private.background_widget.resize = true
    ib.horizontal_fit_policy = self._private.filling_fit_policy
    ib.vertical_fit_policy   = self._private.filling_fit_policy

    return self._private.background_widget
end

local function children_layout(self, borders, width, height, wdg_w, wdg_h)
    assert(wdg_w > 0 and wdg_h > 0)

    -- This need to be after the other because it has to be on top.
    if self._private.widget then
        local p = self._private.paddings or {}
        if not self._private.honor_borders then
            wdg_w, wdg_h = width - (p.right or 0) - (p.left or 0), height - (p.top or 0) - (p.bottom or 0)
            wdg_w, wdg_h = math.max(0, wdg_w), math.max(0, wdg_h)

            return base.place_widget_at(self._private.widget, 0 + (p.left or 0), 0 + (p.top or 0), wdg_w, wdg_h)
        else
            wdg_w, wdg_h = wdg_w - (p.right or 0) - (p.left or 0), wdg_h - (p.top or 0) - (p.bottom or 0)
            wdg_w, wdg_h = math.max(0, wdg_w), math.max(0, wdg_h)

            return base.place_widget_at(
                self._private.widget, borders.left[1] + (p.left or 0), borders.top[2] + (p.top or 0), wdg_w, wdg_h
            )
        end
    end
end

-- Delayed initialization of the border_images.
local function init_border_images(self)
    self._private.pending_border_images = false
    local value = self._private.border_images

    if not value then return end

    if type(value) ~= "table" then
        local ibs = {}

        for _, t in ipairs(fit_types) do
            local ib = imagebox(value)
            rawset(ib, "fit", imagebox_fit)
            init_imagebox(self, ib, self._private[t.."_fit_policy"])
            ibs[t] = ib
        end

        self._private.border_image_widgets = {}

        for component, fit_type in pairs(slice_modes) do
            self._private.border_image_widgets[component] = ibs[fit_type]
        end
    else
        -- Salvage the old objects.
        local old_widgets = self._private.border_image_widgets

        self._private.border_image_widgets = {}

        for component, img in pairs(value) do
            local k, v = next(old_widgets)

            if v then
                old_widgets[k] = nil
            end

            local ib = v or imagebox()
            rawset(ib, "fit", imagebox_fit)
            local mode = slice_modes[component].."_fit_policy"
            init_imagebox(self, ib, self._private[mode])
            ib.image = img
            self._private.border_image_widgets[component] = ib
        end
    end
end

function module:fit(ctx, width, height)
    if self._private.pending_border_images then
        init_border_images(self)
    end

    local w, h, borders = 0, 0

    if self._private.widget then
        w, h = base.fit_widget(self, ctx, self._private.widget, width, height)

        -- Add padding.
        if w > 0 and h > 0 and self._private.paddings then
            w = w + (self._private.paddings.left or 0) + (self._private.paddings.right  or 0)
            h = h + (self._private.paddings.top  or 0) + (self._private.paddings.bottom or 0)
        end

        assert(w >= 0 and h>=0)

        borders = compute_borders(self, ctx, width, height, w, h)
    else
        borders = compute_borders(self, ctx, width, height, width, height)
    end

    -- Make sure the border `fit` are taken into account.
    w = math.max(borders.top [1], w)
    h = math.max(borders.left[2], h)

    -- Add the borders around the central widget.
    w, h = w + borders.left[1] + borders.right[1], h + borders.top[2] + borders.bottom[2]

    assert(w >= 0 and h >= 0)

    return math.min(width, w), math.min(height, h)
end

function module:layout(ctx, width, height)
    local positioned = {}
    local borders, widgets = compute_borders(self, ctx, width, height)

    local wdg_w = width  - borders.left[1] - borders.right[1]
    local wdg_h = height - borders.top[2]  - borders.bottom[2]

    if self._private.ontop == false and self._private.widget then
        table.insert(positioned, children_layout(self, borders, width, height, wdg_w, wdg_h))
    end

    if self._private.slice then
        slice(self, ctx, borders)
        get_all_widgets(self, widgets, ctx)

        -- Use `ipairs` rather than `pairs` on the widget to keep the order stable.
        for _, position in ipairs(components) do
            local geo = borders[position]
            --TODO use unpack
            if geo and widgets[position] and not (position == "fill" and not self._private.fill) then
                local place = base.place_widget_at(widgets[position], geo[3], geo[4], geo[1], geo[2])
                table.insert(positioned, place)
            end
        end
    else
        local bg = setup_background(self, ctx)

        if bg then
            table.insert(
                positioned,
                base.place_widget_at(bg, 0, 0, width, height)
            )
        end
    end

    if self._private.ontop ~= false and self._private.widget then
        table.insert(positioned, children_layout(self, borders, width, height, wdg_w, wdg_h))
    end

    return positioned
end

--- The widget to display inside of the border.
--
-- @property widget
-- @tparam[opt=nil] widget|nil widget

module.set_widget = base.set_widget_common

function module:get_widget()
    return self._private.widget
end

function module:get_children()
    return {self._private.widget}
end

function module:set_children(children)
    self:set_widget(children[1])
end

--- Reset this layout. The widget will be removed and the rotation reset.
-- @method reset
-- @noreturn
-- @interface container
function module:reset()
    self:set_widget(nil)
end

--- A single border image for the border.
--
-- When using this property, the `borders` also **needs** to be specified.
--
-- @property border_image
-- @tparam[opt=nil] string|image|nil border_image
-- @see borders
-- @see border_images

function module:set_border_image(value)
    if self._private.border_image == value then return end

    self._private.slice_cache = nil
    self._private.original_md = nil
    self._private.background_widget = nil
    self._private.border_image = value
    self:emit_signal("widget::redraw_needed")
end

--- Sice the `border_image` to fit the content.
--
-- This applies a CSS-like modifier to the image. It will use the
-- values of the `borders` property to split the original image into
-- multiple smaller images and use the value of `filling_fit_policy`,
-- `sides_fit_policy` and `corners_fit_policy` to resize the content.
--
-- @DOC_wibox_container_border_slice1_EXAMPLE@
--
-- @property slice
-- @tparam[opt=true] boolean slice
-- @propemits true false

function module:set_slice(value)
    if self._private.slice == value then return end

    self._private.slice = value
    self:emit_signal("property::slice", value)
    self:emit_signal("widget::layout_changed")
    self:emit_signal("widget::redraw_needed")
end

--FIXME DPI support remains undocumented because it has unfixed corner
-- cases.
-- Set the DPI of the slice surface.
--
-- If this is set, the slicer will enable HiDPI mode. It will
-- resize the surface using the ratio created by the effective
-- DPI and the source DPI.
--
-- If the `border_image` is a `.svg` file, this will override the
-- SVG default DPI.
--
-- @DOC_wibox_container_border_dpi1_EXAMPLE@
--
-- @property border_image_dpi
-- @tparam[opt=nil] number border_image_dpi

function module:set_border_image_dpi(value)
    self._private.original_md = nil
    self._private.border_image_dpi = value
    self:emit_signal("widget::redraw_needed")
end

--- Set a stylesheet for the slice surface.
--
-- This only affect `.svg` based assets. It does nothing for `.png`/`.jpg`
-- and already loaded `gears.surfaces` based assets.
--
-- @DOC_wibox_container_border_stylesheet1_EXAMPLE@
--
-- @property border_image_stylesheet
-- @tparam[opt=""] string border_image_stylesheet CSS data or file path.
-- @see wibox.widget.imagebox.stylesheet

function module:set_border_image_stylesheet(value)
    self._private.original_md = nil
    self._private.border_image_stylesheet = value
    self:emit_signal("widget::redraw_needed")
end

--- How the border_image(s) are scaled.
--
-- Note that `nearest` and `best` are the most sensible choices. Other
-- modes may introduce anti-aliasing artifacts at the junction of the various images.
--
--<table class='widget_list' border=1>
-- <tr style='font-weight: bold;'>
--  <th align='center'>Value</th>
--  <th align='center'>Description</th>
-- </tr>
-- <tr><td>fast</td><td>A high-performance filter</td></tr>
-- <tr><td>good</td><td>A reasonable-performance filter</td></tr>
-- <tr><td>best</td><td>The highest-quality available</td></tr>
-- <tr><td>nearest</td><td>Nearest-neighbor filtering (blocky)</td></tr>
-- <tr><td>bilinear</td><td>Linear interpolation in two dimensions</td></tr>
--</table>
--
-- <object data="../images/AUTOGEN_wibox_widget_imagebox_scaling_quality.svg" type="image/svg+xml"></object>
--
-- @property image_scaling_quality
-- @tparam[opt="nearest"] string image_scaling_quality
-- @propertyvalue "fast" A high-performance filter.
-- @propertyvalue "good" A reasonable-performance filter.
-- @propertyvalue "best" The highest-quality available.
-- @propertyvalue "nearest" Nearest-neighbor filtering (blocky).
-- @propertyvalue "bilinear" Linear interpolation in two dimensions.

--- Use images for each of the side/corner/filling sections.
--
-- This property is for using different images for each component
-- of the border. If you want to use a single image, use `border_image`.
--
-- Please note that this is mutually exclusive for each corner or side with
-- `border_widgets`. It has priority over `border_image`.
--
-- @DOC_wibox_container_border_border_images1_EXAMPLE@
--
-- @property border_images
-- @tparam[opt=nil] table|image|nil border_images
-- @tparam[opt=nil] string|image|nil border_images.top_left
-- @tparam[opt=nil] string|image|nil border_images.top
-- @tparam[opt=nil] string|image|nil border_images.top_right
-- @tparam[opt=nil] string|image|nil border_images.right
-- @tparam[opt=nil] string|image|nil border_images.bottom_right
-- @tparam[opt=nil] string|image|nil border_images.bottom
-- @tparam[opt=nil] string|image|nil border_images.bottom_left
-- @tparam[opt=nil] string|image|nil border_images.left
-- @propemits true false
-- @see border_image

function module:set_border_images(value)
    self._private.border_image_widgets = {}
    self._private.original_md = nil
    self._private.border_images = value

    -- Delay the initialization because the fit policy might not be
    -- set yet.
    self._private.pending_border_images = true

    self:emit_signal("property::border_images", value)
    self:emit_signal("widget::redraw_needed")
    self:emit_signal("widget::layout_changed")
end

--- The size of the border on each side.
--
-- Ideally, the value should be smaller than half of the original
-- surface size. For example, if the source is 48x48, then the
-- maximum size should be 23 pixels. If the value exceed this, it
-- will be lowered to the maximum value.
--
-- @DOC_wibox_container_border_borders1_EXAMPLE@
--
-- @property borders
-- @tparam[opt=0] table|number borders
-- @tparam[opt=0] number borders.top
-- @tparam[opt=0] number borders.left
-- @tparam[opt=0] number borders.right
-- @tparam[opt=0] number borders.bottom
-- @negativeallowed false

function module:set_borders(value)

    if type(value) == "number" then
        value = {
            top    = value,
            left   = value,
            right  = value,
            bottom = value,
        }
    end

    self._private.borders = value

    self:emit_signal("widget::layout_changed")
end

--- How the sliced image is resized for the border sides.
--
-- In the following example, the gradient based border works
-- will with `fit` and `pad`. The repeated dot works well with
-- `repeat` and `reflect`. The soft shadow one works regardless
-- of the mode because the borders are identical.
--
-- @DOC_wibox_container_border_sides_fit_policy1_EXAMPLE@
--
-- @property sides_fit_policy
-- @tparam[opt="fit"] string sides_fit_policy
-- @propertyvalue "fit" (default)
-- @propertyvalue "repeat"
-- @propertyvalue "reflect"
-- @propertyvalue "pad"
-- @propemits true false
-- @see wibox.widget.imagebox.vertical_fit_policy
-- @see wibox.widget.imagebox.horizontal_fit_policy

--- How the sliced image is resized for the border filling.
--
-- Also note that if `slice` is set to `false`, this will be used for
-- the entire background.
--
-- @DOC_wibox_container_border_filling_fit_policy1_EXAMPLE@
--
-- @property filling_fit_policy
-- @tparam[opt="fit"] string filling_fit_policy
-- @propertyvalue "fit" (default)
-- @propertyvalue "repeat"
-- @propertyvalue "reflect"
-- @propertyvalue "pad"
-- @propemits true false
-- @see fill
-- @see wibox.widget.imagebox.vertical_fit_policy
-- @see wibox.widget.imagebox.horizontal_fit_policy

--- How the sliced image is resized for the border corners.
--
-- @DOC_wibox_container_border_corners_fit_policy1_EXAMPLE@
--
-- @property corners_fit_policy
-- @tparam[opt="fit"] string corners_fit_policy
-- @propertyvalue "fit" (default)
-- @propertyvalue "repeat"
-- @propertyvalue "reflect"
-- @propertyvalue "pad"
-- @propemits true false
-- @see wibox.widget.imagebox.vertical_fit_policy
-- @see wibox.widget.imagebox.horizontal_fit_policy

for _, mode in ipairs {"corners", "sides", "filling" } do
    module["set_"..mode.."_fit_policy"] = function(self, value)
        for _, widgets in pairs(self._private.slice_cache or {}) do
            for position, widget in pairs(widgets) do
                if slice_modes[position] == mode then
                    widget.horizontal_fit_policy = value
                    widget.vertical_fit_policy   = value
                end
            end
        end

        if mode == "filling" and self._private.background_widget then
            self._private.background_widget.horizontal_fit_policy = value
            self._private.background_widget.horizontal_fit_policy = value
        end

        self._private.pending_border_images = true
        self._private[mode.."_fit_policy"] = value
        self:emit_signal("property::"..mode.."_fit_policy", value)
        self:emit_signal("widget::redraw_needed")
    end
end

--- Stretch the child widget over the entire area.
--
-- By default, the widget honors the borders.
--
--@DOC_wibox_container_border_honor_borders1_EXAMPLE@
--
-- @property honor_borders
-- @tparam[opt=true] boolean honor_borders
-- @propemits true false
-- @see ontop

function module:set_honor_borders(value)
    if self._private.honor_borders == value then return end

    self._private.honor_borders = value
    self:emit_signal("property::honor_borders", value)
    self:emit_signal("widget::layout_changed")
end

--- Draw the child widget below or on top of the border.
--
-- `wibox.container.background` supports border clips because it knows the
-- shape of the border. `wibox.container.border` doesn't have this luxury
-- because the border is defined in an image and the "inner limit" could
-- be any pixel.
--
-- This proporty helps mitigate that by allowing the border to be drawn on
-- top of the child widget when `honor_borders` is set to false. When
-- `honor_borders` is `true`, it does nothing. In the example below, some
-- `paddings` are also necessary to look decent.
--
-- @DOC_wibox_container_border_ontop1_EXAMPLE@
--
-- @property ontop
-- @tparam[opt=true] boolean ontop
-- @propemits true false
-- @see honor_borders
-- @see paddings

function module:set_ontop(value)
    self._private.ontop = value

    self:emit_signal("property::ontop", value)
    self:emit_signal("widget::layout_changed")
end

--- Use the center portion of the `border_image` as a background.
--
-- @DOC_wibox_container_border_fill1_EXAMPLE@
--
-- @property fill
-- @tparam[opt=false] boolean fill
-- @propemits true false
-- @see filling_fit_policy

function module:set_fill(value)
    if self._private.fill == value then return end

    self._private.fill = value
    self:emit_signal("property::fill", value)
    self:emit_signal("widget::redraw_needed")
end

--- Add some space between the border and the inner widget.
--
-- This is pretty much exactly what `wibox.container.margin` would be
-- used for, but since this is a very common use case, this container
-- also has it built-in.
--
-- A common use case for this is asymetric borders such as soft shadows.
--
-- @DOC_wibox_container_border_paddings1_EXAMPLE@
--
-- @property paddings
-- @tparam[opt=0] number|table paddings
-- @propemits true false
-- @negativeallowed false
-- @see wibox.container.margin

function module:set_paddings(value)
    if self._private.paddings == value then return end

    if type(value) == "number" then
        value = {
            left   = value,
            right  = value,
            top    = value,
            bottom = value
        }
    end

    self._private.paddings = value
    self:emit_signal("property::paddings", value)
    self:emit_signal("widget::layout_changed")
end

--- Use individual widgets as a border.
--
-- Please note that this is mutually exclusive for each
-- corner or side with `border_images`. It
-- has priority over `border_image`.
--
-- Please note that if `borders` is not defined, the side widgets
-- (`left`, `right`, `top` and `bottom`) will be used to compute
-- the side of the borders. If you only have corner widgets, then
-- `borders` need to be defined.
--
--@DOC_wibox_container_border_border_widgets1_EXAMPLE@
--
-- @property border_widgets
-- @tparam[opt=nil] table|nil border_widgets
-- @tparam[opt=nil] wibox.widget border_widgets.top_left
-- @tparam[opt=nil] wibox.widget border_widgets.top
-- @tparam[opt=nil] wibox.widget border_widgets.top_right
-- @tparam[opt=nil] wibox.widget border_widgets.right
-- @tparam[opt=nil] wibox.widget border_widgets.bottom_right
-- @tparam[opt=nil] wibox.widget border_widgets.bottom
-- @tparam[opt=nil] wibox.widget border_widgets.bottom_left
-- @tparam[opt=nil] wibox.widget border_widgets.left
-- @propemits true false

function module:set_border_widgets(value)
    local widgets = {}

    if type(value) ~= "table" then
        for _, component in ipairs(components) do
            widgets[component] = value
                and base.make_widget_from_value(value)
                or nil
        end
    else
        for component, widget in pairs(value) do
            widgets[component] = widget
                and base.make_widget_from_value(widget)
                or nil
        end
    end

    self._private.border_widgets = widgets

    self:emit_signal("property::border_widgets", widgets)
    self:emit_signal("widget::redraw_needed")
    self:emit_signal("widget::layout_changed")
end

--- Merge the corners widgets into the side widgets.
--
-- Rather than have corner widgets/images, let the side
-- widget span the entire area. This feature is coupled with
-- `border_widgets` and does nothing when only an image is present.
--
-- @DOC_wibox_container_border_border_merging1_EXAMPLE@
--
-- @property border_merging
-- @tparam table|nil border_merging
-- @tparam[opt=false] boolean border_merging.top Extend the `top` side
--  widget by replacing the `top_left` and `top_right` widgets.
-- @tparam[opt=false] boolean border_merging.bottom Extend the `bottom` side
--  widget by replacing the `bottom_left` and `bottom_right` widgets.
-- @tparam[opt=false] boolean border_merging.left Extend the `left` side
--  widget by replacing the `top_left` and `bottom_left` widgets.
-- @tparam[opt=false] boolean border_merging.right Extend the `right` side
--  widget by replacing the `top_right` and `bottom_right` widgets.
-- @propemits true false

function module:set_border_merging(value)
    self._private.border_merging = value

    self:emit_signal("property::border_widgets", value)
    self:emit_signal("widget::redraw_needed")
    self:emit_signal("widget::layout_changed")
end

--- When `border_widgets` is used, allow the border to grow due to corner widgets.
--
-- @DOC_wibox_container_border_expand_corners1_EXAMPLE@
--
-- @property expand_corners
-- @tparam[opt=false] boolean expand_corners

function module:set_expand_corners(value)
    self._private.expand_corners = value

    self:emit_signal("widget::redraw_needed")
    self:emit_signal("widget::layout_changed")
end

local function new(_, args)
    args = args or {}

    local ret = base.make_widget(nil, nil, {enable_properties = true})
    ret._private.corners_fit_policy    = args.corners_fit_policy or "fit"
    ret._private.sides_fit_policy      = args.slides_fit_policy  or "fit"
    ret._private.filling_fit_policy    = args.filling_fit_policy or "fit"
    ret._private.honor_borders         = args.honor_borders ~= false
    ret._private.fill                  = args.fill or false
    ret._private.slice                 = args.slice == nil and true or args.slice

    gtable.crush(ret, module, true)
    gtable.crush(ret, args, false)

    return ret
end

return setmetatable(module, {__call=new})
