--DOC_GEN_IMAGE --DOC_HIDE
local parent    = ... --DOC_NO_USAGE --DOC_HIDE
local wibox     = require( "wibox"     ) --DOC_HIDE
local awful     = { prompt = require("awful.prompt"),--DOC_HIDE
                    util   = require("awful.util"), --DOC_HIDE
                    spawn  = function () end } --DOC_HIDE
local beautiful = require( "beautiful" ) --DOC_HIDE
local gfs       = require("gears.filesystem") --DOC_HIDE

    local atextbox = wibox.widget.textbox()

    -- Custom handler for the return value. This implementation does nothing,
    -- but you might want be notified of the failure, so it is part of this
    -- example.
    local function clear(result)
        atextbox.widget.text =
            type(result) == "string" and result or ""
    end

    local hooks = {
        -- Replace the "normal" `Return` with a custom one
        {{         }, "Return", awful.spawn},

        -- Spawn method to spawn in the current tag
        {{"Mod1"   }, "Return", function(command)
            clear(awful.spawn(command,{
                tag       = mouse.screen.selected_tag
            }))
        end},

        -- Spawn in the current tag as floating and on top
        {{"Shift"  }, "Return", function(command)
            clear(awful.spawn(command,{
                ontop     = true,
                floating  = true,
                tag       = mouse.screen.selected_tag
            }))
        end},

        -- Spawn in a new tag
        {{"Control"}, "Return", function(command)
            clear(awful.spawn(command,{
                new_tag = true
            }))
        end},

        -- Cancel
        {{         }, "Escape", function(_)
            clear()
        end},
    }

    awful.prompt.run {
        prompt        = "<b>Run: </b>",
        hooks         = hooks,
        textbox       = atextbox,
        history_path  = gfs.get_cache_dir() .. "/history",
        done_callback = clear,
    }

parent:add( wibox.widget {    --DOC_HIDE
    atextbox,  --DOC_HIDE
    bg = beautiful.bg_normal,  --DOC_HIDE
    widget = wibox.container.background  --DOC_HIDE
}) --DOC_HIDE
