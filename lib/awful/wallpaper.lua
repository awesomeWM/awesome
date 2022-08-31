---------------------------------------------------------------------------
--- Allows to use the wibox widget system to draw the wallpaper.
--
-- Rather than simply having a function to set an image
-- (stretched, centered or tiled) like most wallpaper tools, this module
-- leverage the full widget system to draw the wallpaper. Note that the result
-- is **not** interactive. If you want an interactive wallpaper, better use
-- a `wibox` object with the `below` property set to `true` and maximized
-- using `awful.placement.maximized`.
--
-- It is possible to create an `awful.wallpaper` object from any places, but
-- it is recommanded to do it from the `request::wallpaper` signal handler.
-- That signal is called everytime something which could affect the wallpaper
-- rendering changes, such as new screens.
--
-- Single image
-- ============
--
-- This is the default `rc.lua` wallpaper format. It fills the whole screen
-- and stretches the image while keeping the aspect ratio.
--
--@DOC_awful_wallpaper_mazimized1_EXAMPLE@
--
-- If the image aspect ratio doesn't match, the `bg` property can be used to
-- fill the empty area:
--
--@DOC_awful_wallpaper_mazimized2_EXAMPLE@
--
-- It is also possible to stretch the image:
--
--@DOC_awful_wallpaper_mazimized3_EXAMPLE@
--
-- Finally, it is also possible to use simpler "branding" in a corner using
-- `awful.placement`:
--
--@DOC_awful_wallpaper_corner1_EXAMPLE@
--
-- Tiled
-- =====
--
-- This example tiles an image:
--
--@DOC_awful_wallpaper_tiled1_EXAMPLE@
--
-- This one tiles a shape using the `wibox.widget.separator` widget:
--
--@DOC_awful_wallpaper_tiled2_EXAMPLE@
--
-- See the `wibox.container.tile` for more advanced tiling configuration
-- options.
--
-- Solid colors and gradients
-- ==========================
--
-- Solid colors can be set using the `bg` property mentionned above. It
-- is also possible to set gradients:
--
--@DOC_awful_wallpaper_gradient1_EXAMPLE@
--
--@DOC_awful_wallpaper_gradient2_EXAMPLE@
--
-- Widgets
-- =======
--
-- It is possible to create a wallpaper using any widgets. However, keep
-- in mind that the wallpaper surface is not interactive, so some widgets
-- like the sliders will render, but will not behave correctly. Also, it
    -- is not recommanded to update the wallpaper too often. This is very slow.
--
--@DOC_awful_wallpaper_widget2_EXAMPLE@
--
-- Cairo graphics API
-- ==================
--
-- AwesomeWM widgets are backed by Cairo. So it is always possible to get
-- access to the Cairo context directly to do some vector art:
--
--@DOC_awful_wallpaper_widget1_EXAMPLE@
--
--
-- SVG vector images
-- =================
--
-- SVG are supported if `librsvg` is installed. Please note that `librsvg`
-- doesn't implement all filters you might find in the latest version of
-- your web browser. It is possible some advanced SVG will not look exactly
-- as they do in a web browser or even Inkscape. However, for most images,
-- it should look identical.
--
-- Our SVG support goes beyond simple rendering. It is possible to set a
-- custom CSS stylesheet (see `wibox.widget.imagebox.stylesheet`):
--
--@DOC_awful_wallpaper_svg_EXAMPLE@
--
-- Note that in the example above, it is raw SVG code, but it is also possible
-- to use a file path. If you have a `.svgz`, you need to uncompress it first
-- using `gunzip` or a software like Inkscape.
--
-- Multiple screen
-- ===============
--
-- The default `rc.lua` creates a new wallpaper everytime `request::wallpaper`
-- is emitted. This is well suited for having a single wallpaper per screen.
-- It is also much simpler to implement slideshows and add/remove screens.
--
-- However, it isn't wall suited for wallpaper rendered across multiple screens.
-- For this case, it is better to capture the return value of `awful.wallpaper {}`
-- as a global variable. Then manually call `add_screen` and `remove_screen` when
-- needed. A shortcut can be to do:
--
-- @DOC_text_awful_wallpaper_multi_screen_EXAMPLE@
--
-- Slideshow
-- =========
--
-- Slideshows (changing the wallpaper after a few minutes) can be implemented
-- directly using a timer and callback, but it is more elegant to simply request
-- a new wallpaper, then get a random image from within the request handler. This
-- way, corner cases such as adding and removing screens are handled:
--
--@DOC_awful_wallpaper_slideshow1_EXAMPLE@
--
-- @author Emmanuel Lepage Vallee &lt;elv1313@gmail.com&gt;
-- @copyright 2019 Emmanuel Lepage Vallee
-- @popupmod awful.wallpaper
---------------------------------------------------------------------------
require("awful._compat")
local gtable     = require( "gears.table"               )
local gobject    = require( "gears.object"              )
local gcolor     = require( "gears.color"               )
local gtimer     = require( "gears.timer"               )
local surface    = require( "gears.surface"             )
local base       = require( "wibox.widget.base"         )
local background = require( "wibox.container.background")
local beautiful  = require( "beautiful"                 )
local cairo      = require( "lgi" ).cairo
local draw       = require( "wibox.widget" ).draw_to_cairo_context
local grect      = require( "gears.geometry" ).rectangle

local capi = { screen = screen, root = root }

local module = {}

local function get_screen(s)
    return s and capi.screen[s]
end

-- Screen as key, wallpaper as values.
local pending_repaint = setmetatable({}, {__mode = 'k'})

local backgrounds = setmetatable({}, {__mode = 'k'})

local panning_modes = {}

-- Get a list of all screen areas.
local function get_rectangles(screens, honor_workarea, honor_padding)
    local ret = {}

    for _, s in ipairs(screens) do
        table.insert(ret, s:get_bounding_geometry {
            honor_padding  = honor_padding,
            honor_workarea = honor_workarea
        })
    end

    return ret
end

-- Outer perimeter of all rectangles.
function panning_modes.outer(self)
    local rectangles = get_rectangles(self.screens, self.honor_workarea, self.honor_padding)
    local p1, p2 = {x = math.huge, y = math.huge}, {x = 0, y = 0}

    for _, rect in ipairs(rectangles) do
        p1.x, p1.y = math.min(p1.x, rect.x),  math.min(p1.y, rect.y)
        p2.x, p2.y = math.max(p2.x, rect.x +  rect.width),  math.max(p2.y, rect.y + rect.height)
    end

    -- Never try to paint this, it would freeze the system.
    assert(p1.x ~= math.huge and p1.y ~= math.huge, "Setting wallpaper failed"..#self.screens)

    return {
        x      = p1.x,
        y      = p1.y,
        width  = p2.x - p1.x,
        height = p2.y - p1.y,
    }
end

-- Horizontal inner perimeter of all rectangles.
function panning_modes.inner_horizontal(self)
    local rectangles = get_rectangles(self.screens, self.honor_workarea, self.honor_padding)
    local p1, p2 = {x = math.huge, y = 0}, {x = 0, y = math.huge}

    for _, rect in ipairs(rectangles) do
        p1.x, p1.y = math.min(p1.x, rect.x),  math.max(p1.y, rect.y)
        p2.x, p2.y = math.max(p2.x, rect.x +  rect.width),  math.min(p2.y, rect.y + rect.height)
    end

    -- Never try to paint this, it would freeze the system.
    assert(p1.x ~= math.huge and p2.y ~= math.huge, "Setting wallpaper failed")

    return {
        x      = p1.x,
        y      = p1.y,
        width  = p2.x - p1.x,
        height = p2.y - p1.y,
    }
end

-- Vertical inner perimeter of all rectangles.
function panning_modes.inner_vertical(self)
    local rectangles = get_rectangles(self.screens, self.honor_workarea, self.honor_padding)
    local p1, p2 = {x = 0, y = math.huge}, {x = math.huge, y = 0}

    for _, rect in ipairs(rectangles) do
        p1.x, p1.y = math.max(p1.x, rect.x),  math.min(p1.y, rect.y)
        p2.x, p2.y = math.min(p2.x, rect.x +  rect.width),  math.max(p2.y, rect.y + rect.height)
    end

    -- Never try to paint this, it would freeze the system.
    assert(p1.y ~= math.huge and p2.a ~= math.huge, "Setting wallpaper failed")

    return {
        x      = p1.x,
        y      = p1.y,
        width  = p2.x - p1.x,
        height = p2.y - p1.y,
    }
end

-- Best or vertical and horizontal "inner" modes.
function panning_modes.inner(self)
    local vert = panning_modes.inner_vertical(self)
    local hori = panning_modes.inner_horizontal(self)

    if vert.width <= 0 or vert.height <= 0 then return hori end
    if hori.width <= 0 or hori.height <= 0 then return vert end

    if vert.width * vert.height > hori.width * hori.height then
        return vert
    else
        return hori
    end
end


local function paint()
    if not next(pending_repaint) then return end

    local root_width, root_height = capi.root.size()

    -- Get the current wallpaper content.
    local source = surface(root.wallpaper())

    local target, cr

    -- It's possible that a wallpaper for 1 screen is set using another tool, so make
    -- sure we copy the current content.
    if source then
        target = source:create_similar(cairo.Content.COLOR, root_width, root_height)
        cr     = cairo.Context(target)

        -- Copy the old wallpaper to the new one
        cr:save()
        cr.operator = cairo.Operator.SOURCE
        cr:set_source_surface(source, 0, 0)

        for s in screen do
            cr:rectangle(
                s.geometry.x,
                s.geometry.y,
                s.geometry.width,
                s.geometry.height
            )
        end

        cr:clip()

        cr:paint()
        cr:restore()
    else
        target = cairo.ImageSurface(cairo.Format.RGB32, root_width, root_height)
        cr     = cairo.Context(target)
    end

    local walls = {}

    for _, wall in pairs(backgrounds) do
        walls[wall] = true
    end

    -- Not supposed to happen, but there is enough API surface for
    -- it to be a side effect of some signals. Calling the panning
    -- mode callback with zero screen is not supported.
    if not next(walls) then
        return
    end

    for wall in pairs(walls) do

        local geo = type(wall._private.panning_area) == "function" and
            wall._private.panning_area(wall) or
            panning_modes[wall._private.panning_area](wall)

        -- If false, this panning area isn't well suited for the screen geometry.
        if geo.width > 0 or geo.height > 0 then
            local uncovered_areas = grect.area_remove(get_rectangles(wall.screens, false, false), geo)

            cr:save()

            -- Prevent overwrite then there is multiple non-continuous screens.
            for _, s in ipairs(wall.screens) do
                cr:rectangle(
                    s.geometry.x,
                    s.geometry.y,
                    s.geometry.width,
                    s.geometry.height
                )
            end

            cr:clip()

            -- The older surface might contain garbage, optionally clean it.
            if wall.uncovered_areas_color then
                cr:set_source(gcolor(wall.uncovered_areas_color))

                for _, area in ipairs(uncovered_areas) do
                    cr:rectangle(area.x, area.y, area.width, area.height)
                    cr:fill()
                end
            end

            if not wall._private.container then
                wall._private.container = background()
                wall._private.container.bg = wall._private.bg or beautiful.wallpaper_bg or "#000000"
                wall._private.container.fg = wall._private.fg or beautiful.wallpaper_fg or "#ffffff"
                wall._private.container.widget = wall.widget
            end

            local a_context = {
                dpi = wall._private.context.dpi
            }

            -- Pick the lowest DPI.
            if not a_context.dpi then
                a_context.dpi = math.huge
                for _, s in ipairs(wall.screens) do
                    a_context.dpi = math.min(
                        s.dpi and s.dpi or s.preferred_dpi, a_context.dpi
                    )
                end
            end

            -- Fallback.
            if not a_context.dpi then
                a_context.dpi = 96
            end

            cr:translate(geo.x, geo.y)
            draw(wall._private.container, cr, geo.width, geo.height, a_context)
            cr:restore()
        end
    end

    -- Set the wallpaper.
    local pattern = cairo.Pattern.create_for_surface(target)
    capi.root.wallpaper(pattern)

    -- Limit some potential GC induced increase in memory usage.
    -- But really, is someone is trying to apply wallpaper changes more
    -- often than the GC is executed, they are doing it wrong.
    target:finish()

end

local mutex = false

-- Uploading the surface to X11 is *very* resource intensive. Given the updates
-- will often happen in batch (like startup), make sure to only do one "real"
-- update.
local function update()
    if mutex then return end

    mutex = true

    gtimer.delayed_call(function()
        -- Remove the mutex first in case `paint()` raises an exception.
        mutex = false
        paint()
    end)
end

capi.screen.connect_signal("removed", function(s)
    if not backgrounds[s] then return end

    backgrounds[s]:remove_screen(s)

    update()
end)

capi.screen.connect_signal("property::geometry", function(s)
    if not backgrounds[s] then return end

    backgrounds[s]:repaint()
end)


--- The wallpaper widget.
--
-- When set, instead of using the `image_path` or `surface` properties, the
-- wallpaper will be defined as a normal `wibox` widget tree.
--
-- @property widget
-- @tparam[opt=nil] widget|nil widget
-- @see wibox.widget.imagebox
-- @see wibox.container.tile

--- The wallpaper DPI (dots per inch).
--
-- Each screen has a DPI. This value will be used by default, but sometime it
-- is useful to override the screen DPI and use a custom one. This makes
-- possible, for example, to draw the widgets bigger than they would otherwise
-- be.
--
-- If not DPI is defined, it will use the smallest DPI from any of the screen.
--
-- In this example, there is 3 screens with DPI of 100, 200 and 300. As you can
-- see, only the text size is affected. Many widgetds are DPI aware, but not all
-- of them. This is either because DPI isn't relevant to them or simply because it
-- isn't supported (like `wibox.widget.graph`).
--
-- @DOC_awful_wallpaper_dpi1_EXAMPLE@
--
-- @property dpi
-- @tparam[opt=self.screen.dpi] number dpi
-- @propertyunit pixel\_per\_inch
-- @negativeallowed false
-- @see screen
-- @see screen.dpi

--- The wallpaper screen.
--
-- Note that there can only be one wallpaper per screen. If there is more, one
-- will be chosen and all other ignored.
--
-- @property screen
-- @tparam screen screen
-- @propertydefault Obtained from the constructor.
-- @see screens
-- @see add_screen
-- @see remove_screen

--- A list of screen for this wallpaper.
--
--@DOC_awful_wallpaper_screens1_EXAMPLE@
--
-- Some large wallpaper are made to span multiple screens.
-- @property screens
-- @tparam[opt={self.screen}] table screens
-- @tablerowtype A list of `screen` objects.
-- @see screen
-- @see add_screen
-- @see remove_screen
-- @see detach

--- The background color.
--
-- It will be used as the "fill" color if the `image` doesn't take all the
-- screen space. It will also be the default background for the `widget`.
--
-- As usual with colors in `AwesomeWM`, it can also be a gradient or a pattern.
--
-- @property bg
-- @tparam[opt=beautiful.wallpaper_bg] color bg
-- @usebeautiful beautiful.wallpaper_bg
-- @see gears.color

--- The foreground color.
--
-- This will be used by the `widget` (if any).
--
-- As usual with colors in `AwesomeWM`, it can also be a gradient or a pattern.
--
-- @property fg
-- @tparam[opt=beautiful.wallpaper_fg] color fg
-- @see gears.color

--- The default wallpaper background color.
-- @beautiful beautiful.wallpaper_bg
-- @tparam color wallpaper_bg
-- @usebeautiful beautiful.wallpaper_fg
-- @see bg

--- The default wallpaper foreground color.
--
-- This is useful when using widgets or text in the wallpaper. A wallpaper
-- created from a single image wont use this.
--
-- @beautiful beautiful.wallpaper_fg
-- @tparam gears.color wallpaper_fg
-- @see bg

--- Honor the workarea.
--
-- When set to `true`, the wallpaper will only fill the workarea space instead
-- of the entire screen. This means it wont be drawn below the `awful.wibar` or
-- docked clients. This is useful when using opaque bars. Note that it can cause
-- aspect ratio issues for the wallpaper `image` and add bars colored with the
-- `bg` color on the sides.
--
--@DOC_awful_wallpaper_workarea1_EXAMPLE@
--
-- @property honor_workarea
-- @tparam[opt=false] boolean honor_workarea
-- @see honor_padding
-- @see uncovered_areas

--- Honor the screen padding.
--
-- When set, this will look at the `screen.padding` property to restrict the
-- area where the wallpaper is rendered.
--
-- @DOC_awful_wallpaper_padding1_EXAMPLE@
--
-- @property honor_padding
-- @tparam[opt=false] boolean honor_padding
-- @see honor_workarea
-- @see uncovered_areas

--- Returns the list of screen(s) area which won't be covered by the wallpaper.
--
-- When `honor_workarea`, `honor_padding` or panning are used, some section of
-- the screen won't have a wallpaper.
--
-- @property uncovered_areas
-- @tparam table uncovered_areas
-- @propertydefault This depends on the `honor_workarea` and `honor_padding` values.
-- @tablerowtype A list of area tables with the following keys:
-- @tablerowkey number x
-- @tablerowkey number y
-- @tablerowkey number width
-- @tablerowkey number height
-- @see honor_workarea
-- @see honor_padding
-- @see uncovered_areas_color

--- The color for the uncovered areas.
--
-- Some application rely on the wallpaper for "fake" transparency. Even if an
-- area is hidden under a wibar (or other clients), its background can still
-- become visible. If you use such application and change your screen geometry
-- often enough, it is possible some areas would become filled with the remains
-- of previous wallpapers. This property allows to clean those areas with a solid
-- color or a gradient.
--
-- @property uncovered_areas_color
-- @tparam[opt="transparent"] color uncovered_areas_color
-- @see uncovered_areas

--- Defines where the wallpaper is placed when there is multiple screens.
--
-- When there is more than 1 screen, it is possible they don't have the same
-- resolution, position or orientation. Panning the wallpaper over them may look
-- better if a continuous rectangle is used rather than creating a virtual rectangle
-- around all screens.
--
-- The default algorithms are:
--
-- **outer:** *(default)*
--
-- Draw an imaginary rectangle around all screens.
--
-- @DOC_awful_wallpaper_panning_outer_EXAMPLE@
--
-- **inner:**
--
-- Take the largest area or either `inner_horizontal` or `inner_vertical`.
--
-- @DOC_awful_wallpaper_panning_inner_EXAMPLE@
--
-- **inner_horizontal:**
--
-- Take the smallest `x` value, the largest `x+width`, the smallest `y`
-- and the smallest `y+height`.
--
-- @DOC_awful_wallpaper_panning_inner_horizontal_EXAMPLE@
--
-- **inner_vertical:**
--
-- Take the smallest `y` value, the largest `y+height`, the smallest `x`
-- and the smallest `x+width`.
--
-- @DOC_awful_wallpaper_panning_inner_vertical_EXAMPLE@
--
-- **Custom function:**
--
-- It is also possible to define a custom function.
--
-- @DOC_awful_wallpaper_panning_custom_EXAMPLE@
--
-- @property panning_area
-- @tparam[opt="outer"] function|string panning_area
-- @propertytype string A panning algorithm
-- @propertyvalue "outer"
-- @propertyvalue "inner"
-- @propertyvalue "inner_horizontal"
-- @propertyvalue "inner_vertical"
-- @propertytype function Custom panning function.
-- @functionparam awful.wallpaper wallpaper The wallpaper object.
-- @functionreturn A table with `x`, `y`, `width` and `height` keys,
-- @see uncovered_areas

function module:set_panning_area(value)
    value = value or "outer"

    assert(type(value) == "function" or panning_modes[value], "Invalid panning mode: "..tostring(value))

    self._private.panning_area = value

    self:repaint()

    self:emit_signal("property::panning_area", value)
end

function module:set_widget(w)
    self._private.widget = base.make_widget_from_value(w)

    if self._private.container then
        self._private.container.widget = self._private.widget
    end

    self:repaint()
end

function module:get_widget()
    return self._private.widget
end

function module:set_dpi(dpi)
    self._private.context.dpi = dpi
    self:repaint()
end

function module:get_dpi()
    return self._private.context.dpi
end

function module:set_screen(s)
    if not s then return end

    self:_clear()
    self:add_screen(s)
end

for _, prop in ipairs {"bg", "fg"} do
    module["set_"..prop] = function(self, color)
        if self._private.container then
            self._private.container[prop] = color
        end

        self._private[prop] = color

        self:repaint()
    end
end

function module:get_uncovered_areas()
    local geo = type(self._private.panning_area) == "function" and
        self._private.panning_area(self) or
        panning_modes[self._private.panning_area](self)

    return grect.area_remove(get_rectangles(self.screens, false, false), geo)
end

function module:set_screens(screens)
    local to_rem = {}

    -- All screens.
    -- The copy is needed because it's a metatable, `ipairs` doesn't work
    -- correctly in all Lua versions.
    if screens == capi.screen then
        screens = {}

        for s in capi.screen do
            table.insert(screens, s)
        end
    end

    for _, s in ipairs(screens) do
        to_rem[get_screen(s)] = true
    end

    for _, s in ipairs(self.screens) do
        to_rem[get_screen(s)] = nil
    end

    for _, s in ipairs(screens) do
        s = get_screen(s)
        self:add_screen(s)
        to_rem[s] = nil
    end

    for s, remove in pairs(to_rem) do
        if remove then
            self:remove_screen(s)
        end
    end
end

function module:get_screens()
    return self._private.screens
end

--- Add another screen (enable panning).
--
-- **Before:**
--
--@DOC_awful_wallpaper_add_screen1_EXAMPLE@
--
-- **After:**
--
--@DOC_awful_wallpaper_add_screen2_EXAMPLE@
--
-- Also note that adding a non-continuous screen might not work well,
-- but will not automatically add the screens in between:
--
--@DOC_awful_wallpaper_add_screen3_EXAMPLE@
--
-- @method add_screen
-- @tparam screen screen The screen object.
-- @noreturn
-- @see remove_screen
function module:add_screen(s)
    s = get_screen(s)

    for _, s2 in ipairs(self._private.screens) do
        if s == s2 then return end
    end

    table.insert(self._private.screens, s)

    if backgrounds[s] and backgrounds[s] ~= self then
        backgrounds[s]:remove_screen(s)
    end

    backgrounds[s] = self

    self:repaint()
end

--- Detach the wallpaper from all screens.
--
-- Adding a new wallpaper to a screen will automatically
-- detach the older one. However there is some case when
-- it is useful to call this manually. For example, when
-- adding a new panned wallpaper, it is possible that 2
-- wallpaper will have an overlap.
--
-- @method detach
-- @noreturn
-- @see remove_screen
-- @see add_screen
function module:detach()
    local screens = gtable.clone(self.screens)

    for _, s in ipairs(screens) do
        self:remove_screen(s)
    end
end

function module:_clear()
    self._private.screens = setmetatable({}, {__mode = "v"})
    update()
end

--- Repaint the wallpaper.
--
-- By default, even if the widget changes, the wallpaper will **NOT** be
-- automatically repainted. Repainting the native X11 wallpaper is slow and
-- it would be too easy to accidentally cause a performance problem. If you
-- really need to repaint the wallpaper, call this method.
--
-- @method repaint
-- @noreturn
function module:repaint()
    for _, s in ipairs(self._private.screens) do
        pending_repaint[s] = true
    end

    update()
end

--- Remove a screen.
--
-- Calling this will remove a screen, but will **not** repaint its area.
-- In this example, the wallpaper was spanning all 3 screens and the
-- first screen was removed:
--
-- @DOC_awful_wallpaper_remove_screen1_EXAMPLE@
--
-- As you can see, the content of screen 1 still looks like it is part of
-- the 3 screen wallpaper. The only use case for calling this method is if
-- you use a 3rd party tools to change the wallpaper.
--
-- If you wish to simply remove a screen and not have leftover content, it is
-- simpler to just create a new wallpaper for that screen:
--
-- @DOC_awful_wallpaper_remove_screen2_EXAMPLE@
--
-- @method remove_screen
-- @tparam screen screen The screen to remove.
-- @treturn boolean `true` if the screen was removed and `false` if the screen
--  wasn't found.
-- @see detach
-- @see add_screen
-- @see screens
function module:remove_screen(s)
    local ret =  false

    s = get_screen(s)

    for k, s2 in ipairs(self._private.screens) do
        if s == s2 then
            table.remove(self._private.screens, k)
            ret = true
        end
    end

    backgrounds[s] = nil

    self:repaint()

    return ret
end

--- Create a wallpaper.
--
-- Note that all parameters are not required. Please refer to the
-- module description and examples to understand parameters usages.
--
-- @constructorfct awful.wallpaper
-- @tparam table args
-- @tparam[opt] wibox.widget args.widget The wallpaper widget.
-- @tparam[opt] number args.dpi The wallpaper DPI (dots per inch).
-- @tparam[opt] screen args.screen The wallpaper screen.
-- @tparam[opt] table args.screens A list of screen for this wallpaper.
--  Use this parameter as a remplacement for `args.screen` to manage multiscreen wallpaper.
--  (Note: the expected table should be an array-like table `{screen1, screen2, ...}`)
-- @tparam[opt] gears.color args.bg The background color.
-- @tparam[opt] gears.color args.fg The foreground color.
-- @tparam[opt] gears.color args.uncovered_areas_color The color for the uncovered areas.
-- @tparam[opt] boolean args.honor_workarea Honor the workarea.
-- @tparam[opt] boolean args.honor_padding Honor the screen padding.
-- @tparam[opt] table args.uncovered_areas Returns the list of screen(s) area which won't be covered by the wallpaper.
-- @tparam[opt] function|string args.panning_area Defines where the wallpaper is placed when there is multiple screens.

local function new(_, args)
    args = args or {}
    local ret = gobject {
        enable_auto_signals = true,
        enable_properties   = true,
    }

    rawset(ret, "_private", {})
    ret._private.context      = {}
    ret._private.panning_area = "outer"

    gtable.crush(ret, module, true)

    ret:_clear()

    -- Set the screen or screens first to avoid a race condition
    -- with the other setters.
    local args_screen, args_screens = args.screen, args.screens
    if args_screen then
        ret.screen = args_screen
    elseif args_screens then
        ret.screens = args_screens
    end

    -- Avoid crushing `screen` and `screens` twice.
    args.screen, args.screens = nil, nil
    gtable.crush(ret, args, false)
    args.screen, args.screens = args_screen, args_screens

    return ret
end

return setmetatable(module, {__call = new})
