---------------------------------------------------------------------------
-- @author Julien Danjou &lt;julien@danjou.info&gt;
-- @copyright 2009 Julien Danjou
-- @release @AWESOME_VERSION@
---------------------------------------------------------------------------

-- Grab environment we need
local ipairs = ipairs
local table = table
local capi =
{
    awesome = awesome,
    root = root
}

--- Startup notification module for awful
-- awful.startup_notification

local app_starting = {}

local cursor_waiting = "watch"

local function update_cursor()
    if #app_starting > 0 then
        capi.root.cursor(cursor_waiting)
    else
        capi.root.cursor("left_ptr")
    end
end

local function unregister_event(event_id)
    for k, v in ipairs(app_starting) do
        if v == event_id then
            table.remove(app_starting, k)
            update_cursor()
            break
        end
    end
end

local function register_event(event_id)
    table.insert(app_starting, event_id)
    update_cursor()
end

local function unregister_hook(event) unregister_event(event.id) end
local function register_hook(event) register_event(event.id) end

capi.awesome.connect_signal("spawn::initiated", register_hook)
capi.awesome.connect_signal("spawn::canceled", unregister_hook)
capi.awesome.connect_signal("spawn::completed", unregister_hook)
capi.awesome.connect_signal("spawn::timeout", unregister_hook)

-- vim: filetype=lua:expandtab:shiftwidth=4:tabstop=8:softtabstop=4:textwidth=80
