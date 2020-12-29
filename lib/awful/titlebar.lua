---------------------------------------------------------------------------
--- Titlebars for awful.
--**Create a titlebar:**
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
local floating = require("awful.layout.suit.floating")
local gdebug = require("gears.debug")

local capi = {
    client = client
}


local titlebar = {
    widget = {},
    enable_tooltip = true,
    fallback_name = '<unknown>'
}


--- Show tooltips when hover on titlebar buttons.
-- @tfield[opt=true] boolean awful.titlebar.enable_tooltip
-- @param boolean

--- Title to display if client name is not set.
-- @field[opt='\<unknown\>'] awful.titlebar.fallback_name
-- @tparam[opt='\<unknown\>'] string fallback_name


--- The titlebar foreground (text) color.
-- @beautiful beautiful.titlebar_fg_normal
-- @param color
-- @see gears.color

--- The titlebar background color.
-- @beautiful beautiful.titlebar_bg_normal
-- @param color
-- @see gears.color

--- The titlebar background image image.
-- @beautiful beautiful.titlebar_bgimage_normal
-- @tparam gears.surface|string path
-- @see gears.surface

--- The titlebar foreground (text) color.
-- @beautiful beautiful.titlebar_fg
-- @param color
-- @see gears.color

--- The titlebar background color.
-- @beautiful beautiful.titlebar_bg
-- @param color
-- @see gears.color

--- The titlebar background image image.
-- @beautiful beautiful.titlebar_bgimage
-- @tparam gears.surface|string path
-- @see gears.surface

--- The focused titlebar foreground (text) color.
-- @beautiful beautiful.titlebar_fg_focus
-- @param color
-- @see gears.color

--- The focused titlebar background color.
-- @beautiful beautiful.titlebar_bg_focus
-- @param color
-- @see gears.color

--- The focused titlebar background image image.
-- @beautiful beautiful.titlebar_bgimage_focus
-- @tparam gears.surface|string path
-- @see gears.surface

--- floating_button_normal.
-- @beautiful beautiful.titlebar_floating_button_normal
-- @tparam gears.surface|string path
-- @see gears.surface

--- maximized_button_normal.
-- @beautiful beautiful.titlebar_maximized_button_normal
-- @tparam gears.surface|string path
-- @see gears.surface

--- minimize_button_normal.
-- @beautiful beautiful.titlebar_minimize_button_normal
-- @tparam gears.surface|string path
-- @see gears.surface

--- minimize_button_normal_hover.
-- @beautiful beautiful.titlebar_minimize_button_normal_hover
-- @tparam gears.surface|string path
-- @see gears.surface

--- minimize_button_normal_press.
-- @beautiful beautiful.titlebar_minimize_button_normal_press
-- @tparam gears.surface|string path
-- @see gears.surface

--- close_button_normal.
-- @beautiful beautiful.titlebar_close_button_normal
-- @tparam gears.surface|string path
-- @see gears.surface

--- close_button_normal_hover.
-- @beautiful beautiful.titlebar_close_button_normal_hover
-- @tparam gears.surface|string path
-- @see gears.surface

--- close_button_normal_press.
-- @beautiful beautiful.titlebar_close_button_normal_press
-- @tparam gears.surface|string path
-- @see gears.surface

--- ontop_button_normal.
-- @beautiful beautiful.titlebar_ontop_button_normal
-- @tparam gears.surface|string path
-- @see gears.surface

--- sticky_button_normal.
-- @beautiful beautiful.titlebar_sticky_button_normal
-- @tparam gears.surface|string path
-- @see gears.surface

--- floating_button_focus.
-- @beautiful beautiful.titlebar_floating_button_focus
-- @tparam gears.surface|string path
-- @see gears.surface

--- maximized_button_focus.
-- @beautiful beautiful.titlebar_maximized_button_focus
-- @tparam gears.surface|string path
-- @see gears.surface

--- minimize_button_focus.
-- @beautiful beautiful.titlebar_minimize_button_focus
-- @tparam gears.surface|string path
-- @see gears.surface

--- minimize_button_focus_hover.
-- @beautiful beautiful.titlebar_minimize_button_focus_hover
-- @tparam gears.surface|string path
-- @see gears.surface

--- minimize_button_focus_press.
-- @beautiful beautiful.titlebar_minimize_button_focus_press
-- @tparam gears.surface|string path
-- @see gears.surface

--- close_button_focus.
-- @beautiful beautiful.titlebar_close_button_focus
-- @tparam gears.surface|string path
-- @see gears.surface

--- close_button_focus_hover.
-- @beautiful beautiful.titlebar_close_button_focus_hover
-- @tparam gears.surface|string path
-- @see gears.surface

--- close_button_focus_press.
-- @beautiful beautiful.titlebar_close_button_focus_press
-- @tparam gears.surface|string path
-- @see gears.surface

--- ontop_button_focus.
-- @beautiful beautiful.titlebar_ontop_button_focus
-- @tparam gears.surface|string path
-- @see gears.surface

--- sticky_button_focus.
-- @beautiful beautiful.titlebar_sticky_button_focus
-- @tparam gears.surface|string path
-- @see gears.surface

--- floating_button_normal_active.
-- @beautiful beautiful.titlebar_floating_button_normal_active
-- @tparam gears.surface|string path
-- @see gears.surface

--- floating_button_normal_active_hover.
-- @beautiful beautiful.titlebar_floating_button_normal_active_hover
-- @tparam gears.surface|string path
-- @see gears.surface

--- floating_button_normal_active_press.
-- @beautiful beautiful.titlebar_floating_button_normal_active_press
-- @tparam gears.surface|string path
-- @see gears.surface

--- maximized_button_normal_active.
-- @beautiful beautiful.titlebar_maximized_button_normal_active
-- @tparam gears.surface|string path
-- @see gears.surface

--- maximized_button_normal_active_hover.
-- @beautiful beautiful.titlebar_maximized_button_normal_active_hover
-- @tparam gears.surface|string path
-- @see gears.surface

--- maximized_button_normal_active_press.
-- @beautiful beautiful.titlebar_maximized_button_normal_active_press
-- @tparam gears.surface|string path
-- @see gears.surface

--- ontop_button_normal_active.
-- @beautiful beautiful.titlebar_ontop_button_normal_active
-- @tparam gears.surface|string path
-- @see gears.surface

--- ontop_button_normal_active_hover.
-- @beautiful beautiful.titlebar_ontop_button_normal_active_hover
-- @tparam gears.surface|string path
-- @see gears.surface

--- ontop_button_normal_active_press.
-- @beautiful beautiful.titlebar_ontop_button_normal_active_press
-- @tparam gears.surface|string path
-- @see gears.surface

--- sticky_button_normal_active.
-- @beautiful beautiful.titlebar_sticky_button_normal_active
-- @tparam gears.surface|string path
-- @see gears.surface

--- sticky_button_normal_active_hover.
-- @beautiful beautiful.titlebar_sticky_button_normal_active_hover
-- @tparam gears.surface|string path
-- @see gears.surface

--- sticky_button_normal_active_press.
-- @beautiful beautiful.titlebar_sticky_button_normal_active_press
-- @tparam gears.surface|string path
-- @see gears.surface

--- floating_button_focus_active.
-- @beautiful beautiful.titlebar_floating_button_focus_active
-- @tparam gears.surface|string path
-- @see gears.surface

--- floating_button_focus_active_hover.
-- @beautiful beautiful.titlebar_floating_button_focus_active_hover
-- @tparam gears.surface|string path
-- @see gears.surface

--- floating_button_focus_active_press.
-- @beautiful beautiful.titlebar_floating_button_focus_active_press
-- @tparam gears.surface|string path
-- @see gears.surface

--- maximized_button_focus_active.
-- @beautiful beautiful.titlebar_maximized_button_focus_active
-- @tparam gears.surface|string path
-- @see gears.surface

--- maximized_button_focus_active_hover.
-- @beautiful beautiful.titlebar_maximized_button_focus_active_hover
-- @tparam gears.surface|string path
-- @see gears.surface

--- maximized_button_focus_active_press.
-- @beautiful beautiful.titlebar_maximized_button_focus_active_press
-- @tparam gears.surface|string path
-- @see gears.surface

--- ontop_button_focus_active.
-- @beautiful beautiful.titlebar_ontop_button_focus_active
-- @tparam gears.surface|string path
-- @see gears.surface

--- ontop_button_focus_active_hover.
-- @beautiful beautiful.titlebar_ontop_button_focus_active_hover
-- @tparam gears.surface|string path
-- @see gears.surface

--- ontop_button_focus_active_press.
-- @beautiful beautiful.titlebar_ontop_button_focus_active_press
-- @tparam gears.surface|string path
-- @see gears.surface

--- sticky_button_focus_active.
-- @beautiful beautiful.titlebar_sticky_button_focus_active
-- @tparam gears.surface|string path
-- @see gears.surface

--- sticky_button_focus_active_hover.
-- @beautiful beautiful.titlebar_sticky_button_focus_active_hover
-- @tparam gears.surface|string path
-- @see gears.surface

--- sticky_button_focus_active_press.
-- @beautiful beautiful.titlebar_sticky_button_focus_active_press
-- @tparam gears.surface|string path
-- @see gears.surface

--- floating_button_normal_inactive.
-- @beautiful beautiful.titlebar_floating_button_normal_inactive
-- @tparam gears.surface|string path
-- @see gears.surface

--- floating_button_normal_inactive_hover.
-- @beautiful beautiful.titlebar_floating_button_normal_inactive_hover
-- @tparam gears.surface|string path
-- @see gears.surface

--- floating_button_normal_inactive_press.
-- @beautiful beautiful.titlebar_floating_button_normal_inactive_press
-- @tparam gears.surface|string path
-- @see gears.surface

--- maximized_button_normal_inactive.
-- @beautiful beautiful.titlebar_maximized_button_normal_inactive
-- @tparam gears.surface|string path
-- @see gears.surface

--- maximized_button_normal_inactive_hover.
-- @beautiful beautiful.titlebar_maximized_button_normal_inactive_hover
-- @tparam gears.surface|string path
-- @see gears.surface

--- maximized_button_normal_inactive_press.
-- @beautiful beautiful.titlebar_maximized_button_normal_inactive_press
-- @tparam gears.surface|string path
-- @see gears.surface

--- ontop_button_normal_inactive.
-- @beautiful beautiful.titlebar_ontop_button_normal_inactive
-- @tparam gears.surface|string path
-- @see gears.surface

--- ontop_button_normal_inactive_hover.
-- @beautiful beautiful.titlebar_ontop_button_normal_inactive_hover
-- @tparam gears.surface|string path
-- @see gears.surface

--- ontop_button_normal_inactive_press.
-- @beautiful beautiful.titlebar_ontop_button_normal_inactive_press
-- @tparam gears.surface|string path
-- @see gears.surface

--- sticky_button_normal_inactive.
-- @beautiful beautiful.titlebar_sticky_button_normal_inactive
-- @tparam gears.surface|string path
-- @see gears.surface

--- sticky_button_normal_inactive_hover.
-- @beautiful beautiful.titlebar_sticky_button_normal_inactive_hover
-- @tparam gears.surface|string path
-- @see gears.surface

--- sticky_button_normal_inactive_press.
-- @beautiful beautiful.titlebar_sticky_button_normal_inactive_press
-- @tparam gears.surface|string path
-- @see gears.surface

--- floating_button_focus_inactive.
-- @beautiful beautiful.titlebar_floating_button_focus_inactive
-- @tparam gears.surface|string path
-- @see gears.surface

--- floating_button_focus_inactive_hover.
-- @beautiful beautiful.titlebar_floating_button_focus_inactive_hover
-- @tparam gears.surface|string path
-- @see gears.surface

--- floating_button_focus_inactive_press.
-- @beautiful beautiful.titlebar_floating_button_focus_inactive_press
-- @tparam gears.surface|string path
-- @see gears.surface

--- maximized_button_focus_inactive.
-- @beautiful beautiful.titlebar_maximized_button_focus_inactive
-- @tparam gears.surface|string path
-- @see gears.surface

--- maximized_button_focus_inactive_hover.
-- @beautiful beautiful.titlebar_maximized_button_focus_inactive_hover
-- @tparam gears.surface|string path
-- @see gears.surface

--- maximized_button_focus_inactive_press.
-- @beautiful beautiful.titlebar_maximized_button_focus_inactive_press
-- @tparam gears.surface|string path
-- @see gears.surface

--- ontop_button_focus_inactive.
-- @beautiful beautiful.titlebar_ontop_button_focus_inactive
-- @tparam gears.surface|string path
-- @see gears.surface

--- ontop_button_focus_inactive_hover.
-- @beautiful beautiful.titlebar_ontop_button_focus_inactive_hover
-- @tparam gears.surface|string path
-- @see gears.surface

--- ontop_button_focus_inactive_press.
-- @beautiful beautiful.titlebar_ontop_button_focus_inactive_press
-- @tparam gears.surface|string path
-- @see gears.surface

--- sticky_button_focus_inactive.
-- @beautiful beautiful.titlebar_sticky_button_focus_inactive
-- @tparam gears.surface|string path
-- @see gears.surface

--- sticky_button_focus_inactive_hover.
-- @beautiful beautiful.titlebar_sticky_button_focus_inactive_hover
-- @tparam gears.surface|string path
-- @see gears.surface

--- sticky_button_focus_inactive_press.
-- @beautiful beautiful.titlebar_sticky_button_focus_inactive_press
-- @tparam gears.surface|string path
-- @see gears.surface

--- Set a declarative widget hierarchy description.
-- See [The declarative layout system](../documentation/03-declarative-layout.md.html)
-- @param args An array containing the widgets disposition
-- @method setup


local all_titlebars = setmetatable({}, { __mode = 'k' })

-- Get a color for a titlebar, this tests many values from the array and the theme
local function get_color(name, c, args)
    local suffix = "_normal"
    if c.active then
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
-- @tparam[opt=false] boolean resize_client Resize the client to give space to
--   the titlebar.
-- @treturn boolean If the titlebars were loaded
local function load_titlebars(c, hide_all, keep, context, resize_client)
    if c._request_titlebars_called then return false end

    c:emit_signal("request::titlebars", context, {})

    if hide_all then
        -- Don't bother checking if it has been created, `.hide` don't works
        -- anyway.
        for _, tb in ipairs {"top", "bottom", "left", "right"} do
            if tb ~= keep then
                titlebar.hide {
                    client = c,
                    position = tb,
                    resize_client = resize_client
                }
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


--- Get a client's titlebar.
-- @tparam client c The client for which a titlebar is wanted.
-- @tparam[opt={}] table args A table with extra arguments for the titlebar.
-- @tparam[opt=font.height*1.5] number args.size The height of the titlebar.
-- @tparam[opt=top] string args.position" values are `top`,
-- `left`, `right` and `bottom`.
-- @tparam[opt=false] boolean args.resize_client Resize the client to give
--   space to the titlebar.
-- @tparam[opt=top] string args.bg_normal
-- @tparam[opt=top] string args.bg_focus
-- @tparam[opt=top] string args.bgimage_normal
-- @tparam[opt=top] string args.bgimage_focus
-- @tparam[opt=top] string args.fg_normal
-- @tparam[opt=top] string args.fg_focus
-- @tparam[opt=top] string args.font
-- @constructorfct awful.titlebar
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
            update_colors = update_colors
        }

        -- Update the colors when focus changes
        c:connect_signal("property::active", update_colors)

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

    local lay = c.screen.selected_tag and c.screen.selected_tag.layout or nil
    if ((lay and lay == floating) or c.floating) and args.resize_client then
        if position == "top" or position == "bottom" then
            local tb_height = ret.widget.height or 0
            c.height = c.height - tb_height
            if position == "top" then c.y = c.y + tb_height end
        elseif position == "right" or position == "left" then
            local tb_width = ret.widget.width or 0
            c.width = c.width - tb_width
            if position == "left" then c.x = c.x + tb_width end
        end
    end

    c._private = c._private or {}
    c._private.titlebars = bars

    return ret
end

--- Show a client's titlebar.
-- @tparam table args
-- @tparam client args.client The client whose titlebar is modified.
-- @tparam[opt="top"] string args.position The position of the titlebar. Must
--   be one of "left", "right", "top", "bottom".
-- @tparam[opt=false] boolean args.resize_client Resize the client to give
--   space to the titlebar.
-- @param[opt="top"] position **DEPRECATED** use `args.position` instead.
-- @tparam[opt=false] boolean resize_client **DEPRECATED** use `args.resize_client` instead.
-- @request client titlebars show granted Called when `awful.titlebar.show` is
--   called.
-- @staticfct awful.titlebar.show
function titlebar.show(args, position, resize_client)
    local client = nil

    if type(args.geometry) == 'function' then
        gdebug.deprecate(
            "The `c` paramater is deprecated, use `args.client`.",
            { deprecated_in = 5 })

        client = args
        args = {}
    end

    assert(type(args) == "table")

    for k, v in pairs {
        position = position,
        resize_client = resize_client
    } do
        gdebug.deprecate(
            "The `" .. k .. "` paramater is deprecated, use `args." .. k .. "`.",
            { deprecated_in = 5 })

        args[k] = v
    end

    client = client or args.client
    position = args.position or "top"
    resize_client = args.resize_client or false

    if load_titlebars(client, true, position, "show", resize_client) then return end

    local bars = all_titlebars[client]
    local data = bars and bars[position]
    local tb_args = data and data.args or {}

    tb_args.resize_client = resize_client

    local _, size = get_titlebar_function(client, position)(client)
    if size == 0 then
        new(client, tb_args)
    end
end

--- Hide a client's titlebar.
-- @tparam table args
-- @tparam client args.client The client whose titlebar is modified.
-- @tparam[opt="top"] string args.position The position of the titlebar. Must
--   be one of "left", "right", "top", "bottom".
-- @tparam[opt=false] boolean args.resize_client Resize the client to fill the
--   space used by the titlebar.
-- @param[opt="top"] position **DEPRECATED** use `args.position` instead.
-- @tparam[opt=false] boolean resize_client **DEPRECATED** use `args.resize_client` instead.
-- @staticfct awful.titlebar.hide
function titlebar.hide(args, position, resize_client)
    local client = nil

    if type(args.geometry) == 'function' then
        gdebug.deprecate(
            "The `c` paramater is deprecated, use `args.client`.",
            { deprecated_in = 5 })

        client = args
        args = {}
    end

    assert(type(args) == "table")

    for k, v in pairs {
        position = position,
        resize_client = resize_client
    } do
        gdebug.deprecate(
            "The `" .. k .. "` paramater is deprecated, use `args." .. k .. "`.",
            { deprecated_in = 5 })

        args[k] = v
    end

    client = client or args.client
    position = args.position or "top"
    resize_client = args.resize_client or false

    local tb = get_titlebar_function(client, position)
    local lay = client.screen.selected_tag and client.screen.selected_tag.layout or nil

    if ((lay and lay == floating) or client.floating) and resize_client then
        local _, tb_size = tb(client)

        if position == "top" or position == "bottom" then
            client.height = client.height + tb_size
            if position == "top" then client.y = client.y - tb_size end
        elseif position == "right" or position == "left" then
            client.width = client.width + tb_size
            if position == "left" then client.x = client.x - tb_size end
        end
    end

    tb(client, 0)
end

--- Toggle a client's titlebar, hiding it if it is visible, otherwise showing it.
-- @tparam table args
-- @tparam client args.client The client whose titlebar is modified.
-- @tparam[opt="top"] string args.position The position of the titlebar. Must
--   be one of "left", "right", "top", "bottom".
--@tparam[opt=false] boolean args.resize_client Resize the client to fill the
--   space used by the titlebar / give space to the titlebar.
-- @param[opt="top"] position **DEPRECATED** use `args.position` instead.
-- @tparam[opt=false] boolean resize_client **DEPRECATED** use `args.resize_client` instead.
-- @staticfct awful.titlebar.toggle
-- @request client titlebars toggle granted Called when `awful.titlebar.toggle` is
--  called.
function titlebar.toggle(args, position, resize_client)
    local client = nil

    if type(args.geometry) == 'function' then
        gdebug.deprecate(
            "The `c` paramater is deprecated, use `args.client`.",
            { deprecated_in = 5 })

        client = args
        args = {}
    end

    assert(type(args) == "table")

    for k, v in pairs {
        position = position,
        resize_client = resize_client
    } do
        gdebug.deprecate(
            "The `" .. k .. "` paramater is deprecated, use `args." .. k .. "`.",
            { deprecated_in = 5 })

        args[k] = v
    end

    client = client or args.client
    position = args.position or "top"
    resize_client = args.resize_client or false

    if load_titlebars(client, true, position, "toggle", resize_client) then return end
    local _, size = get_titlebar_function(client, position)(client)

    if size == 0 then
        titlebar.show {
            client = client,
            position = position,
            resize_client = resize_client
        }
    else
        titlebar.hide {
            client = client,
            position = position,
            resize_client = resize_client
        }
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

--- Create a new titlewidget. A title widget displays the name of a client.
-- Please note that this returns a textbox and all of textbox' API is available.
-- This way, you can e.g. modify the font that is used.
-- @param c The client for which a titlewidget should be created.
-- @return The title widget.
-- @staticfct awful.titlebar.widget.titlewidget
function titlebar.widget.titlewidget(c)
    local ret = textbox()
    local function update()
        ret:set_text(c.name or titlebar.fallback_name)
    end
    ret.update = update
    update_on_signal(c, "property::name", ret)
    update()

    return ret
end

--- Create a new icon widget. An icon widget displays the icon of a client.
-- Please note that this returns an imagebox and all of the imagebox' API is
-- available. This way, you can e.g. disallow resizes.
-- @param c The client for which an icon widget should be created.
-- @return The icon widget.
-- @staticfct awful.titlebar.widget.iconwidget
function titlebar.widget.iconwidget(c)
    return clienticon(c)
end

--- Create a new button widget. A button widget displays an image and reacts to
-- mouse clicks. Please note that the caller has to make sure that this widget
-- gets redrawn when needed by calling the returned widget's update() function.
-- The selector function should return a value describing a state. If the value
-- is a boolean, either "active" or "inactive" are used. The actual image is
-- then found in the theme as "titlebar_[name]_button_[normal/focus]_[state]".
-- If that value does not exist, the focused state is ignored for the next try.
-- @param c The client for which a button is created.
-- @tparam string name Name of the button, used for accessing the theme and
--   in the tooltip.
-- @param selector A function that selects the image that should be displayed.
-- @param action Function that is called when the button is clicked.
-- @return The widget
-- @staticfct awful.titlebar.widget.button
function titlebar.widget.button(c, name, selector, action)
    local ret = imagebox()

    if titlebar.enable_tooltip then
        ret._private.tooltip = atooltip({ objects = {ret}, delay_show = 1 })
        ret._private.tooltip:set_text(name)
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
        end
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
-- @param c The client for which the button is wanted.
-- @staticfct awful.titlebar.widget.floatingbutton
function titlebar.widget.floatingbutton(c)
    local widget = titlebar.widget.button(c, "floating", aclient.object.get_floating, aclient.floating.toggle)
    update_on_signal(c, "property::floating", widget)
    return widget
end

--- Create a new maximize button for a client.
-- @param c The client for which the button is wanted.
-- @staticfct awful.titlebar.widget.maximizedbutton
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
-- @param c The client for which the button is wanted.
-- @staticfct awful.titlebar.widget.minimizebutton
function titlebar.widget.minimizebutton(c)
    local widget = titlebar.widget.button(c, "minimize",
                                          function() return "" end,
                                          function(cl) cl.minimized = not cl.minimized end)
    update_on_signal(c, "property::minimized", widget)
    return widget
end

--- Create a new closing button for a client.
-- @param c The client for which the button is wanted.
-- @staticfct awful.titlebar.widget.closebutton
function titlebar.widget.closebutton(c)
    return titlebar.widget.button(c, "close", function() return "" end, function(cl) cl:kill() end)
end

--- Create a new ontop button for a client.
-- @param c The client for which the button is wanted.
-- @staticfct awful.titlebar.widget.ontopbutton
function titlebar.widget.ontopbutton(c)
    local widget = titlebar.widget.button(c, "ontop",
                                          function(cl) return cl.ontop end,
                                          function(cl, state) cl.ontop = not state end)
    update_on_signal(c, "property::ontop", widget)
    return widget
end

--- Create a new sticky button for a client.
-- @param c The client for which the button is wanted.
-- @staticfct awful.titlebar.widget.stickybutton
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
