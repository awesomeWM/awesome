---------------------------------------------------------------------------
--- The main AwesomeWM "bar" module.
--
-- This module allows you to easily create wibox and attach them to the edge of
-- a screen.
--
--@DOC_awful_wibar_default_EXAMPLE@
--
-- You can even have vertical bars too.
--
--@DOC_awful_wibar_left_EXAMPLE@
--
-- @author Emmanuel Lepage Vallee &lt;elv1313@gmail.com&gt;
-- @copyright 2016 Emmanuel Lepage Vallee
-- @popupmod awful.wibar
-- @supermodule awful.popup
---------------------------------------------------------------------------

-- Grab environment we need
local capi =
{
    screen = screen,
    client = client
}
local setmetatable = setmetatable
local tostring = tostring
local ipairs = ipairs
local error = error
local wibox = require("wibox")
local beautiful = require("beautiful")
local gdebug = require("gears.debug")
local placement = require("awful.placement")
local gtable = require("gears.table")

local function get_screen(s)
    return s and capi.screen[s]
end

local awfulwibar = { mt = {} }

--- Array of table with wiboxes inside.
-- It's an array so it is ordered.
local wiboxes = setmetatable({}, {__mode = "v"})

local opposite_margin = {
    top    = "bottom",
    bottom = "top",
    left   = "right",
    right  = "left"
}

local align_map = {
    top      = "left",
    left     = "top",
    bottom   = "right",
    right    = "bottom",
    centered = "centered"
}

--- If the wibar needs to be stretched to fill the screen.
--
-- @DOC_awful_wibar_stretch_EXAMPLE@
--
-- @property stretch
-- @tparam boolean stretch
-- @propbeautiful
-- @propemits true false
-- @see align

--- How to align non-stretched wibars.
--
-- Values are:
--
--  * `"top"`
--  * `"bottom"`
--  * `"left"`
--  * `"right"`
--  * `"centered"`
--
--  @DOC_awful_wibar_align_EXAMPLE@
--
-- @property align
-- @tparam string align
-- @propbeautiful
-- @propemits true false
-- @see stretch

--- Margins on each side of the wibar.
--
-- It can either be a table with `top`, `bottom`, `left` and `right`
-- properties, or a single number that applies to all four sides.
--
-- @DOC_awful_wibar_margins_EXAMPLE@
--
-- @property margins
-- @tparam number|table margins
-- @propbeautiful
-- @propemits true false

--- If the wibar needs to be stretched to fill the screen.
--
-- @beautiful beautiful.wibar_stretch
-- @tparam boolean stretch

--- Allow or deny the tiled clients to cover the wibar.
--
-- In the example below, we can see that the first screen workarea
-- shrunk by the height of the wibar while the second screen is
-- unchanged.
--
-- @DOC_screen_wibar_workarea_EXAMPLE@
--
-- @property restrict_workarea
-- @tparam[opt=true] boolean restrict_workarea
-- @propemits true false
-- @see client.struts
-- @see screen.workarea

--- If there is both vertical and horizontal wibar, give more space to vertical ones.
--
-- By default, if multiple wibars risk overlapping, it will be resolved
-- by giving more space to the horizontal one:
--
-- ![wibar position](../images/AUTOGEN_awful_wibar_position.svg)
--
-- If this variable is to to `true`, it will behave like:
--
-- @DOC_awful_wibar_position2_EXAMPLE@
--
-- @beautiful beautiful.wibar_favor_vertical
-- @tparam[opt=false] boolean favor_vertical

--- The wibar border width.
-- @beautiful beautiful.wibar_border_width
-- @tparam integer border_width

--- The wibar border color.
-- @beautiful beautiful.wibar_border_color
-- @tparam string border_color

--- If the wibar is to be on top of other windows.
-- @beautiful beautiful.wibar_ontop
-- @tparam boolean ontop

--- The wibar's mouse cursor.
-- @beautiful beautiful.wibar_cursor
-- @tparam string cursor

--- The wibar opacity, between 0 and 1.
-- @beautiful beautiful.wibar_opacity
-- @tparam number opacity

--- The window type (desktop, normal, dock, …).
-- @beautiful beautiful.wibar_type
-- @tparam string type

--- The wibar's width.
-- @beautiful beautiful.wibar_width
-- @tparam integer width

--- The wibar's height.
-- @beautiful beautiful.wibar_height
-- @tparam integer height

--- The wibar's background color.
-- @beautiful beautiful.wibar_bg
-- @tparam color bg

--- The wibar's background image.
-- @beautiful beautiful.wibar_bgimage
-- @tparam surface bgimage

--- The wibar's foreground (text) color.
-- @beautiful beautiful.wibar_fg
-- @tparam color fg

--- The wibar's shape.
-- @beautiful beautiful.wibar_shape
-- @tparam gears.shape shape

--- The wibar's margins.
-- @beautiful beautiful.wibar_margins
-- @tparam number|table margins

--- The wibar's alignments.
-- @beautiful beautiful.wibar_align
-- @tparam string align


-- Compute the margin on one side
local function get_margin(w, position, auto_stop)
    local h_or_w = (position == "top" or position == "bottom") and "height" or "width"
    local ret = 0

    for _, v in ipairs(wiboxes) do
        -- Ignore the wibars placed after this one
        if auto_stop and v == w then break end

        if v.position == position and v.screen == w.screen and v.visible then
            ret = ret + v[h_or_w]

            local wb_margins = v.margins

            if wb_margins then
                ret = ret + wb_margins[position] + wb_margins[opposite_margin[position]]
            end

        end

    end

    return ret
end

-- `honor_workarea` cannot be used as it does modify the workarea itself.
-- a manual padding has to be generated.
local function get_margins(w)
    local position = w.position
    assert(position)

    local margins = gtable.clone(w._private.margins)

    margins[position] =  margins[position] + get_margin(w, position, true)

    -- Avoid overlapping wibars
    if (position == "left" or position == "right") and not beautiful.wibar_favor_vertical then
        margins.top    = get_margin(w, "top"   )
        margins.bottom = get_margin(w, "bottom")
    elseif (position == "top" or position == "bottom") and beautiful.wibar_favor_vertical then
        margins.left  = get_margin(w, "left" )
        margins.right = get_margin(w, "right")
    end

    return margins
end

-- Create the placement function
local function gen_placement(position, align, stretch)
    local maximize = (position == "right" or position == "left") and
        "maximize_vertically" or "maximize_horizontally"

    local corner = nil

    if align ~= "centered" then
        if position == "right" or position == "left" then
            corner = placement[align .. "_" .. position]
                or placement[align_map[align] .. "_" .. position]
        else
            corner = placement[position .. "_" .. align]
                or placement[position .. "_" .. align_map[align]]
        end
    end

    corner = corner or placement[position]

    return corner + (stretch and placement[maximize] or nil)
end

-- Attach the placement function.
local function attach(wb, position)
    gen_placement(position, wb._private.align, wb._stretch)(wb, {
        attach          = true,
        update_workarea = wb._private.restrict_workarea,
        margins         = get_margins(wb)
    })
end

-- Re-attach all wibars on a given wibar screen
local function reattach(wb)
    local s = wb.screen
    for _, w in ipairs(wiboxes) do
        if w ~= wb and w.screen == s then
            if w.detach_callback then
                w.detach_callback()
                w.detach_callback = nil
            end
            attach(w, w.position)
        end
    end
end

--- The wibox position.
--
-- The valid values are:
--
-- * left
-- * right
-- * top
-- * bottom
--
-- @DOC_awful_wibar_position_EXAMPLE@
--
-- @property position
-- @tparam string position Either "left", right", "top" or "bottom"
-- @propemits true false

function awfulwibar.get_position(wb)
    return wb._position or "top"
end

function awfulwibar.set_position(wb, position, screen)
    if position == wb._position then return end

    if screen then
        gdebug.deprecate("Use `wb.screen = screen` instead of awful.wibar.set_position", {deprecated_in=4})
    end

    -- Detach first to avoid any uneeded callbacks
    if wb.detach_callback then
        wb.detach_callback()

        -- Avoid disconnecting twice, this produces a lot of warnings
        wb.detach_callback = nil
    end

    -- Move the wibar to the end of the list to avoid messing up the others in
    -- case there is stacked wibars on one side.
    for k, w in ipairs(wiboxes) do
        if w == wb then
            table.remove(wiboxes, k)
        end
    end
    table.insert(wiboxes, wb)

    -- In case the position changed, it may be necessary to reset the size
    if (wb._position == "left" or wb._position == "right")
      and (position == "top" or position == "bottom") then
        wb.height = math.ceil(beautiful.get_font_height(wb.font) * 1.5)
    elseif (wb._position == "top" or wb._position == "bottom")
      and (position == "left" or position == "right") then
        wb.width = math.ceil(beautiful.get_font_height(wb.font) * 1.5)
    end

    -- Set the new position
    wb._position = position

    -- Attach to the new position
    attach(wb, position)

    -- A way to skip reattach is required when first adding a wibar as it's not
    -- in the `wiboxes` table yet and can't be added until it's attached.
    if not wb._private.skip_reattach then
        -- Changing the position will also cause the other margins to be invalidated.
        -- For example, adding a wibar to the top will change the margins of any left
        -- or right wibars. To solve, this, they need to be re-attached.
        reattach(wb)
    end

    wb:emit_signal("property::position", position)
end

function awfulwibar.get_stretch(w)
    return w._stretch
end

function awfulwibar.set_stretch(w, value)
    w._stretch = value

    attach(w, w.position)

    w:emit_signal("property::stretch", value)
end


function awfulwibar.get_restrict_workarea(w)
    return w._private.restrict_workarea
end

function awfulwibar.set_restrict_workarea(w, value)
    w._private.restrict_workarea = value

    attach(w, w.position)

    w:emit_signal("property::restrict_workarea", value)
end


function awfulwibar.set_margins(w, value)
    if type(value) == "number" then
        value = {
            top = value,
            bottom = value,
            right = value,
            left = value,
        }
    end

    local old = gtable.crush({
        left   = 0,
        right  = 0,
        top    = 0,
        bottom = 0
    }, w._private.margins or {}, true)

   value = gtable.crush(old, value or {}, true)

    w._private.margins = value

    attach(w, w.position)

    w:emit_signal("property::margins", value)
end

-- Allow each margins to be set individually.
local function meta_margins(self)
    return setmetatable({}, {
        __index = self._private.margins,
        __newindex = function(_, k, v)
            self._private.margins[k] = v
            awfulwibar.set_margins(self, self._private.margins)
        end
    })
end

function awfulwibar.get_margins(self)
    return self._private.meta_margins
end


function awfulwibar.get_align(self)
    return self._private.align
end

function awfulwibar.set_align(self, value, screen)
    if value == "center" then
        gdebug.deprecate("awful.wibar.align(wb, 'center' is deprecated, use 'centered'", {deprecated_in=4})
        value = "centered"
    end

    if screen then
        gdebug.deprecate("awful.wibar.align 'screen' argument is deprecated", {deprecated_in=4})
    end

    assert(align_map[value])

    self._private.align = value

    attach(self, self.position)

    self:emit_signal("property::align", value)
end

--- Remove a wibar.
-- @method remove

function awfulwibar.remove(self)
    self.visible = false

    if self.detach_callback then
        self.detach_callback()
        self.detach_callback = nil
    end

    for k, w in ipairs(wiboxes) do
        if w == self then
            table.remove(wiboxes, k)
        end
    end

    self._screen = nil
end

--- Attach a wibox to a screen.
--
-- This function has been moved to the `awful.placement` module. Calling this
-- no longer does anything.
--
-- @param wb The wibox to attach.
-- @param position The position of the wibox: top, bottom, left or right.
-- @param screen The screen to attach to
-- @see awful.placement
-- @deprecated awful.wibar.attach
local function legacy_attach(wb, position, screen) --luacheck: no unused args
    gdebug.deprecate("awful.wibar.attach is deprecated, use the 'attach' property"..
        " of awful.placement. This method doesn't do anything anymore",
        {deprecated_in=4}
    )
end

--- Align a wibox.
--
-- Supported alignment are:
--
-- * top_left
-- * top_right
-- * bottom_left
-- * bottom_right
-- * left
-- * right
-- * top
-- * bottom
-- * centered
-- * center_vertical
-- * center_horizontal
--
-- @param wb The wibox.
-- @param align The alignment
-- @param screen This argument is deprecated. It is not used. Use wb.screen
--  directly.
-- @deprecated awful.wibar.align
-- @see awful.placement.align
local function legacy_align(wb, align, screen) --luacheck: no unused args
    if align == "center" then
        gdebug.deprecate("awful.wibar.align(wb, 'center' is deprecated, use 'centered'", {deprecated_in=4})
        align = "centered"
    end

    if screen then
        gdebug.deprecate("awful.wibar.align 'screen' argument is deprecated", {deprecated_in=4})
    end

    if placement[align] then
        return placement[align](wb)
    end
end

--- Stretch a wibox so it takes all screen width or height.
--
-- **This function has been removed.**
--
-- @deprecated awful.wibox.stretch
-- @see awful.placement
-- @see awful.wibar.stretch

--- Create a new wibox and attach it to a screen edge.
-- You can add also position key with value top, bottom, left or right.
-- You can also use width or height in % and set align to center, right or left.
-- You can also set the screen key with a screen number to attach the wibox.
-- If not specified, the primary screen is assumed.
-- @see wibox
-- @tparam[opt=nil] table args
-- @tparam string args.position The position.
-- @tparam string args.stretch If the wibar need to be stretched to fill the screen.
-- @tparam boolean args.restrict_workarea Allow or deny the tiled clients to cover the wibar.
-- @tparam string args.align How to align non-stretched wibars.
-- @tparam table|number args.margins The wibar margins.
--@DOC_wibox_constructor_COMMON@
-- @return The new wibar
-- @constructorfct awful.wibar
function awfulwibar.new(args)
    args = args or {}
    local position = args.position or "top"
    local has_to_stretch = true
    local screen = get_screen(args.screen or 1)

    args.type = args.type or "dock"

    if position ~= "top" and position ~="bottom"
            and position ~= "left" and position ~= "right" then
        error("Invalid position in awful.wibar(), you may only use"
            .. " 'top', 'bottom', 'left' and 'right'")
    end

    -- Set default size
    if position == "left" or position == "right" then
        args.width = args.width or beautiful["wibar_width"]
            or math.ceil(beautiful.get_font_height(args.font) * 1.5)
        if args.height then
            has_to_stretch = false
            if args.screen then
                local hp = tostring(args.height):match("(%d+)%%")
                if hp then
                    args.height = math.ceil(screen.geometry.height * hp / 100)
                end
            end
        end
    else
        args.height = args.height or beautiful["wibar_height"]
            or math.ceil(beautiful.get_font_height(args.font) * 1.5)
        if args.width then
            has_to_stretch = false
            if args.screen then
                local wp = tostring(args.width):match("(%d+)%%")
                if wp then
                    args.width = math.ceil(screen.geometry.width * wp / 100)
                end
            end
        end
    end

    args.screen = nil

    -- The C code scans the table directly, so metatable magic cannot be used.
    for _, prop in ipairs {
        "border_width", "border_color", "font", "opacity", "ontop", "cursor",
        "bgimage", "bg", "fg", "type", "stretch", "shape", "margins", "align"
    } do
        if (args[prop] == nil) and beautiful["wibar_"..prop] ~= nil then
            args[prop] = beautiful["wibar_"..prop]
        end
    end

    local w = wibox(args)

    w._private.align = (args.align and align_map[args.align]) and args.align or "centered"

    w._private.margins = {
        left   = 0,
        right  = 0,
        top    = 0,
        bottom = 0
    }
    w._private.meta_margins = meta_margins(w)

    w._private.restrict_workarea = true

    -- `w` needs to be inserted in `wiboxes` before reattach or its own offset
    -- will not be taken into account by the "older" wibars when `reattach` is
    -- called. `skip_reattach` is required.
    w._private.skip_reattach = true


    w.screen   = screen
    w._screen  = screen --HACK When a screen is removed, then getbycoords wont work
    w._stretch = args.stretch == nil and has_to_stretch or args.stretch

    if args.visible == nil then w.visible = true end

    gtable.crush(w, awfulwibar, true)
    gtable.crush(w, args, false)

    -- Now, let set_position behave normally.
    w._private.skip_reattach = false

    awfulwibar.set_margins(w, args.margins)

    -- Force all the wibars to be moved
    reattach(w)

    w:connect_signal("property::visible", function() reattach(w) end)

    assert(w.buttons)

    return w
end

capi.screen.connect_signal("removed", function(s)
    local wibars = {}
    for _, wibar in ipairs(wiboxes) do
        if wibar._screen == s then
            table.insert(wibars, wibar)
        end
    end
    for _, wibar in ipairs(wibars) do
        wibar:remove()
    end
end)

function awfulwibar.mt:__call(...)
    return awfulwibar.new(...)
end

function awfulwibar.mt:__index(_, k)
    if k == "align" then
        return legacy_align
    elseif k == "attach" then
        return legacy_attach
    end
end

--@DOC_wibox_COMMON@

--@DOC_object_COMMON@

return setmetatable(awfulwibar, awfulwibar.mt)

-- vim: filetype=lua:expandtab:shiftwidth=4:tabstop=8:softtabstop=4:textwidth=80
