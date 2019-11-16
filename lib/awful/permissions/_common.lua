-- Common code for the permission framework.
-- It is in its own file to avoid circular dependencies.
--
-- It is **NOT** a public API.

local module = {}

-- The default permissions for all requests.
-- If something is not in this list, then it is assumed it should be granted.
local default_permissions = {
    client = {
        autoactivate = {
            -- To preserve the default from AwesomeWM 3.3-4.3, do not steal
            -- focus by default for these contexts:
            mouse_enter = false,
            switch_tag  = false,
            history     = false,
        }
    }
}

function module.check(class, request, context)
    if not default_permissions[class] then return true end
    if not default_permissions[class][request] then return true end
    if default_permissions[class][request][context] == nil then return true end

    return default_permissions[class][request][context]
end

function module.set(class, request, context, granted)
    assert(type(granted) == "boolean")

    if not default_permissions[class] then
       default_permissions[class] = {}
    end

    if not default_permissions[class][request] then
        default_permissions[class][request] = {}
    end

    default_permissions[class][request][context] = granted
end

-- Awesome 3.3-4.3 had an `awful.autofocus` module. That module had no APIs, but
-- was simply "enabled" when you `require()`d it for the first time. This was
-- non-standard and was the only module in `awful` to only do things when
-- explicitly `require()`d.
--
-- It is now a dummy module which will set the property to `true`.
function module._deprecated_autofocus_in_use()
    module.set("client", "autoactivate", "mouse_enter", true)
    module.set("client", "autoactivate", "switch_tag" , true)
    module.set("client", "autoactivate", "history"    , true)
end

return module
