----------------------------------------------------------------------------
--- Manage a notification action list.
--
-- A notification action is a "button" that will trigger an action on the sender
-- process. `notify-send` doesn't support actions, but `libnotify` based
-- applications do.
--
--@DOC_wibox_nwidget_actionlist_simple_EXAMPLE@
--
-- This example has a custom vertical widget template:
--
--@DOC_wibox_nwidget_actionlist_fancy_EXAMPLE@
--
-- This example has a horizontal widget template and icons:
--
--@DOC_wibox_nwidget_actionlist_fancy_icons_EXAMPLE@
--
-- This example uses the theme/style variables instead of the template. This is
-- less flexible, but easier to put in the theme file. Note that each style
-- variable has a `beautiful` equivalent.
--
--@DOC_wibox_nwidget_actionlist_style_EXAMPLE@
--
-- @author Emmanuel Lepage Vallee &lt;elv1313@gmail.com&gt;
-- @copyright 2017 Emmanuel Lepage Vallee
-- @widgetmod naughty.list.actions
-- @supermodule wibox.widget.base
-- @see awful.widget.common
----------------------------------------------------------------------------

local wibox    = require("wibox")
local awcommon = require("awful.widget.common")
local abutton  = require("awful.button")
local gtable   = require("gears.table")
local beautiful= require("beautiful")

local module = {}

--- Whether or not to underline the action name.
-- @beautiful beautiful.notification_action_underline_normal
-- @param[opt=true] boolean

--- Whether or not to underline the selected action name.
-- @beautiful beautiful.notification_action_underline_selected
-- @param[opt=true] boolean

--- Whether or not the action label should be shown.
-- @beautiful beautiful.notification_action_icon_only
-- @param[opt=false] boolean

--- Whether or not the action icon should be shown.
-- @beautiful beautiful.notification_action_label_only
-- @param[opt=false] boolean

--- The shape used for a normal action.
-- @beautiful beautiful.notification_action_shape_normal
-- @tparam[opt=gears.shape.rectangle] gears.shape shape
-- @see gears.shape

--- The shape used for a selected action.
-- @beautiful beautiful.notification_action_shape_selected
-- @tparam[opt=gears.shape.rectangle] gears.shape shape
-- @see gears.shape

--- The shape border color for normal actions.
-- @beautiful beautiful.notification_action_shape_border_color_normal
-- @param color
-- @see gears.color

--- The shape border color for selected actions.
-- @beautiful beautiful.notification_action_shape_border_color_selected
-- @param color
-- @see gears.color

--- The shape border width for normal actions.
-- @beautiful beautiful.notification_action_shape_border_width_normal
-- @param[opt=0] number

--- The shape border width for selected actions.
-- @beautiful beautiful.notification_action_shape_border_width_selected
-- @param[opt=0] number

--- The action icon size.
-- @beautiful beautiful.notification_action_icon_size_normal
-- @param[opt=0] number

--- The selected action icon size.
-- @beautiful beautiful.notification_action_icon_size_selected
-- @param[opt=0] number

--- The background color for normal actions.
-- @beautiful beautiful.notification_action_bg_normal
-- @param color
-- @see gears.color

--- The background color for selected actions.
-- @beautiful beautiful.notification_action_bg_selected
-- @param color
-- @see gears.color

--- The foreground color for normal actions.
-- @beautiful beautiful.notification_action_fg_normal
-- @param color
-- @see gears.color

--- The foreground color for selected actions.
-- @beautiful beautiful.notification_action_fg_selected
-- @param color
-- @see gears.color

--- The background image for normal actions.
-- @beautiful beautiful.notification_action_bgimage_normal
-- @tparam gears.surface|string action_bgimage_normal
-- @see gears.surface

--- The background image for selected actions.
-- @beautiful beautiful.notification_action_bgimage_selected
-- @tparam gears.surface|string action_bgimage_selected
-- @see gears.surface

local props = {"shape_border_color", "bg_image" , "fg",
               "shape_border_width", "underline", "bg",
               "shape",              "icon_size",     }

-- Use a cached loop instead of an large function like the taglist and tasklist
local function update_style(self)
    self._private.style_cache = self._private.style_cache or {}

    for _, state in ipairs {"normal", "selected"} do
        local s = {}

        for _, prop in ipairs(props) do
            if self._private.style[prop.."_"..state] ~= nil then
                s[prop] = self._private.style[prop.."_"..state]
            else
                s[prop] = beautiful["notification_action_"..prop.."_"..state]
            end
        end

        -- Set a fallback for the icon size to prevent them from being gigantic
        s.icon_size = s.icon_size
            or beautiful.get_font_height(beautiful.font) * 1.5

        self._private.style_cache[state] = s
    end
end

local function wb_label(action, self)
    -- Get the name
    local name = action.name

    local style = self._private.style_cache[action.selected and "selected" or "normal"]

    -- Add the underline
    name = style.underline ~= false and
        ("<u>"..name.."</u>") or name

    local icon = beautiful.notification_action_label_only ~= true and action.icon or nil

    if style.fg then
        name = "<span color='" .. style.fg .. "'>" .. name .. "</span>"
    end

    if action.icon_only or beautiful.notification_action_icon_only then
        name = nil
    end

    return name, style.bg, style.bg_image, icon, style
end

local function update(self)
    local n = self._private.notification[1]

    if not self._private.layout or not n then return end

    awcommon.list_update(
        self._private.layout,
        self._private.default_buttons,
        function(o) return wb_label(o, self) end,
        self._private.data,
        n.actions,
        {
            widget_template = self._private.widget_template
        }
    )
end

local actionlist = {}

--- The actionlist parent notification.
-- @property notification
-- @tparam[opt=nil] naughty.notification|nil notification
-- @propemits true false
-- @see naughty.notification

--- The actionlist layout.
-- If no layout is specified, a `wibox.layout.fixed.horizontal` will be created
-- automatically.
-- @property base_layout
-- @tparam[opt=wibox.layout.fixed.horizontal] widget base_layout
-- @propemits true false
-- @see wibox.layout.fixed.horizontal

--- The actionlist parent notification.
-- @property widget_template
-- @tparam[opt=nil] wibox.template|nil widget_template
-- @propemits true false

--- A table with values to override each `beautiful.notification_action` values.
-- @property style
-- @tparam[opt={}] table|nil style
-- @tparam boolean|nil style.underline_normal
-- @tparam boolean|nil style.underline_selected
-- @tparam shape|nil style.shape_normal
-- @tparam shape|nil style.shape_selected
-- @tparam gears.color|string|nil style.shape_border_color_normal
-- @tparam gears.color|string|nil style.shape_border_color_selected
-- @tparam number|nil style.shape_border_width_normal
-- @tparam number|nil style.shape_border_width_selected
-- @tparam number|nil style.icon_size
-- @tparam color|string|nil style.bg_normal
-- @tparam color|string|nil style.bg_selected
-- @tparam color|string|nil style.fg_normal
-- @tparam color|string|nil style.fg_selected
-- @tparam surface|string|nil style.bgimage_normal
-- @tparam surface|string|nil style.bgimage_selected
-- @propemits true false
-- @usebeautiful beautiful.font Fallback when the `font` property isn't set.
-- @usebeautiful beautiful.notification_action_underline_normal Fallback.
-- @usebeautiful beautiful.notification_action_underline_selected Fallback.
-- @usebeautiful beautiful.notification_action_icon_only Fallback.
-- @usebeautiful beautiful.notification_action_label_only Fallback.
-- @usebeautiful beautiful.notification_action_shape_normal Fallback.
-- @usebeautiful beautiful.notification_action_shape_selected Fallback.
-- @usebeautiful beautiful.notification_action_shape_border_color_normal Fallback.
-- @usebeautiful beautiful.notification_action_shape_border_color_selected Fallback.
-- @usebeautiful beautiful.notification_action_shape_border_width_normal Fallback.
-- @usebeautiful beautiful.notification_action_shape_border_width_selected Fallback.
-- @usebeautiful beautiful.notification_action_icon_size_normal Fallback.
-- @usebeautiful beautiful.notification_action_icon_size_selected Fallback.
-- @usebeautiful beautiful.notification_action_bg_normal Fallback.
-- @usebeautiful beautiful.notification_action_bg_selected Fallback.
-- @usebeautiful beautiful.notification_action_fg_normal Fallback.
-- @usebeautiful beautiful.notification_action_fg_selected Fallback.
-- @usebeautiful beautiful.notification_action_bgimage_normal Fallback.
-- @usebeautiful beautiful.notification_action_bgimage_selected Fallback.


function actionlist:set_notification(notif)
    self._private.notification = setmetatable({notif}, {__mode="v"})

    if not self._private.layout then
        self._private.layout = wibox.layout.fixed.horizontal()
    end

    update(self)

    self:emit_signal("widget::layout_changed")
    self:emit_signal("widget::redraw_needed")
    self:emit_signal("property::notification", notif)
end

function actionlist:set_base_layout(layout)
    self._private.layout = layout

    update(self)

    self:emit_signal("widget::layout_changed")
    self:emit_signal("widget::redraw_needed")
    self:emit_signal("property::base_layout", layout)
end

function actionlist:set_widget_template(widget_template)
    self._private.widget_template = wibox.template.make_from_value(widget_template)

    -- Remove the existing instances
    self._private.data = {}

    update(self)

    self:emit_signal("widget::layout_changed")
    self:emit_signal("widget::redraw_needed")
    self:emit_signal("property::widget_template", self._private.widget_template)
end

function actionlist:set_style(style)
    self._private.style = style or {}

    update_style(self)
    update(self)

    self:emit_signal("widget::layout_changed")
    self:emit_signal("widget::redraw_needed")
    self:emit_signal("property::style", style)
end

function actionlist:get_notification()
    return self._private.notification
end

function actionlist:layout(_, width, height)
    if self._private.layout then
        return { wibox.widget.base.place_widget_at(self._private.layout, 0, 0, width, height) }
    end
end

function actionlist:fit(context, width, height)
    if not self._private.layout then
        return 0, 0
    end

    return wibox.widget.base.fit_widget(self, context, self._private.layout, width, height)
end

--- Create an action list.
--
-- @tparam table args
-- @tparam naughty.notification args.notification The notification.
-- @tparam widget args.base_layout The action layout.
-- @tparam table args.style Override the beautiful values.
-- @tparam boolean args.style.underline_normal
-- @tparam boolean args.style.underline_selected
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
-- @tparam[opt] table widget_template A custom widget to be used for each action.
-- @treturn widget The action widget.
-- @constructorfct naughty.list.actions

local function new(_, args)
    args = args or {}

    local wdg = wibox.widget.base.make_widget(nil, nil, {
        enable_properties = true,
    })

    gtable.crush(wdg, actionlist, true)

    wdg._private.data = {}
    wdg._private.notification = {}

    gtable.crush(wdg, args, false)

    wdg._private.style = wdg._private.style or {}

    update_style(wdg)

    wdg._private.default_buttons = gtable.join(
        abutton({ }, 1, function(a)
            local notif = wdg._private.notification[1]
            a:invoke(notif)
        end)
    )

    return wdg
end

--@DOC_object_COMMON@

return setmetatable(module, {__call = new})

-- vim: filetype=lua:expandtab:shiftwidth=4:tabstop=8:softtabstop=4:textwidth=80
