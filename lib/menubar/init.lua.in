---------------------------------------------------------------------------
-- @author Alexander Yakushev &lt;yakushev.alex@gmail.com&gt;
-- @copyright 2011-2012 Alexander Yakushev
-- @release @AWESOME_VERSION@
---------------------------------------------------------------------------

-- Grab environment we need
local capi = {
    client = client,
    mouse = mouse,
    screen = screen
}
local awful = require("awful")
local common = require("awful.widget.common")
local theme = require("beautiful")
local wibox = require("wibox")

-- menubar
local menubar = { mt = {} }
menubar.menu_gen = require("menubar.menu_gen")
menubar.utils = require("menubar.utils")

--- List of menubar keybindings:
-- <p><ul>
-- <li>"Left"  | "C-j" select an item on the left</li>
-- <li>"Right" | "C-k" select an item on the right</li>
-- <li>"Backspace"     exit the current category if we are in any</li>
-- <li>"Escape"        exit the current directory or exit menubar</li>
-- <li>"Home"          select the first item</li>
-- <li>"End"           select the last</li>
-- <li>"Return"        execute the entry</li>
-- <li>"C-Return"      execute the command with awful.util.spawn</li>
-- <li>"C-M-Return"    execute the command in a terminal</li>
-- </ul></p>
--
-- @class table
-- @name Menubar keybindings

-- Options section

--- When true the .desktop files will be reparsed only when the
-- extension is initialized. Use this if menubar takes much time to
-- open.
menubar.cache_entries = true

--- When true the categories will be shown alongside application
-- entries.
menubar.show_categories = true

--- Specifies the geometry of the menubar. This is a table with the keys
-- x, y, width and height. Missing values are replaced via the screen's
-- geometry. However, missing height is replaced by the font size.
menubar.geometry = { width = nil,
                     height = nil,
                     x = nil,
                     y = nil }

--- Allows user to specify custom parameters for prompt.run function
-- (like colors).
menubar.prompt_args = {}

-- Private section
local current_item = 1
local previous_item = nil
local current_category = nil
local shownitems = nil
local instance = { prompt = nil,
                   widget = nil,
                   wibox = nil }

local common_args = { w = wibox.layout.fixed.horizontal(),
                      data = setmetatable({}, { __mode = 'kv' }) }

-- Wrap the text with the color span tag.
-- @param s The text.
-- @param c The desired text color.
-- @return the text wrapped in a span tag.
local function colortext(s, c)
    return "<span color='" .. c .. "'>" .. s .. "</span>"
end

-- Generate a pattern matching expression that ignores case.
-- @param s Original pattern matching expresion.
local function nocase (s)
    s = string.gsub(s, "%a",
                    function (c)
                        return string.format("[%s%s]", string.lower(c),
                                             string.upper(c))
                    end)
    return s
end

-- Get how the menu item should be displayed.
-- @param o The menu item.
-- @return item name, item background color, background image, item icon.
local function label(o)
    if o.focused then
        local color = awful.util.color_strip_alpha(theme.fg_focus)
        return colortext(o.name, color), theme.bg_focus, nil, o.icon
    else
        return o.name, theme.bg_normal, nil, o.icon
    end
end

-- Perform an action for the given menu item.
-- @param o The menu item.
-- @return if the function processed the callback, new awful.prompt command, new awful.prompt prompt text.
local function perform_action(o)
    if not o then return end
    if o.key then
        current_category = o.key
        local new_prompt = shownitems[current_item].name .. ": "
        previous_item = current_item
        current_item = 1
        return true, "", new_prompt
    elseif shownitems[current_item].cmdline then
        awful.util.spawn(shownitems[current_item].cmdline)
        -- Let awful.prompt execute dummy exec_callback and
        -- done_callback to stop the keygrabber properly.
        return false
    end
end

-- Update the menubar according to the command entered by user.
-- @param query The text to filter entries by.
local function menulist_update(query)
    local query = query or ""
    shownitems = {}
    local match_inside = {}

    -- First we add entries which names match the command from the
    -- beginning to the table shownitems, and the ones that contain
    -- command in the middle to the table match_inside.

    -- Add the categories
    if menubar.show_categories then
        for _, v in pairs(menubar.menu_gen.all_categories) do
            v.focused = false
            if not current_category and v.use then
                if string.match(v.name, nocase(query)) then
                    if string.match(v.name, "^" .. nocase(query)) then
                        table.insert(shownitems, v)
                    else
                        table.insert(match_inside, v)
                    end
                end
            end
        end
    end

    -- Add the applications according to their name and cmdline
    for i, v in ipairs(menu_entries) do
        v.focused = false
        if not current_category or v.category == current_category then
            if string.match(v.name, nocase(query))
                or string.match(v.cmdline, nocase(query)) then
                if string.match(v.name, "^" .. nocase(query))
                    or string.match(v.cmdline, "^" .. nocase(query)) then
                    table.insert(shownitems, v)
                else
                    table.insert(match_inside, v)
                end
            end
        end
    end

    -- Now add items from match_inside to shownitems
    for i, v in ipairs(match_inside) do
        table.insert(shownitems, v)
    end

    if #shownitems > 0 then
        -- Insert a run item value as the last choice
        table.insert(shownitems, { name = "Exec: " .. query, cmdline = query, icon = nil })

        if current_item > #shownitems then
            current_item = #shownitems
        end
        shownitems[current_item].focused = true
    else
        table.insert(shownitems, { name = "", cmdline = query, icon = nil })
    end

    common.list_update(common_args.w, nil, label,
                       common_args.data,
                       shownitems)
end

-- Create the menubar wibox and widgets.
local function initialize()
    instance.wibox = wibox({})
    instance.widget = menubar.get()
    instance.wibox.ontop = true
    instance.prompt = awful.widget.prompt()
    local layout = wibox.layout.fixed.horizontal()
    layout:add(instance.prompt)
    layout:add(instance.widget)
    instance.wibox:set_widget(layout)
end

--- Refresh menubar's cache by reloading .desktop files.
function menubar.refresh()
    menu_entries = menubar.menu_gen.generate()
end

-- Awful.prompt keypressed callback to be used when the user presses a key.
-- @param mod Table of key combination modifiers (Control, Shift).
-- @param key The key that was pressed.
-- @param comm The current command in the prompt.
-- @return if the function processed the callback, new awful.prompt command, new awful.prompt prompt text.
local function prompt_keypressed_callback(mod, key, comm)
    if key == "Left" or (mod.Control and key == "j") then
        current_item = math.max(current_item - 1, 1)
        return true
    elseif key == "Right" or (mod.Control and key == "k") then
        current_item = current_item + 1
        return true
    elseif key == "BackSpace" then
        if comm == "" and current_category then
            current_category = nil
            current_item = previous_item
            return true, nil, "Run: "
        end
    elseif key == "Escape" then
        if current_category then
            current_category = nil
            current_item = previous_item
            return true, nil, "Run: "
        end
    elseif key == "Home" then
        current_item = 1
        return true
    elseif key == "End" then
        current_item = #shownitems
        return true
    elseif key == "Return" or key == "KP_Enter" then
        if mod.Control then
            current_item = #shownitems
            if mod.Mod1 then
                -- add a terminal to the cmdline
                shownitems[current_item].cmdline = menubar.utils.terminal
                        .. " -e " .. shownitems[current_item].cmdline
            end
        end
        return perform_action(shownitems[current_item])
    end
    return false
end

--- Show the menubar on the given screen.
-- @param scr Screen number.
function menubar.show(scr)
    if not instance.wibox then
        initialize()
    elseif instance.wibox.visible then -- Menu already shown, exit
        return
    elseif not menubar.cache_entries then
        menubar.refresh()
    end

    -- Set position and size
    scr = scr or capi.mouse.screen or 1
    local scrgeom = capi.screen[scr].workarea
    local geometry = menubar.geometry
    instance.wibox:geometry({x = geometry.x or scrgeom.x,
                             y = geometry.y or scrgeom.y,
                             height = geometry.height or theme.get_font_height() * 1.5,
                             width = geometry.width or scrgeom.width})

    current_item = 1
    current_category = nil
    menulist_update()

    local prompt_args = menubar.prompt_args or {}
    prompt_args.prompt = "Run: "
    awful.prompt.run(prompt_args, instance.prompt.widget,
                function(s) end,            -- exe_callback function set to do nothing
                awful.completion.shell,     -- completion_callback
                awful.util.getdir("cache") .. "/history_menu",
                nil, menubar.hide, menulist_update, prompt_keypressed_callback
                )
    instance.wibox.visible = true
end

--- Hide the menubar.
function menubar.hide()
    instance.wibox.visible = false
end

--- Get a menubar wibox.
-- @return menubar wibox.
function menubar.get()
    if app_folders then
        menubar.menu_gen.all_menu_dirs = app_folders
    end
    menubar.refresh()
    -- Add to each category the name of its key in all_categories
    for k, v in pairs(menubar.menu_gen.all_categories) do
        v.key = k
    end
    return common_args.w
end

function menubar.mt:__call(...)
    return menubar.get(...)
end

return setmetatable(menubar, menubar.mt)

-- vim: filetype=lua:expandtab:shiftwidth=4:tabstop=8:softtabstop=4:textwidth=80
