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
-- This one tiles a shape using the `wibox.widget.serparator` widget:
--
--@DOC_awful_wallpaper_tiled2_EXAMPLE@
--
-- Here is how to tile a small image:
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
-- Keybindings (like `mod4+s`)
-- ===========================
--
--@DOC_awful_wallpaper_keybindings1_EXAMPLE@
--
-- Panning and multi-screen
-- ========================
--
-- TODO
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
local gtimer     = require( "gears.timer"               )
local surface    = require( "gears.surface"             )
local base       = require( "wibox.widget.base"         )
local background = require( "wibox.container.background")
local beautiful  = require( "beautiful"                 )
local cairo      = require( "lgi" ).cairo
local draw       = require( "wibox.widget" ).draw_to_cairo_context

local capi = { screen = screen, root = root }

local module = {}

local function get_screen(s)
    return s and capi.screen[s]
end

-- Screen as key, wallpaper as values.
local pending_repaint = {}

local backgrounds = setmetatable({}, {__mode = 'k'})

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
        cr:paint()
        cr:restore()
    else
        target = cairo.ImageSurface(cairo.Format.RGB32, root_width, root_height)
        cr     = cairo.Context(target)
    end

    for s, wdg in pairs(backgrounds) do
        local wall = wdg._private.wallpaper

        local geo

        if wall.honor_workarea then
            geo = s.workarea
        elseif wall.honor_padding then
            geo = s.tiled_area
        else
            geo = s.geometry
        end

        cr:save()
        cr:translate(geo.x, geo.y)
        draw(wdg, cr, geo.width, geo.height, wall._private.context)
        cr:restore()
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
        paint()
        mutex = false
    end)
end

local function get_container(self, s)
    if backgrounds[s] then
        return backgrounds[s]
    end

    backgrounds[s] = background()
    backgrounds[s].bg = beautiful.wallpaper_bg or self._private.bg or "#000000"
    backgrounds[s].fg = beautiful.wallpaper_fg or self._private.fg or "#ffffff"
    backgrounds[s]._private.wall_screen = s

    return backgrounds[s]
end

capi.screen.connect_signal("removed", function(s)
    if not backgrounds[s] then return end

    backgrounds[s]._private.wallpaper:remove_screen(s)

    backgrounds[s] = nil

    update()
end)

capi.screen.connect_signal("property::geometry", function(s)
    if not backgrounds[s] then return end

    pending_repaint[s] = true
    update()
end)


--- The wallpaper widget.
--
-- When set, instead of using the `image_path` or `surface` properties, the
-- wallpaper will be defined as a normal `wibox` widget tree.
--
-- @property widget
-- @param wibox.widget
-- @see wibox.widget.imagebox
-- @see wibox.container.tile

--- The wallpaper DPI (dots per inch).
--
-- Each screen has a DPI. This value will be used by default, but sometime it
-- is useful to override the screen DPI and use a custom one. This makes
-- possible, for example, to draw the widgets bigger than they would otherwise
-- be.
--
-- @property dpi
-- @param[opt=screen.dpi] number
-- @see screen

--- The wallpaper screen.
--
-- Note that there can only be one wallpaper per screen. If there is more, one
-- will be chosen and all other ignored.
--
-- @property screen
-- @param screen

-- A list of screen for this wallpaper.
--
-- Some large wallpaper are made to span multiple screens.
--

--- The background color.
--
-- It will be used as the "fill" color if the `image` doesn't take all the
-- screen space. It will also be the default background for the `widget.
--
-- As usual with colors in `AwesomeWM`, it can also be a gradient or a pattern.
--
-- @property bg
-- @param gears.color
-- @see gears.color

--- The foreground color.
--
-- This will be used by the `widget` (if any).
--
-- As usual with colors in `AwesomeWM`, it can also be a gradient or a pattern.
--
-- @property fg
-- @param gears.color
-- @see gears.color

--- The default wallpaper background color.
-- @beautiful beautiful.wallpaper_bg
-- @see bg
-- @see beautiful.bg_normal

--- The default wallpaper foreground color.
--
-- This is useful when using widgets or text in the wallpaper. A wallpaper
-- created from a single image wont use this.
--
-- @beautiful beautiful.wallpaper_bg
-- @see bg
-- @see beautiful.bg_normal


--- Honor the workarea.
--
-- When set to `true`, the wallpaper will only fill the workarea space instead
-- of the entire screen. This means it wont be drawn below the `awful.wibar` or
-- docked clients. This is useful when using opaque bars. Note that it can cause
-- aspect ratio issues for the wallpaper `image` and add bars colored with the
-- `bg` color on the sides.
--
-- @property honor_workarea
-- @param[opt=false] boolean

function module:set_widget(w)
    self._private.widget = base.make_widget_from_value(w)

    for _, s in ipairs(self.screens) do
        local bg = get_container(self, s)
        bg.widget = self._private.widget
    end

    self:_update()
end

function module:get_widget()
    return self._private.widget
end

function module:set_dpi(dpi)
    self._private.context.dpi = dpi
    self:_update()
end

function module:get_dpi()
    return self._private.context.dpi
end

function module:set_screen(s)
    if not s then return end

    self:_clear()
    self:add_screen(s)
    self:_update()
end

for _, prop in ipairs {"bg", "fg"} do
    module["set_"..prop] = function(self, color)
        for _, s in ipairs(self.screens) do
            local bg = get_container(self, s)
            bg[prop] = color
        end

        self._private["prop"] = color

        self:_update()
    end
end

function module:set_screens(screens)
    local to_rem = {}

    for _, s in ipairs(screens) do
        to_rem[s] = true
    end

    for _, s in ipairs(self.screens) do
        to_rem[s] = nil
    end

    for _, s in ipairs(screens) do
        self:add_screen(s)
    end

    for s in pairs(to_rem) do
        self:remove_screen(s)
    end
end

function module:get_screens()
    return self._private.screens
end

--- .
-- @method add_screen
function module:add_screen(s)
    s = get_screen(s)

    --TODO remove from other existing wallpaper object

    for _, s2 in ipairs(self._private.screens) do
        if s == s2 then return end
    end

    table.insert(self._private.screens, s)

    local bg = get_container(self, s)

    bg._private.wallpaper = self

    if self.widget then
        bg.widget = self.widget
    end

    self:_update()
end

function module:_clear()
    self._private.screens = {}
    update()
end

function module:_update()
    for _, s in ipairs(self._private.screens) do
        pending_repaint[s] = true
    end

    update()
end

--- .
-- @method remove_screen
function module:remove_screen(s)

    for k, s2 in ipairs(self._private.screens) do
        if s == s2 then
            table.remove(self._private.screens, k)
        end
    end

    self:_update()
end

local function new(_, args)
    args = args or {}
    local ret = gobject {
        enable_auto_signals = true,
        enable_properties   = true,
    }

    rawset(ret, "_private", {})
    ret._private.context = {dpi=96}

    gtable.crush(ret, module, true)

    ret:_clear()

    -- Set the screen or screens first to avoid a race condition
    -- with the other setters.
    if args.screen then
        ret.screen = args.screen
    elseif args.screens then
        ret.screens = args.screens
    end

    gtable.crush(ret, args, false)

    return ret
end

return setmetatable(module, {__call = new})

--TODO multiscreen with single background widget (joint and disjoint) wibox.container.view?
--TODO test gradient panning across multi-screen
--TODO test image panning screen multi-screen
--TODO test solid color
--TODO test gradient
--TODO test multi-screen
--TODO test reolution changes
--TODO test screen added
--TODO test DPI changes
--TODO test tiled
--TODO test centered/aligned
--TODO test stretch
--TODO test resize (no stretch)
--TODO test SVG stretch
--TODO test SVG centered (with multiple DPIs)
--TODO test honor_workarea
--TODO test honor_padding
--TODO test new awful.wallpaper when there's already one for the same screen
