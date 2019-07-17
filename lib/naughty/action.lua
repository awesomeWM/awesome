---------------------------------------------------------------------------
--- A notification action.
--
-- A notification can have multiple actions to chose from. This module allows
-- to manage such actions. An action object can be shared by multiple
-- notifications.
--
-- @author Emmanuel Lepage Vallee &lt;elv1313@gmail.com&gt;
-- @copyright 2019 Emmanuel Lepage Vallee
-- @coreclassmod naughty.action
---------------------------------------------------------------------------
local gtable  = require("gears.table" )
local gobject = require("gears.object")

local action = {}

--- Create a new action.
-- @constructorfct naughty.action
-- @tparam table args The arguments.
-- @tparam string args.name The name.
-- @tparam string args.position The position.
-- @tparam string args.icon The icon.
-- @tparam naughty.notification args.notification The notification object.
-- @tparam boolean args.selected If this action is currently selected.
-- @return A new action.

-- The action name.
-- @property name
-- @tparam string name The name.

-- If the action is selected.
--
-- Only a single action can be selected per notification. This is useful to
-- implement keyboard navigation.
--
-- @property selected
-- @param boolean

--- The action position (index).
-- @property position
-- @param number

--- The action icon.
-- @property icon
-- @tparam gears.surface|string icon

--- If the action should hide the label and only display the icon.
--
-- @DOC_wibox_nwidget_actionlist_icon_only_EXAMPLE@
--
-- @property icon_only
-- @param[opt=false] boolean

--- When a notification is invoked.
-- @signal invoked

function action:get_selected()
    return self._private.selected
end

function action:set_selected(value)
    self._private.selected = value
    self:emit_signal("property::selected", value)
    self:emit_signal("_changed")

    --TODO deselect other actions from the same notification
end

function action:get_position()
    return self._private.position
end

function action:set_position(value)
    self._private.position = value
    self:emit_signal("property::position", value)
    self:emit_signal("_changed")

    --TODO make sure the position is unique
end

for _, prop in ipairs { "name", "icon", "icon_only" } do
    action["get_"..prop] = function(self)
        return self._private[prop]
    end

    action["set_"..prop] = function(self, value)
        self._private[prop] = value
        self:emit_signal("property::"..prop, value)
        self:emit_signal("_changed")
    end
end

--TODO v4.5, remove this.
function action.set_notification()
    -- It didn't work because it prevented actions defined in the rules to be
    -- in multiple notifications at once.
    assert(
        false,
        "Setting a notification object was a bad idea and is now forbidden"
    )
end

--- Execute this action.
--
-- This only emits the `invoked` signal.
--
-- @method invoke
-- @tparam[opt={}] naughty.notification notif A notification object on which
--  the action was invoked. If a notification is shared by many object (like
--  a "mute" or "snooze" action added to all notification), calling `:invoke()`
--  without adding the `notif` context will cause unexpected results.
function action:invoke(notif)
    self:emit_signal("invoked", notif)
end

local function new(_, args)
    args = args or {}
    local ret = gobject { enable_properties = true }

    gtable.crush(ret, action, true)

    local default = {
        -- See "table 1" of the spec about the default name
        name         = args.name or "default",
        selected     = args.selected == true,
        position     = args.position,
        icon         = args.icon,
        notification = args.notification,
        icon_only    = args.icon_only or false,
    }

    rawset(ret, "_private", default)

    gtable.crush(ret, args)

    return ret
end

--@DOC_object_COMMON@

return setmetatable(action, {__call = new})
