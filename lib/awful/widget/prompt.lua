---------------------------------------------------------------------------
-- The widget version of `awful.prompt`.
--
-- @DOC_wibox_awidget_defaults_prompt_EXAMPLE@
--
-- @author Julien Danjou &lt;julien@danjou.info&gt;
-- @copyright 2009 Julien Danjou
-- @classmod awful.widget.prompt
---------------------------------------------------------------------------

--- The prompt foreground color.
-- @beautiful beautiful.prompt_fg
-- @param color
-- @see gears.color

--- The prompt background color.
-- @beautiful beautiful.prompt_bg
-- @param color
-- @see gears.color

local setmetatable = setmetatable

local completion = require("awful.completion")
local util = require("awful.util")
local spawn = require("awful.spawn")
local prompt = require("awful.prompt")
local beautiful = require("beautiful")
local textbox = require("wibox.widget.textbox")
local background = require("wibox.container.background")
local type = type

local widgetprompt = { mt = {} }

--- Run method for promptbox.
--
-- @param promptbox The promptbox to run.
local function run(promptbox)
    return prompt.run {
        prompt              = promptbox.prompt,
        textbox             = promptbox.widget,
        completion_callback = completion.shell,
        history_path        = util.get_cache_dir() .. "/history",
        exe_callback        = function (...)
            promptbox:spawn_and_handle_error(...)
        end,
    }
end

local function spawn_and_handle_error(self, ...)
    local result = spawn(...)
    if type(result) == "string" then
        self.widget:set_text(result)
    end
end

--- Create a prompt widget which will launch a command.
--
-- @param args Arguments table. "prompt" is the prompt to use.
-- @return A launcher widget.
-- @function awful.widget.prompt
function widgetprompt.new(args)
    args = args or {}
    local widget = textbox()
    local promptbox = background()

    promptbox.widget = widget
    promptbox.widget:set_ellipsize("start")
    promptbox.run = run
    promptbox.spawn_and_handle_error = spawn_and_handle_error
    promptbox.prompt = args.prompt or "Run: "
    promptbox.fg = beautiful.prompt_fg or beautiful.fg_normal
    promptbox.bg = beautiful.prompt_bg or beautiful.bg_normal
    return promptbox
end

function widgetprompt.mt:__call(...)
    return widgetprompt.new(...)
end

return setmetatable(widgetprompt, widgetprompt.mt)

-- vim: filetype=lua:expandtab:shiftwidth=4:tabstop=8:softtabstop=4:textwidth=80
