---------------------------------------------------------------------------
--- A notification action.
--
-- A notification can have multiple actions to chose from. This module allows
-- to manage such actions.
--
-- @author Emmanuel Lepage Vallee &lt;elv1313@gmail.com&gt;
-- @copyright 2019 Emmanuel Lepage Vallee
-- @classmod naughty.action
---------------------------------------------------------------------------
local gtable  = require("gears.table" )
local gobject = require("gears.object")

local action = {}

--- Create a new action.
-- @function naughty.action
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
-- Only a single action can be selected per notification. It will be applied
-- when `my_notification:apply()` is called.
--
-- @property selected
-- @param boolean

--- The action position (index).
-- @property position
-- @param number

--- The action icon.
-- @property icon
-- @param gears.surface

--- The notification.
-- @property notification
-- @tparam naughty.notification notification

--- When a notification is invoked.
-- @signal invoked

function action:get_selected()
    return self._private.selected
end

function action:set_selected(value)
    self._private.selected = value
    self:emit_signal("property::selected", value)

    if self._private.notification then
        self._private.notification:emit_signal("property::actions")
    end

    --TODO deselect other actions from the same notification
end

function action:get_position()
    return self._private.position
end

function action:set_position(value)
    self._private.position = value
    self:emit_signal("property::position", value)

    if self._private.notification then
        self._private.notification:emit_signal("property::actions")
    end

    --TODO make sure the position is unique
end

for _, prop in ipairs { "name", "icon", "notification" } do
    action["get_"..prop] = function(self)
        return self._private[prop]
    end

    action["set_"..prop] = function(self, value)
        self._private[prop] = value
        self:emit_signal("property::"..prop, value)

        -- Make sure widgets with as an actionlist is updated.
        if self._private.notification then
            self._private.notification:emit_signal("property::actions")
        end
    end
end

--- Execute this action.
function action:invoke()
    assert(self._private.notification,
        "Cannot invoke an action without a notification")

    self:emit_signal("invoked")
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
    }

    rawset(ret, "_private", default)

    return ret
end

return setmetatable(action, {__call = new})
