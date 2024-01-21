----------------------------------------------------------------------------
--- Get a list of all currently active notifications.
--
-- @DOC_awful_notification_notificationlist_bottombar_EXAMPLE@
--
-- @author Emmanuel Lepage Vallee &lt;elv1313@gmail.com&gt;
-- @copyright 2017 Emmanuel Lepage Vallee
-- @widgetmod naughty.list.notifications
-- @supermodule wibox.widget.base
-- @see awful.widget.common
----------------------------------------------------------------------------

local wibox    = require("wibox")
local awcommon = require("awful.widget.common")
local abutton  = require("awful.button")
local gtable   = require("gears.table")
local gtimer   = require("gears.timer")
local beautiful= require("beautiful")
local naughty  = require("naughty.core")

local default_widget = require("naughty.widget._default")

local module = {}

--- The shape used for a normal notification.
-- @beautiful beautiful.notification_shape_normal
-- @tparam[opt=gears.shape.rectangle] gears.shape shape
-- @see gears.shape

--- The shape used for a selected notification.
-- @beautiful beautiful.notification_shape_selected
-- @tparam[opt=gears.shape.rectangle] gears.shape shape
-- @see gears.shape

--- The shape border color for normal notifications.
-- @beautiful beautiful.notification_shape_border_color_normal
-- @param color
-- @see gears.color

--- The shape border color for selected notifications.
-- @beautiful beautiful.notification_shape_border_color_selected
-- @param color
-- @see gears.color

--- The shape border width for normal notifications.
-- @beautiful beautiful.notification_shape_border_width_normal
-- @param[opt=0] number

--- The shape border width for selected notifications.
-- @beautiful beautiful.notification_shape_border_width_selected
-- @param[opt=0] number

--- The notification icon size.
-- @beautiful beautiful.notification_icon_size_normal
-- @param[opt=0] number

--- The selected notification icon size.
-- @beautiful beautiful.notification_icon_size_selected
-- @param[opt=0] number

--- The background color for normal notifications.
-- @beautiful beautiful.notification_bg_normal
-- @param color
-- @see gears.color

--- The background color for selected notifications.
-- @beautiful beautiful.notification_bg_selected
-- @param color
-- @see gears.color

--- The foreground color for normal notifications.
-- @beautiful beautiful.notification_fg_normal
-- @param color
-- @see gears.color

--- The foreground color for selected notifications.
-- @beautiful beautiful.notification_fg_selected
-- @param color
-- @see gears.color

--- The background image for normal notifications.
-- @beautiful beautiful.notification_bgimage_normal
-- @tparam string|gears.surface bgimage_normal
-- @see gears.surface

--- The background image for selected notifications.
-- @beautiful beautiful.notification_bgimage_selected
-- @tparam string|gears.surface bgimage_selected
-- @see gears.surface

local default_buttons = gtable.join(
    abutton({ }, 1, function(n) n:destroy() end),
    abutton({ }, 3, function(n) n:destroy() end)
)

local props = {"shape_border_color", "bg_image" , "fg",
               "shape_border_width", "shape"    , "bg",
               "icon_size"}

-- Use a cached loop instead of an large function like the taglist and tasklist
local function update_style(self)
    self._private.style_cache = self._private.style_cache or {}

    for _, state in ipairs {"normal", "selected"} do
        local s = {}

        for _, prop in ipairs(props) do
            if self._private.style[prop.."_"..state] ~= nil then
                s[prop] = self._private.style[prop.."_"..state]
            else
                s[prop] = beautiful["notification_"..prop.."_"..state]
            end
        end

        self._private.style_cache[state] = s
    end
end

local function wb_label(notification, self)
    -- Get the title
    local title = notification.title

    local style = self._private.style_cache[notification.selected and "selected" or "normal"]

    if notification.fg or style.fg then
        title = "<span color='" .. (notification.fg or style.fg) .. "'>" .. title .. "</span>"
    end

    return title, notification.bg or style.bg, style.bg_image, notification.icon, {
        shape              = notification.shape         or style.shape,
        shape_border_width =  notification.border_width or style.shape_border_width,
        shape_border_color =  notification.border_color or style.shape_border_color,
        icon_size          =  style.icon_size,
    }
end

-- Remove some callback boilerplate from the user provided templates.
local function create_callback(w, n)
    awcommon._set_common_property(w, "notification", n)
end

local function update(self)
    -- Checking style_cache helps to avoid useless redraw during initialization
    if not self._private.base_layout or not self._private.style_cache then return end

    awcommon.list_update(
        self._private.base_layout,
        default_buttons,
        function(o) return wb_label(o, self) end,
        self._private.data,
        naughty.active,
        {
            create_callback = create_callback,
            widget_template = self._private.widget_template or default_widget
        }
    )
end

local notificationlist = {}

--- The notificationlist parent notification.
-- @property notification
-- @tparam naughty.notification notification
-- @propertydefault This is usually set in the construtor.
-- @propemits true false
-- @see naughty.notification

--- A `wibox.layout` to be used to place the entries.
--
-- If no layout is specified, a `wibox.layout.flex.horizontal` will be created
-- automatically.
--
-- @property base_layout
-- @tparam[opt=wibox.layout.flex.horizontal] widget base_layout
-- @propemits true false
-- @usebeautiful beautiful.notification_spacing
-- @see wibox.layout.fixed.horizontal
-- @see wibox.layout.fixed.vertical
-- @see wibox.layout.flex.horizontal
-- @see wibox.layout.flex.vertical
-- @see wibox.layout.grid

--- The notificationlist parent notification.
-- @property widget_template
-- @tparam[opt=nil] wibox.template|nil widget_template
-- @propertydefault The default template displays the icon, title, message and
--  actions.
-- @propemits true false

--- A table with values to override each `beautiful.notification_action` values.
-- @property style
-- @tparam[opt=nil] table|nil style
-- @propertytype nil Use the values from `beautiful` rather than hardcoded ones.
-- @propemits true false
-- @usebeautiful beautiful.notification_shape_normal Fallback.
-- @usebeautiful beautiful.notification_shape_selected Fallback.
-- @usebeautiful beautiful.notification_shape_border_color_normal Fallback.
-- @usebeautiful beautiful.notification_shape_border_color_selected Fallback.
-- @usebeautiful beautiful.notification_shape_border_width_normal Fallback.
-- @usebeautiful beautiful.notification_shape_border_width_selected Fallback.
-- @usebeautiful beautiful.notification_icon_size_normal Fallback.
-- @usebeautiful beautiful.notification_icon_size_selected Fallback.
-- @usebeautiful beautiful.notification_bg_normal Fallback.
-- @usebeautiful beautiful.notification_bg_selected Fallback.
-- @usebeautiful beautiful.notification_fg_normal Fallback.
-- @usebeautiful beautiful.notification_fg_selected Fallback.
-- @usebeautiful beautiful.notification_bgimage_normal Fallback.
-- @usebeautiful beautiful.notification_bgimage_selected Fallback.

function notificationlist:set_widget_template(widget_template)
    self._private.widget_template = wibox.template.make_from_value(widget_template)

    -- Remove the existing instances
    self._private.data = {}

    update(self)

    self:emit_signal("widget::layout_changed")
    self:emit_signal("widget::redraw_needed")
    self:emit_signal("property::widget_template", self._private.widget_template)
end

function notificationlist:set_style(style)
    self._private.style = style or {}

    update_style(self)
    update(self)

    self:emit_signal("widget::layout_changed")
    self:emit_signal("widget::redraw_needed")
    self:emit_signal("property::style", style)
end

function notificationlist:layout(_, width, height)
    if self._private.base_layout then
        return { wibox.widget.base.place_widget_at(self._private.base_layout, 0, 0, width, height) }
    end
end

function notificationlist:fit(context, width, height)
    if not self._private.base_layout then
        return 0, 0
    end

    return wibox.widget.base.fit_widget(self, context, self._private.base_layout, width, height)
end

--- A function to prevent some notifications from being added to the list.
-- @property filter
-- @tparam[opt=nil] function|nil filter
-- @functionparam naughty.notification n The notification object.
-- @functionparam number count The number of notifications in the list.
-- @functionreturn boolean `true` if the notification is allowed and `false` if
--  it is rejected.
-- @propemits true false

for _, prop in ipairs { "filter", "base_layout" } do
    notificationlist["set_"..prop] = function(self, value)
        self._private[prop] = value

        update(self)

        self:emit_signal("widget::layout_changed")
        self:emit_signal("widget::redraw_needed")
        self:emit_signal("property::"..prop, value)
    end

    notificationlist["get_"..prop] = function(self)
        return self._private[prop]
    end
end

--- Create an notification list.
--
-- @tparam table args
-- @tparam widget args.base_layout The notification list base_layout.
-- @tparam widget args.filter The list filter.
-- @tparam table args.style Override the beautiful values.
-- @tparam gears.shape args.style.shape_normal
-- @tparam gears.shape args.style.shape_selected
-- @tparam gears.color|string args.style.shape_border_color_normal
-- @tparam gears.color|string args.style.shape_border_color_selected
-- @tparam number args.style.shape_border_width_normal
-- @tparam number args.style.shape_border_width_selected
-- @tparam number args.style.icon_size
-- @tparam gears.color|string args.style.bg_normal
-- @tparam gears.color|string args.style.bg_selected
-- @tparam gears.color|string args.style.fg_normal
-- @tparam gears.color|string args.style.fg_selected
-- @tparam gears.surface|string args.style.bgimage_normal
-- @tparam gears.surface|string args.style.bgimage_selected
-- @tparam[opt] wibox.template args.widget_template A custom widget to be used for each
--  notifications.
-- @treturn widget The notification list widget.
-- @constructorfct naughty.list.notifications

local function new(_, args)
    args = args or {}

    local wdg = wibox.widget.base.make_widget(nil, nil, {
        enable_properties = true,
    })

    gtable.crush(wdg, notificationlist, true)

    wdg._private.data = {}

    gtable.crush(wdg, args)

    wdg._private.style = wdg._private.style or {}

    -- Don't do this right away since the base_layout may not have been set yet.
    -- This also avoids `update()` being executed during initialization and
    -- causing an output that isn't reproducible.
    gtimer.delayed_call(function()
        update_style(wdg)

        if not wdg._private.base_layout then
            wdg._private.base_layout = wibox.layout.flex.horizontal()
            wdg._private.base_layout:set_spacing(beautiful.notification_spacing or 0)
            wdg:emit_signal("widget::layout_changed")
            wdg:emit_signal("widget::redraw_needed")
        end

        update(wdg)

        local is_scheduled = false

        -- Prevent multiple updates due to the many signals.
        local function f()
            if is_scheduled then return end

            is_scheduled = true

            gtimer.delayed_call(function() update(wdg); is_scheduled = false end)
        end

        -- Yes, this will cause 2 updates when a new notification arrives, but
        -- on the other hand, request::display is required to auto-disable the
        -- fallback legacy mode and property::active is needed to remove the
        -- destroyed notifications.
        naughty.connect_signal("property::active", f)
        naughty.connect_signal("request::display", f)
    end)

    return wdg
end

module.filter = {}

--- All notifications.
-- @tparam naughty.notification n The notification.
-- @treturn boolean Always returns true because it doesn't filter anything at all.
-- @filterfunction naughty.list.notifications.filter.all
function module.filter.all(n) -- luacheck: no unused args
    return true
end

--- Only get the most recent notification(s).
--
-- To set the count, the function needs to be wrapped:
--
--    filter = function(n) return naughty.list.notifications.filter.most_recent(n, 3) end
--
-- @tparam naughty.notification n The notification.
-- @tparam[opt=1] number count The number of recent notifications to allow.
-- @treturn boolean Always returns true because it doesn't filter anything at all.
-- @filterfunction naughty.list.notifications.filter.most_recent
function module.filter.most_recent(n, count)
    for i=1, count or 1 do
        if n == naughty.active[i] then
            return true
        end
    end

    return false
end

--@DOC_object_COMMON@

return setmetatable(module, {__call = new})

-- vim: filetype=lua:expandtab:shiftwidth=4:tabstop=8:softtabstop=4:textwidth=80
