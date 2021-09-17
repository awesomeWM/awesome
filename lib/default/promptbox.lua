local awful = require("awful")
    -- Create a promptbox for each screen

local result  = {
    perscreen = true
}
local function worker(s)
    s.mypromptbox = awful.widget.prompt()
    return s.mypromptbox
end

table.insert(globalwidgets.right, setmetatable(result, { __call = function(_,...) return worker(...) end}))


awful.keyboard.append_global_keybindings({
    awful.key({ modkey }, "x",
              function ()
                  awful.prompt.run {
                    prompt       = "Run Lua code: ",
                    textbox      = awful.screen.focused().mypromptbox.widget,
                    exe_callback = awful.util.eval,
                    history_path = awful.util.get_cache_dir() .. "/history_eval"
                  }
              end,
              {description = "lua execute prompt", group = "awesome"}),
    awful.key({ modkey },            "r",     function () awful.screen.focused().mypromptbox:run() end,
              {description = "run prompt", group = "launcher"}),
})
