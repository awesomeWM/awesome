---------------------------------------------------------------------------
-- The widget version of `awful.prompt`.
--
-- @DOC_wibox_awidget_defaults_prompt_EXAMPLE@
--
-- @author Julien Danjou &lt;julien@danjou.info&gt;
-- @copyright 2009 Julien Danjou
-- @copyright 2018 Aire-One
-- @widgetmod awful.widget.prompt
-- @supermodule wibox.container.background
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
local gfs = require("gears.filesystem")
local gdebug = require("gears.debug")
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
        prompt               = promptbox.prompt,
        textbox              = promptbox.widget,
        fg_cursor            = promptbox.fg_cursor,
        bg_cursor            = promptbox.bg_cursor,
        ul_cursor            = promptbox.ul_cursor,
        font                 = promptbox.font,
        autoexec             = promptbox.autoexec,
        highlighter          = promptbox.highlighter,
        exe_callback         = promptbox.exe_callback,
        completion_callback  = promptbox.completion_callback,
        history_path         = promptbox.history_path,
        history_max          = promptbox.history_max,
        done_callback        = promptbox.done_callback,
        changed_callback     = promptbox.changed_callback,
        keypressed_callback  = promptbox.keypressed_callback,
        keyreleased_callback = promptbox.keyreleased_callback,
        hooks                = promptbox.hooks
    }
end

local function spawn_and_handle_error(self, ...)
    local f = self.with_shell and spawn.with_shell or spawn
    local result = f(...)
    if type(result) == "string" then
        self.widget:set_text(result)
    end
end

--- Always spawn using a shell.
--
-- When using the default `exe_callback`, use `awful.spawn.with_shell` instead
-- of `awful.spawn`. Depending on the ammount of customization to your shell
-- environment, this can increase startup time.
-- @property with_shell
-- @param[opt=false] boolean

--- Create a prompt widget which will launch a command.
-- For additional documentation about `args` parameter, please refer to
-- @{awful.prompt} and @{awful.prompt.run}.
--
-- @tparam table args Prompt arguments.
-- @tparam[opt="Run: "] string args.prompt Prompt text.
-- @tparam[opt=`beautiful.prompt_bg` or `beautiful.bg_normal`] color args.bg
--   Prompt background color.
-- @tparam[opt=`beautiful.prompt_fg` or `beautiful.fg_normal`] color args.fg
--   Prompt foreground color.
-- @tparam[opt] gears.color args.fg_cursor
-- @tparam[opt] gears.color args.bg_cursor
-- @tparam[opt] gears.color args.ul_cursor
-- @tparam[opt] string args.font
-- @tparam[opt] boolean args.autoexec
-- @tparam[opt] function args.highlighter A function to add syntax highlighting
--   to the command.
-- @tparam[opt] function args.exe_callback The callback function to call with
--   command as argument when finished.
-- @tparam[opt=false] boolean args.with_shell Use a (terminal) shell to execute this.
-- @tparam[opt=`awful.completion.shell`] function args.completion_callback
--   The callback function to call to get completion. See @{awful.prompt.run}
--   for details.
-- @tparam[opt=`gears.filesystem.get_cache_dir() .. '/history'`] string
--   args.history_path File path where the history should be saved.
-- @tparam[opt=50] integer args.history_max Set the maximum entries in
--   history file.
-- @tparam[opt] function args.done_callback
--   The callback function to always call without arguments, regardless of
--   whether the prompt was cancelled. See @{awful.prompt.run} for details.
-- @tparam[opt] function args.changed_callback The callback function to call
--   with command as argument when a command was changed.
-- @tparam[opt] function args.keypressed_callback The callback function to call
--   with mod table, key and command as arguments when a key was pressed.
-- @tparam[opt] function args.keyreleased_callback The callback function to call
--   with mod table, key and command as arguments when a key was pressed.
-- @tparam[opt] table args.hooks Similar to @{awful.key}. It will call a function
--   for the matching modifiers + key. See @{awful.prompt.run} for details.
-- @return An instance of prompt widget, inherits from
--   `wibox.container.background`.
-- @constructorfct awful.widget.prompt
function widgetprompt.new(args)
    args = args or {}
    local promptbox = background()
    promptbox.widget = textbox()
    promptbox.widget:set_ellipsize("start")
    promptbox.run = run
    promptbox.spawn_and_handle_error = spawn_and_handle_error
    promptbox.prompt = args.prompt or "Run: "
    promptbox.fg = args.fg or beautiful.prompt_fg or beautiful.fg_normal
    promptbox.bg = args.bg or beautiful.prompt_bg or beautiful.bg_normal
    promptbox.fg_cursor = args.fg_cursor or nil
    promptbox.bg_cursor = args.bg_cursor or nil
    promptbox.ul_cursor = args.ul_cursor or nil
    promptbox.font = args.font or nil
    promptbox.autoexec = args.autoexec or nil
    promptbox.highlighter = args.highlighter or nil
    promptbox.exe_callback = args.exe_callback or function (...)
        promptbox:spawn_and_handle_error(...)
    end
    promptbox.completion_callback = args.completion_callback or completion.shell
    promptbox.history_path = args.history_path or
        gfs.get_cache_dir() .. 'history'
    promptbox.history_max = args.history_max or nil
    promptbox.done_callback = args.done_callback or nil
    promptbox.changed_callback = args.changed_callback or nil
    promptbox.keypressed_callback = args.keypressed_callback or nil
    promptbox.keyreleased_callback = args.keyreleased_callback or nil

    if args.hook and not args.hooks then
        gdebug.deprecate("Use `args.hooks` instead of `args.hook`",
            {deprecated_in=5})

        args.hooks = args.hook
    end

    promptbox.hooks = args.hooks or nil
    return promptbox
end

function widgetprompt.mt:__call(...)
    return widgetprompt.new(...)
end

return setmetatable(widgetprompt, widgetprompt.mt)

-- vim: filetype=lua:expandtab:shiftwidth=4:tabstop=8:softtabstop=4:textwidth=80
