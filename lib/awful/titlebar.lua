---------------------------------------------------------------------------
--- Create widget area on the edge of a client.
--
-- Create a titlebar
-- =================
--
-- This example reproduces what the default `rc.lua` does. It shows how to
-- handle the titlebars on a lower level.
--
-- @DOC_awful_titlebar_default_EXAMPLE@
--
-- @author Uli Schlachter
-- @copyright 2012 Uli Schlachter
-- @popupmod awful.titlebar
---------------------------------------------------------------------------

local error = error
local pairs = pairs
local table = table
local type = type
local gmath = require("gears.math")
local abutton = require("awful.button")
local aclient = require("awful.client")
local atooltip = require("awful.tooltip")
local clienticon = require("awful.widget.clienticon")
local beautiful = require("beautiful")
local drawable = require("wibox.drawable")
local imagebox = require("wibox.widget.imagebox")
local textbox = require("wibox.widget.textbox")
local base = require("wibox.widget.base")
local capi = {
    client = client
}


local titlebar = {
    widget = {},
    enable_tooltip = true,
    fallback_name = '<unknown>'
}

local default_tooltip_messages = {
    close = "Close",
    minimize = "Minimize",
    maximized_active = "Unmaximize",
    maximized_inactive = "Maximize",
    floating_active = "Tiling",
    floating_inactive = "Floating",
    ontop_active = "NotOnTop",
    ontop_inactive = "OnTop",
    sticky_active = "NotSticky",
    sticky_inactive = "Sticky"
}

--- Show tooltips when hover on titlebar buttons.
--
-- @tfield[opt=true] boolean awful.titlebar.enable_tooltip
-- @param boolean

--- Title to display if client name is not set.
--
-- @field[opt='\<unknown\>'] awful.titlebar.fallback_name
-- @tparam[opt='\<unknown\>'] string fallback_name

--- The titlebar foreground (text) color.
--
-- @beautiful beautiful.titlebar_fg_normal
-- @param color
-- @see gears.color

--- The titlebar background color.
--
-- @beautiful beautiful.titlebar_bg_normal
-- @param color
-- @see gears.color

--- The titlebar background image image.
--
-- @beautiful beautiful.titlebar_bgimage_normal
-- @tparam gears.surface|string path
-- @see gears.surface

--- The titlebar foreground (text) color.
--
-- @beautiful beautiful.titlebar_fg
-- @param color
-- @see gears.color

--- The titlebar background color.
--
-- @beautiful beautiful.titlebar_bg
-- @param color
-- @see gears.color

--- The titlebar background image image.
--
-- @beautiful beautiful.titlebar_bgimage
-- @tparam gears.surface|string path
-- @see gears.surface

--- The focused titlebar foreground (text) color.
--
-- @beautiful beautiful.titlebar_fg_focus
-- @param color
-- @see gears.color

--- The focused titlebar background color.
--
-- @beautiful beautiful.titlebar_bg_focus
-- @param color
-- @see gears.color

--- The focused titlebar background image image.
--
-- @beautiful beautiful.titlebar_bgimage_focus
-- @tparam gears.surface|string path
-- @see gears.surface

--- The urgent titlebar foreground (text) color.
--
-- @beautiful beautiful.titlebar_fg_urgent
-- @param color
-- @see gears.color

--- The urgent titlebar background color.
--
-- @beautiful beautiful.titlebar_bg_urgent
-- @param color
-- @see gears.color

--- The urgent titlebar background image.
--
-- @beautiful beautiful.titlebar_bgimage_urgent
-- @tparam gears.surface|string path
-- @see gears.surface

--- The normal non-floating button image.
--
-- @beautiful beautiful.titlebar_floating_button_normal
-- @tparam gears.surface|string path
-- @see gears.surface

--- The normal non-maximized button image.
--
-- @beautiful beautiful.titlebar_maximized_button_normal
-- @tparam gears.surface|string path
-- @see gears.surface

--- The normal minimize button image.
--
-- @beautiful beautiful.titlebar_minimize_button_normal
-- @tparam gears.surface|string path
-- @see gears.surface

--- The hovered minimize button image.
--
-- @beautiful beautiful.titlebar_minimize_button_normal_hover
-- @tparam gears.surface|string path
-- @see gears.surface

--- The pressed minimize button image.
--
-- @beautiful beautiful.titlebar_minimize_button_normal_press
-- @tparam gears.surface|string path
-- @see gears.surface

--- The normal close button image.
--
-- @beautiful beautiful.titlebar_close_button_normal
-- @tparam gears.surface|string path
-- @see gears.surface

--- The hovered close button image.
--
-- @beautiful beautiful.titlebar_close_button_normal_hover
-- @tparam gears.surface|string path
-- @see gears.surface

--- The pressed close button image.
--
-- @beautiful beautiful.titlebar_close_button_normal_press
-- @tparam gears.surface|string path
-- @see gears.surface

--- The normal non-ontop button image.
--
-- @beautiful beautiful.titlebar_ontop_button_normal
-- @tparam gears.surface|string path
-- @see gears.surface

--- The normal non-sticky button image.
--
-- @beautiful beautiful.titlebar_sticky_button_normal
-- @tparam gears.surface|string path
-- @see gears.surface

--- The focused client non-floating button image.
--
-- @beautiful beautiful.titlebar_floating_button_focus
-- @tparam gears.surface|string path
-- @see gears.surface

--- The focused client non-maximized button image.
--
-- @beautiful beautiful.titlebar_maximized_button_focus
-- @tparam gears.surface|string path
-- @see gears.surface

--- The focused client minimize button image.
--
-- @beautiful beautiful.titlebar_minimize_button_focus
-- @tparam gears.surface|string path
-- @see gears.surface

--- The hovered+focused client minimize button image.
--
-- @beautiful beautiful.titlebar_minimize_button_focus_hover
-- @tparam gears.surface|string path
-- @see gears.surface

--- The pressed+focused minimize button image.
--
-- @beautiful beautiful.titlebar_minimize_button_focus_press
-- @tparam gears.surface|string path
-- @see gears.surface

--- The focused client close button image.
--
-- @beautiful beautiful.titlebar_close_button_focus
-- @tparam gears.surface|string path
-- @see gears.surface

--- The hovered+focused close button image.
--
-- @beautiful beautiful.titlebar_close_button_focus_hover
-- @tparam gears.surface|string path
-- @see gears.surface

--- The pressed+focused close button image.
--
-- @beautiful beautiful.titlebar_close_button_focus_press
-- @tparam gears.surface|string path
-- @see gears.surface

--- The focused client non-ontop button image.
--
-- @beautiful beautiful.titlebar_ontop_button_focus
-- @tparam gears.surface|string path
-- @see gears.surface

--- The focused client sticky button image.
--
-- @beautiful beautiful.titlebar_sticky_button_focus
-- @tparam gears.surface|string path
-- @see gears.surface

--- The normal floating button image.
--
-- @beautiful beautiful.titlebar_floating_button_normal_active
-- @tparam gears.surface|string path
-- @see gears.surface

--- The hovered floating client button image.
--
-- @beautiful beautiful.titlebar_floating_button_normal_active_hover
-- @tparam gears.surface|string path
-- @see gears.surface

--- The pressed floating client button image.
--
-- @beautiful beautiful.titlebar_floating_button_normal_active_press
-- @tparam gears.surface|string path
-- @see gears.surface

--- The maximized client button image.
--
-- @beautiful beautiful.titlebar_maximized_button_normal_active
-- @tparam gears.surface|string path
-- @see gears.surface

--- The hozered+maximized client button image.
--
-- @beautiful beautiful.titlebar_maximized_button_normal_active_hover
-- @tparam gears.surface|string path
-- @see gears.surface

--- The pressed+maximized button image.
--
-- @beautiful beautiful.titlebar_maximized_button_normal_active_press
-- @tparam gears.surface|string path
-- @see gears.surface

--- The ontop button image.
--
-- @beautiful beautiful.titlebar_ontop_button_normal_active
-- @tparam gears.surface|string path
-- @see gears.surface

--- The hovered+ontop client button image.
--
-- @beautiful beautiful.titlebar_ontop_button_normal_active_hover
-- @tparam gears.surface|string path
-- @see gears.surface

--- The pressed+ontop client button image.
--
-- @beautiful beautiful.titlebar_ontop_button_normal_active_press
-- @tparam gears.surface|string path
-- @see gears.surface

--- The sticky button image.
--
-- @beautiful beautiful.titlebar_sticky_button_normal_active
-- @tparam gears.surface|string path
-- @see gears.surface

--- The hovered+sticky button image.
--
-- @beautiful beautiful.titlebar_sticky_button_normal_active_hover
-- @tparam gears.surface|string path
-- @see gears.surface

--- The pressed+sticky client button image.
--
-- @beautiful beautiful.titlebar_sticky_button_normal_active_press
-- @tparam gears.surface|string path
-- @see gears.surface

--- The floating+focused client button image.
--
-- @beautiful beautiful.titlebar_floating_button_focus_active
-- @tparam gears.surface|string path
-- @see gears.surface

--- The hovered+floating+focused button image.
--
-- @beautiful beautiful.titlebar_floating_button_focus_active_hover
-- @tparam gears.surface|string path
-- @see gears.surface

--- The pressed+floating+focused button image.
--
-- @beautiful beautiful.titlebar_floating_button_focus_active_press
-- @tparam gears.surface|string path
-- @see gears.surface

--- The maximized+focused button image.
--
-- @beautiful beautiful.titlebar_maximized_button_focus_active
-- @tparam gears.surface|string path
-- @see gears.surface

--- The hovered+maximized+focused button image.
--
-- @beautiful beautiful.titlebar_maximized_button_focus_active_hover
-- @tparam gears.surface|string path
-- @see gears.surface

--- The pressed+maximized+focused button image.
--
-- @beautiful beautiful.titlebar_maximized_button_focus_active_press
-- @tparam gears.surface|string path
-- @see gears.surface

--- The ontop+focused button image.
--
-- @beautiful beautiful.titlebar_ontop_button_focus_active
-- @tparam gears.surface|string path
-- @see gears.surface

--- The hovered+ontop+focused button image.
--
-- @beautiful beautiful.titlebar_ontop_button_focus_active_hover
-- @tparam gears.surface|string path
-- @see gears.surface

--- The pressed+ontop+focused button image.
--
-- @beautiful beautiful.titlebar_ontop_button_focus_active_press
-- @tparam gears.surface|string path
-- @see gears.surface

--- The sticky+focused button image.
--
-- @beautiful beautiful.titlebar_sticky_button_focus_active
-- @tparam gears.surface|string path
-- @see gears.surface

--- The hovered+sticky+focused button image.
--
-- @beautiful beautiful.titlebar_sticky_button_focus_active_hover
-- @tparam gears.surface|string path
-- @see gears.surface

--- The pressed+sticky+focused button image.
--
-- @beautiful beautiful.titlebar_sticky_button_focus_active_press
-- @tparam gears.surface|string path
-- @see gears.surface

--- The inactive+floating button image.
--
-- @beautiful beautiful.titlebar_floating_button_normal_inactive
-- @tparam gears.surface|string path
-- @see gears.surface

--- The hovered+inactive+floating button image.
--
-- @beautiful beautiful.titlebar_floating_button_normal_inactive_hover
-- @tparam gears.surface|string path
-- @see gears.surface

--- The pressed+inactive+floating button image.
--
-- @beautiful beautiful.titlebar_floating_button_normal_inactive_press
-- @tparam gears.surface|string path
-- @see gears.surface

--- The inactive+maximized button image.
--
-- @beautiful beautiful.titlebar_maximized_button_normal_inactive
-- @tparam gears.surface|string path
-- @see gears.surface

--- The hovered+inactive+maximized button image.
--
-- @beautiful beautiful.titlebar_maximized_button_normal_inactive_hover
-- @tparam gears.surface|string path
-- @see gears.surface

--- The pressed+maximized+inactive button image.
--
-- @beautiful beautiful.titlebar_maximized_button_normal_inactive_press
-- @tparam gears.surface|string path
-- @see gears.surface

--- The inactive+ontop button image.
--
-- @beautiful beautiful.titlebar_ontop_button_normal_inactive
-- @tparam gears.surface|string path
-- @see gears.surface

--- The hovered+inactive+ontop button image.
--
-- @beautiful beautiful.titlebar_ontop_button_normal_inactive_hover
-- @tparam gears.surface|string path
-- @see gears.surface

--- The pressed+inactive+ontop button image.
--
-- @beautiful beautiful.titlebar_ontop_button_normal_inactive_press
-- @tparam gears.surface|string path
-- @see gears.surface

--- The inactive+sticky button image.
--
-- @beautiful beautiful.titlebar_sticky_button_normal_inactive
-- @tparam gears.surface|string path
-- @see gears.surface

--- The hovered+inactive+sticky button image.
--
-- @beautiful beautiful.titlebar_sticky_button_normal_inactive_hover
-- @tparam gears.surface|string path
-- @see gears.surface

--- The pressed+inactive+sticky button image.
--
-- @beautiful beautiful.titlebar_sticky_button_normal_inactive_press
-- @tparam gears.surface|string path
-- @see gears.surface

--- The inactive+focused+floating button image.
--
-- @beautiful beautiful.titlebar_floating_button_focus_inactive
-- @tparam gears.surface|string path
-- @see gears.surface

--- The hovered+inactive+focused+floating button image.
--
-- @beautiful beautiful.titlebar_floating_button_focus_inactive_hover
-- @tparam gears.surface|string path
-- @see gears.surface

--- The pressed+inactive+focused+floating button image.
--
-- @beautiful beautiful.titlebar_floating_button_focus_inactive_press
-- @tparam gears.surface|string path
-- @see gears.surface

--- The inactive+focused+maximized button image.
--
-- @beautiful beautiful.titlebar_maximized_button_focus_inactive
-- @tparam gears.surface|string path
-- @see gears.surface

--- The hovered+inactive+focused+maximized button image.
--
-- @beautiful beautiful.titlebar_maximized_button_focus_inactive_hover
-- @tparam gears.surface|string path
-- @see gears.surface

--- The pressed+inactive+focused+maximized button image.
--
-- @beautiful beautiful.titlebar_maximized_button_focus_inactive_press
-- @tparam gears.surface|string path
-- @see gears.surface

--- The inactive+focused+ontop button image.
--
-- @beautiful beautiful.titlebar_ontop_button_focus_inactive
-- @tparam gears.surface|string path
-- @see gears.surface

--- The hovered+inactive+focused+ontop button image.
--
-- @beautiful beautiful.titlebar_ontop_button_focus_inactive_hover
-- @tparam gears.surface|string path
-- @see gears.surface

--- The pressed+inactive+focused+ontop button image.
--
-- @beautiful beautiful.titlebar_ontop_button_focus_inactive_press
-- @tparam gears.surface|string path
-- @see gears.surface

--- The inactive+focused+sticky button image.
--
-- @beautiful beautiful.titlebar_sticky_button_focus_inactive
-- @tparam gears.surface|string path
-- @see gears.surface

--- The hovered+inactive+focused+sticky button image.
--
-- @beautiful beautiful.titlebar_sticky_button_focus_inactive_hover
-- @tparam gears.surface|string path
-- @see gears.surface

--- The pressed+inactive+focused+sticky button image.
--
-- @beautiful beautiful.titlebar_sticky_button_focus_inactive_press
-- @tparam gears.surface|string path
-- @see gears.surface

--- Titlebar tooltip message for close button.
-- @beautiful beautiful.titlebar_tooltip_messages_close
-- @tparam string tooltip tooltip message
-- @see awful.titlebar

--- Titlebar tooltip message for minimize button.
-- @beautiful beautiful.titlebar_tooltip_messages_minimize
-- @tparam string tooltip tooltip message
-- @see awful.titlebar

--- Titlebar tooltip message for maximized button when client is maximized.
-- @beautiful beautiful.titlebar_tooltip_messages_maximized_active
-- @tparam string tooltip tooltip message
-- @see awful.titlebar

--- Titlebar tooltip message for maximized button when client is unmaximized.
-- @beautiful beautiful.titlebar_tooltip_messages_maximized_inactive
-- @tparam string tooltip tooltip message
-- @see awful.titlebar

--- Titlebar tooltip message for floating button when client is floating.
-- @beautiful beautiful.titlebar_tooltip_messages_floating_active
-- @tparam string tooltip tooltip message
-- @see awful.titlebar

--- Titlebar tooltip message for floating button when client isn't floating.
-- @beautiful beautiful.titlebar_tooltip_messages_floating_inactive
-- @tparam string tooltip tooltip message
-- @see awful.titlebar

--- Titlebar tooltip message for onTop button when client is onTop.
-- @beautiful beautiful.titlebar_tooltip_messages_ontop_active
-- @tparam string tooltip tooltip message
-- @see awful.titlebar

--- Titlebar tooltip message for onTop button when client isn't onTop.
-- @beautiful beautiful.titlebar_tooltip_messages_ontop_inactive
-- @tparam string tooltip tooltip message
-- @see awful.titlebar

--- Titlebar tooltip message for onTop button when client is sticky.
-- @beautiful beautiful.titlebar_tooltip_messages_sticky_active
-- @tparam string tooltip tooltip message
-- @see awful.titlebar

--- Titlebar tooltip message for onTop button when client isn't sticky.
-- @beautiful beautiful.titlebar_tooltip_messages_sticky_inactive
-- @tparam string tooltip tooltip message
-- @see awful.titlebar

--- Titlebar buttons tooltip parameter `delay_show`.
-- This parameter `delay_show` defines a delay in seconds before showing the tooltip.
-- @beautiful beautiful.titlebar_tooltip_delay_show
-- @tparam integer delay in seconds before showing the tooltip
-- @see awful.tooltip

--- Titlebar buttons tooltip parameter `margins_leftright`.
-- This parameter `margins_leftright` defines the left/right margin for the tooltip text.
-- @beautiful beautiful.titlebar_tooltip_margins_leftright
-- @tparam integer margin for left/right corner for the tooltop text
-- @see awful.tooltip

--- Titlebar buttons tooltip parameter `margins_topbottom`.
-- This parameter `margins_topbottom` defines the top/bottom margin for the tooltip text.
-- @beautiful beautiful.titlebar_tooltip_margins_topbottom
-- @tparam integer margin for top/bottom corner for the tooltip text
-- @see awful.tooltip

--- Titlebar buttons tooltip parameter `timeout`.
-- This parameter `timeout` defines the timeout value for declared `timer_function`.
-- @beautiful beautiful.titlebar_tooltip_timeout
-- @tparam number timeout timeout value in seconds for activation of `timer_function`
-- @see awful.tooltip

--- Titlebar buttons tooltip parameter `align`.
-- This parameter `align` defines the alignment: 
-- `right`, `top_right`, `left`, `bottom_left`, `top_left`, `bottom`, `top`
-- @beautiful beautiful.titlebar_tooltip_align
-- @tparam string align the alignment string type
-- @see awful.tooltip

--- Set a declarative widget hierarchy description.
--
-- See [The declarative layout system](../documentation/03-declarative-layout.md.html)
-- @tparam table args An array containing the widgets disposition
-- @method setup
-- @noreturn


local all_titlebars = setmetatable({}, { __mode = 'k' })

-- Get a color for a titlebar, this tests many values from the array and the theme
local function get_color(name, c, args)
    local suffix = "_normal"

    if c.urgent then
        suffix = "_urgent"
    elseif c.active then
        suffix = "_focus"
    end
    local function get(array)
        return array["titlebar_"..name..suffix] or array["titlebar_"..name] or array[name..suffix] or array[name]
    end
    return get(args) or get(beautiful)
end

local function get_titlebar_function(c, position)
    if position == "left" then
        return c.titlebar_left
    elseif position == "right" then
        return c.titlebar_right
    elseif position == "top" then
        return c.titlebar_top
    elseif position == "bottom" then
        return c.titlebar_bottom
    else
        error("Invalid titlebar position '" .. position .. "'")
    end
end

--- Call `request::titlebars` to allow themes or rc.lua to create them even
-- when `titlebars_enabled` is not set in the rules.
-- @tparam client c The client.
-- @tparam[opt=false] boolean hide_all Hide all titlebars except `keep`
-- @tparam string keep Keep the titlebar at this position.
-- @tparam string context The reason why this was called.
-- @treturn boolean If the titlebars were loaded
local function load_titlebars(c, hide_all, keep, context)
    if c._request_titlebars_called then return false end

    c:emit_signal("request::titlebars", context, {})

    if hide_all then
        -- Don't bother checking if it has been created, `.hide` don't works
        -- anyway.
        for _, tb in ipairs {"top", "bottom", "left", "right"} do
            if tb ~= keep then
                titlebar.hide(c, tb)
            end
        end
    end

    c._request_titlebars_called = true

    return true
end

local function get_children_by_id(self, name)
    --TODO v5: Move the ID management to the hierarchy.
    if self._drawable._widget
      and self._drawable._widget._private
      and self._drawable._widget._private.by_id then
          return self._drawable.widget._private.by_id[name]
    end

    return {}
end


--- Create a new titlebar for the given client.
--
-- Every client can hold up to four titlebars, one for each side (i.e. each
-- value of `args.position`).
--
-- If this constructor is called again with the same
-- values for the client (`c`) and the titlebar position (`args.position`),
-- the previous titlebar will be removed and replaced by the new one.
--
-- @DOC_awful_titlebar_constructor_EXAMPLE@
--
-- @tparam client c The client the titlebar will be attached to.
-- @tparam[opt={}] table args A table with extra arguments for the titlebar.
-- @tparam[opt=font.height*1.5] number args.size The size of the titlebar. Will
--   be interpreted as `height` for horizontal titlebars or as `width` for
--   vertical titlebars.
-- @tparam[opt="top"] string args.position Possible values are `"top"`,
-- `"left"`, `"right"` and `"bottom"`.
-- @tparam[opt] string args.bg_normal
-- @tparam[opt] string args.bg_focus
-- @tparam[opt] string args.bg_urgent
-- @tparam[opt] string args.bgimage_normal
-- @tparam[opt] string args.bgimage_focus
-- @tparam[opt] string args.fg_normal
-- @tparam[opt] string args.fg_focus
-- @tparam[opt] string args.fg_urgent
-- @tparam[opt] string args.font
-- @constructorfct awful.titlebar
-- @treturn wibox.drawable The newly created titlebar object.
-- @usebeautiful beautiful.titlebar_fg_normal
-- @usebeautiful beautiful.titlebar_bg_normal
-- @usebeautiful beautiful.titlebar_bgimage_normal
-- @usebeautiful beautiful.titlebar_fg
-- @usebeautiful beautiful.titlebar_bg
-- @usebeautiful beautiful.titlebar_bgimage
-- @usebeautiful beautiful.titlebar_fg_focus
-- @usebeautiful beautiful.titlebar_bg_focus
-- @usebeautiful beautiful.titlebar_bgimage_focus
-- @usebeautiful beautiful.titlebar_fg_urgent
-- @usebeautiful beautiful.titlebar_bg_urgent
-- @usebeautiful beautiful.titlebar_bgimage_urgent
local function new(c, args)
    args = args or {}
    local position = args.position or "top"
    local size = args.size or gmath.round(beautiful.get_font_height(args.font) * 1.5)
    local d = get_titlebar_function(c, position)(c, size)

    -- Make sure that there is never more than one titlebar for any given client
    local bars = all_titlebars[c]
    if not bars then
        bars = {}
        all_titlebars[c] = bars
    end

    local ret
    if not bars[position] then
        local context = {
            client = c,
            position = position
        }
        ret = drawable(d, context, "awful.titlebar")
        ret:_inform_visible(true)
        local function update_colors()
            local args_ = bars[position].args
            ret:set_bg(get_color("bg", c, args_))
            ret:set_fg(get_color("fg", c, args_))
            ret:set_bgimage(get_color("bgimage", c, args_))
        end

        bars[position] = {
            args = args,
            drawable = ret,
            font = args.font or beautiful.titlebar_font,
            update_colors = update_colors
        }

        -- Update the colors when focus changes
        c:connect_signal("property::active", update_colors)
        c:connect_signal("property::urgent", update_colors)

        -- Inform the drawable when it becomes invisible
        c:connect_signal("request::unmanage", function()
            ret:_inform_visible(false)
        end)
    else
        bars[position].args = args
        ret = bars[position].drawable
    end

    -- Make sure the titlebar has the right colors applied
    bars[position].update_colors()

    -- Handle declarative/recursive widget container
    ret.setup = base.widget.setup
    ret.get_children_by_id = get_children_by_id

    c._private = c._private or {}
    c._private.titlebars = bars

    return ret
end

--- Show the client's titlebar.
-- @tparam client c The client whose titlebar is modified
-- @tparam[opt="top"] string position The position of the titlebar. Must be one of `"left"`,
--   `"right"`, `"top"`, `"bottom"`.
-- @noreturn
-- @staticfct awful.titlebar.show
-- @request client titlebars show granted Called when `awful.titlebar.show` is
--  called.
function titlebar.show(c, position)
    position = position or "top"
    if load_titlebars(c, true, position, "show") then return end
    local bars = all_titlebars[c]
    local data = bars and bars[position]
    local args = data and data.args
    new(c, args)
end

--- Hide the client's titlebar.
-- @tparam client c The client whose titlebar is modified
-- @tparam[opt="top"] string position The position of the titlebar. Must be one of `"left"`,
--   `"right"`, `"top"`, `"bottom"`.
-- @noreturn
-- @staticfct awful.titlebar.hide
function titlebar.hide(c, position)
    position = position or "top"
    get_titlebar_function(c, position)(c, 0)
end

--- Toggle the client's titlebar, hiding it if it is visible, otherwise showing it.
-- @tparam client c The client whose titlebar is modified
-- @tparam[opt="top"] string position The position of the titlebar. Must be one of `"left"`,
--   `"right"`, `"top"`, `"bottom"`.
-- @noreturn
-- @staticfct awful.titlebar.toggle
-- @request client titlebars toggle granted Called when `awful.titlebar.toggle` is
--  called.
function titlebar.toggle(c, position)
    position = position or "top"
    if load_titlebars(c, true, position, "toggle") then return end
    local _, size = get_titlebar_function(c, position)(c)
    if size == 0 then
        titlebar.show(c, position)
    else
        titlebar.hide(c, position)
    end
end

local instances = {}

-- Do the equivalent of
--     c:connect_signal(signal, widget.update)
-- without keeping a strong reference to the widget.
local function update_on_signal(c, signal, widget)
    local sig_instances = instances[signal]
    if sig_instances == nil then
        sig_instances = setmetatable({}, { __mode = "k" })
        instances[signal] = sig_instances
        capi.client.connect_signal(signal, function(cl)
            local widgets = sig_instances[cl]
            if widgets then
                for _, w in pairs(widgets) do
                    w.update()
                end
            end
        end)
    end
    local widgets = sig_instances[c]
    if widgets == nil then
        widgets = setmetatable({}, { __mode = "v" })
        sig_instances[c] = widgets
    end
    table.insert(widgets, widget)
end

--- Honor the font.
local function draw_title(self, ctx, cr, width, height)
    if ctx.position and ctx.client then
        local bars = all_titlebars[ctx.client]
        local data = bars and bars[ctx.position]

        if data and data.font then
            self:set_font(data.font)
        end
    end

    textbox.draw(self, ctx, cr, width, height)
end

--- Create a new title widget.
--
-- A title widget displays the name of a client.
-- Please note that this returns a textbox and all of textbox' API is available.
-- This way, you can e.g. modify the font that is used.
--
-- @tparam client c The client for which a titlewidget should be created.
-- @return The title widget.
-- @constructorfct awful.titlebar.widget.titlewidget
function titlebar.widget.titlewidget(c)
    local ret = textbox()

    rawset(ret, "draw", draw_title)

    local function update()
        ret:set_text(c.name or titlebar.fallback_name)
    end
    ret.update = update
    update_on_signal(c, "property::name", ret)
    update()

    return ret
end

--- Create a new icon widget.
--
-- An icon widget displays the icon of a client.
-- Please note that this returns an imagebox and all of the imagebox' API is
-- available. This way, you can e.g. disallow resizes.
--
-- @tparam client c The client for which an icon widget should be created.
-- @return The icon widget.
-- @constructorfct awful.titlebar.widget.iconwidget
function titlebar.widget.iconwidget(c)
    return clienticon(c)
end

--- Create a new button widget.
--
-- A button widget displays an image and reacts to
-- mouse clicks. Please note that the caller has to make sure that this widget
-- gets redrawn when needed by calling the returned widget's `:update()` method.
-- The selector function should return a value describing a state. If the value
-- is a boolean, either `"active"` or `"inactive"` are used. The actual image is
-- then found in the theme as `titlebar_[name]_button_[normal/focus]_[state]`.
-- If that value does not exist, the focused state is ignored for the next try.
--
-- @tparam client c The client for which a button is created.
-- @tparam string name Name of the button, used for accessing the theme and
--   in the tooltip.
-- @tparam function selector A function that selects the image that should be displayed.
-- @tparam function action Function that is called when the button is clicked.
-- @treturn wibox.widget The widget
-- @constructorfct awful.titlebar.widget.button
function titlebar.widget.button(c, name, selector, action)
    local ret = imagebox()
    if titlebar.enable_tooltip then
        ret._private.tooltip = atooltip({
            objects = {ret},
            delay_show = beautiful["titlebar_tooltip_delay_show"] or 1,
            margins_leftright = beautiful["titlebar_tooltip_margins_leftright"],
            margins_topbottom = beautiful["titlebar_tooltip_margins_topbottom"],
            timeout = beautiful["titlebar_tooltip_timeout"],
            align = beautiful["titlebar_tooltip_align"]
        })
    end

    local function update()
        local img = selector(c)
        if type(img) ~= "nil" then
            -- Convert booleans automatically
            if type(img) == "boolean" then
                if img then
                    img = "active"
                else
                    img = "inactive"
                end
            end
            local prefix = "normal"
            if c.active then
                prefix = "focus"
            end
            if img ~= "" then
                prefix = prefix .. "_"
            end
            local state = ret.state
            if state ~= "" then
                state = "_" .. state
            end
            -- try select user defined tooltip texts according to state
            local tooltip_text = beautiful["titlebar_tooltip_messages_" .. name .. "_" .. img]
                or beautiful["titlebar_tooltip_messages_" .. name]
                or default_tooltip_messages[name .. "_" .. img]
                or default_tooltip_messages[name]
                or name
            -- First try with a prefix based on the client's focus state,
            -- then try again without that prefix if nothing was found,
            -- and finally, try a fallback for compatibility with Awesome 3.5 themes
            local theme = beautiful["titlebar_" .. name .. "_button_" .. prefix .. img .. state]
                       or beautiful["titlebar_" .. name .. "_button_" .. prefix .. img]
                       or beautiful["titlebar_" .. name .. "_button_" .. img]
                       or beautiful["titlebar_" .. name .. "_button_" .. prefix .. "_inactive"]
            if theme then
                img = theme
            end
            -- Set tooltip text for button
            if titlebar.enable_tooltip then
                ret._private.tooltip:set_text(tooltip_text)
            end
        end
        -- Set button image by focus and activity state
        ret:set_image(img)
    end
    ret.state = ""
    if action then
        ret.buttons = {
            abutton({ }, 1, nil, function()
                ret.state = ""
                update()
                action(c, selector(c))
            end)
        }
    else
        ret.buttons = {
            abutton({ }, 1, nil, function()
                ret.state = ""
                update()
            end)
        }
    end
    ret:connect_signal("mouse::enter", function()
        ret.state = "hover"
        update()
    end)
    ret:connect_signal("mouse::leave", function()
        ret.state = ""
        update()
    end)
    ret:connect_signal("button::press", function(_, _, _, b)
        if b == 1 then
            ret.state = "press"
            update()
        end
    end)
    ret.update = update
    update()

    -- We do magic based on whether a client is focused above, so we need to
    -- connect to the corresponding signal here.
    update_on_signal(c, "focus", ret)
    update_on_signal(c, "unfocus", ret)

    return ret
end

--- Create a new float button for a client.
--
-- @constructorfct awful.titlebar.widget.floatingbutton
-- @tparam client c The client for which the button is wanted.
-- @usebeautiful beautiful.titlebar_floating_button_normal
-- @usebeautiful beautiful.titlebar_floating_button_focus
-- @usebeautiful beautiful.titlebar_floating_button_normal_active
-- @usebeautiful beautiful.titlebar_floating_button_normal_active_hover
-- @usebeautiful beautiful.titlebar_floating_button_normal_active_press
-- @usebeautiful beautiful.titlebar_floating_button_focus_active
-- @usebeautiful beautiful.titlebar_floating_button_focus_active_hover
-- @usebeautiful beautiful.titlebar_floating_button_focus_active_press
-- @usebeautiful beautiful.titlebar_floating_button_normal_inactive
-- @usebeautiful beautiful.titlebar_floating_button_normal_inactive_hover
-- @usebeautiful beautiful.titlebar_floating_button_normal_inactive_press
-- @usebeautiful beautiful.titlebar_floating_button_focus_inactive
-- @usebeautiful beautiful.titlebar_floating_button_focus_inactive_hover
-- @usebeautiful beautiful.titlebar_floating_button_focus_inactive_press
function titlebar.widget.floatingbutton(c)
    local widget = titlebar.widget.button(c, "floating", aclient.object.get_floating, aclient.floating.toggle)
    update_on_signal(c, "property::floating", widget)
    return widget
end

--- Create a new maximize button for a client.
--
-- @constructorfct awful.titlebar.widget.maximizedbutton
-- @tparam client c The client for which the button is wanted.
-- @usebeautiful beautiful.titlebar_maximized_button_focus_active
-- @usebeautiful beautiful.titlebar_maximized_button_focus_active_hover
-- @usebeautiful beautiful.titlebar_maximized_button_focus_active_press
-- @usebeautiful beautiful.titlebar_maximized_button_normal_inactive
-- @usebeautiful beautiful.titlebar_maximized_button_normal_inactive_hover
-- @usebeautiful beautiful.titlebar_maximized_button_normal_inactive_press
-- @usebeautiful beautiful.titlebar_maximized_button_focus_inactive
-- @usebeautiful beautiful.titlebar_maximized_button_focus_inactive_hover
-- @usebeautiful beautiful.titlebar_maximized_button_focus_inactive_press
-- @usebeautiful beautiful.titlebar_maximized_button_normal
-- @usebeautiful beautiful.titlebar_maximized_button_focus
-- @usebeautiful beautiful.titlebar_maximized_button_normal_active
-- @usebeautiful beautiful.titlebar_maximized_button_normal_active_hover
-- @usebeautiful beautiful.titlebar_maximized_button_normal_active_press
function titlebar.widget.maximizedbutton(c)
    local widget = titlebar.widget.button(c, "maximized", function(cl)
        return cl.maximized
    end, function(cl, state)
        cl.maximized = not state
    end)
    update_on_signal(c, "property::maximized", widget)
    return widget
end

--- Create a new minimize button for a client.
--
-- @constructorfct awful.titlebar.widget.minimizebutton
-- @tparam client c The client for which the button is wanted.
-- @usebeautiful beautiful.titlebar_minimize_button_normal
-- @usebeautiful beautiful.titlebar_minimize_button_normal_hover
-- @usebeautiful beautiful.titlebar_minimize_button_normal_press
-- @usebeautiful beautiful.titlebar_minimize_button_focus
-- @usebeautiful beautiful.titlebar_minimize_button_focus_hover
-- @usebeautiful beautiful.titlebar_minimize_button_focus_press
function titlebar.widget.minimizebutton(c)
    local widget = titlebar.widget.button(c, "minimize",
                                          function() return "" end,
                                          function(cl) cl.minimized = not cl.minimized end)
    update_on_signal(c, "property::minimized", widget)
    return widget
end

--- Create a new closing button for a client.
--
-- @constructorfct awful.titlebar.widget.closebutton
-- @tparam client c The client for which the button is wanted.
-- @usebeautiful beautiful.titlebar_close_button_normal
-- @usebeautiful beautiful.titlebar_close_button_normal_hover
-- @usebeautiful beautiful.titlebar_close_button_normal_press
-- @usebeautiful beautiful.titlebar_close_button_focus
-- @usebeautiful beautiful.titlebar_close_button_focus_hover
-- @usebeautiful beautiful.titlebar_close_button_focus_press
function titlebar.widget.closebutton(c)
    return titlebar.widget.button(c, "close", function() return "" end, function(cl) cl:kill() end)
end

--- Create a new ontop button for a client.
--
-- @constructorfct awful.titlebar.widget.ontopbutton
-- @tparam client c The client for which the button is wanted.
-- @usebeautiful beautiful.titlebar_ontop_button_normal
-- @usebeautiful beautiful.titlebar_ontop_button_focus
-- @usebeautiful beautiful.titlebar_ontop_button_normal_active
-- @usebeautiful beautiful.titlebar_ontop_button_normal_active_hover
-- @usebeautiful beautiful.titlebar_ontop_button_normal_active_press
-- @usebeautiful beautiful.titlebar_ontop_button_focus_active
-- @usebeautiful beautiful.titlebar_ontop_button_focus_active_hover
-- @usebeautiful beautiful.titlebar_ontop_button_focus_active_press
-- @usebeautiful beautiful.titlebar_ontop_button_normal_inactive
-- @usebeautiful beautiful.titlebar_ontop_button_normal_inactive_hover
-- @usebeautiful beautiful.titlebar_ontop_button_normal_inactive_press
-- @usebeautiful beautiful.titlebar_ontop_button_focus_inactive
-- @usebeautiful beautiful.titlebar_ontop_button_focus_inactive_hover
-- @usebeautiful beautiful.titlebar_ontop_button_focus_inactive_press
function titlebar.widget.ontopbutton(c)
    local widget = titlebar.widget.button(c, "ontop",
                                          function(cl) return cl.ontop end,
                                          function(cl, state) cl.ontop = not state end)
    update_on_signal(c, "property::ontop", widget)
    return widget
end

--- Create a new sticky button for a client.
-- @constructorfct awful.titlebar.widget.stickybutton
-- @tparam client c The client for which the button is wanted.
-- @usebeautiful beautiful.titlebar_sticky_button_normal
-- @usebeautiful beautiful.titlebar_sticky_button_focus
-- @usebeautiful beautiful.titlebar_sticky_button_normal_active
-- @usebeautiful beautiful.titlebar_sticky_button_normal_active_hover
-- @usebeautiful beautiful.titlebar_sticky_button_normal_active_press
-- @usebeautiful beautiful.titlebar_sticky_button_focus_active
-- @usebeautiful beautiful.titlebar_sticky_button_focus_active_hover
-- @usebeautiful beautiful.titlebar_sticky_button_focus_active_press
-- @usebeautiful beautiful.titlebar_sticky_button_normal_inactive
-- @usebeautiful beautiful.titlebar_sticky_button_normal_inactive_hover
-- @usebeautiful beautiful.titlebar_sticky_button_normal_inactive_press
-- @usebeautiful beautiful.titlebar_sticky_button_focus_inactive
-- @usebeautiful beautiful.titlebar_sticky_button_focus_inactive_hover
-- @usebeautiful beautiful.titlebar_sticky_button_focus_inactive_press
function titlebar.widget.stickybutton(c)
    local widget = titlebar.widget.button(c, "sticky",
                                          function(cl) return cl.sticky end,
                                          function(cl, state) cl.sticky = not state end)
    update_on_signal(c, "property::sticky", widget)
    return widget
end

client.connect_signal("request::unmanage", function(c)
    all_titlebars[c] = nil
end)

return setmetatable(titlebar, { __call = function(_, ...) return new(...) end})

-- vim: filetype=lua:expandtab:shiftwidth=4:tabstop=8:softtabstop=4:textwidth=80
