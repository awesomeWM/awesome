---------------------------------------------------------------------------
--- Utility module for menubar
--
-- @author Antonio Terceiro
-- @copyright 2009, 2011-2012 Antonio Terceiro, Alexander Yakushev
-- @module menubar.utils
---------------------------------------------------------------------------

-- Grab environment
local table = table
local ipairs = ipairs
local string = string
local screen = screen
local gfs = require("gears.filesystem")
local theme = require("beautiful")
local lgi = require("lgi")
local gio = lgi.Gio
local glib = lgi.GLib
local w_textbox = require("wibox.widget.textbox")
local gdebug = require("gears.debug")
local protected_call = require("gears.protected_call")
local gstring = require("gears.string")
local unpack = unpack or table.unpack -- luacheck: globals unpack (compatibility with Lua 5.1)

local utils = {}

-- NOTE: This icons/desktop files module was written according to the
-- following freedesktop.org specifications:
-- Icons: http://standards.freedesktop.org/icon-theme-spec/icon-theme-spec-0.11.html
-- Desktop files: http://standards.freedesktop.org/desktop-entry-spec/desktop-entry-spec-1.0.html

-- Options section

--- Terminal which applications that need terminal would open in.
utils.terminal = 'xterm'

--- The default icon for applications that don't provide any icon in
-- their .desktop files.
local default_icon = nil

--- Name of the WM for the OnlyShowIn entry in the .desktop file.
utils.wm_name = "awesome"

-- Maps keys in desktop entries to suitable getter function.
-- The order of entries is as in the spec.
-- https://standards.freedesktop.org/desktop-entry-spec/latest/ar01s05.html
local keys_getters
do
    local function get_string(kf, key)
        return kf:get_string("Desktop Entry", key)
    end
    local function get_strings(kf, key)
        return kf:get_string_list("Desktop Entry", key, nil)
    end
    local function get_localestring(kf, key)
        return kf:get_locale_string("Desktop Entry", key, nil)
    end
    local function get_localestrings(kf, key)
        return kf:get_locale_string_list("Desktop Entry", key, nil, nil)
    end
    local function get_boolean(kf, key)
        return kf:get_boolean("Desktop Entry", key)
    end

    keys_getters = {
        Type = get_string,
        Version = get_string,
        Name = get_localestring,
        GenericName = get_localestring,
        NoDisplay = get_boolean,
        Comment = get_localestring,
        Icon = get_localestring,
        Hidden = get_boolean,
        OnlyShowIn = get_strings,
        NotShowIn = get_strings,
        DBusActivatable = get_boolean,
        TryExec = get_string,
        Exec = get_string,
        Path = get_string,
        Terminal = get_boolean,
        Actions = get_strings,
        MimeType = get_strings,
        Categories = get_strings,
        Implements = get_strings,
        Keywords = get_localestrings,
        StartupNotify = get_boolean,
        StartupWMClass = get_string,
        URL = get_string,
    }
end

-- Private section

local do_protected_call, call_callback
do
    -- Lua 5.1 cannot yield across a protected call. Instead of hardcoding a
    -- check, we check for this problem: The following coroutine yields true on
    -- success (so resume() returns true, true). On failure, pcall returns
    -- false and a message, so resume() returns true, false, message.
    local _, has_yieldable_pcall = coroutine.resume(coroutine.create(function()
        return pcall(coroutine.yield, true)
    end))
    if has_yieldable_pcall then
        do_protected_call = protected_call.call
        call_callback = function(callback, ...)
            return callback(...)
        end
    else
        do_protected_call = function(f, ...)
            return f(...)
        end
        call_callback = protected_call.call
    end
end

local all_icon_sizes = {
    'scalable',
    '128x128',
    '96x96',
    '72x72',
    '64x64',
    '48x48',
    '36x36',
    '32x32',
    '24x24',
    '22x22',
    '16x16'
}

--- List of supported icon formats.
local supported_icon_formats = { png = 1, xpm = 2, svg = 3 }

local icon_lookup_path = nil
--- Get a list of icon lookup paths.
-- @treturn table A list of directories, without trailing slash.
local function get_icon_lookup_path()
    if icon_lookup_path then return icon_lookup_path end

    local function ensure_args(t, paths)
        if type(paths) == 'string' then paths = { paths } end
        return t or {}, paths
    end

    local function add_if_readable(t, paths)
        t, paths = ensure_args(t, paths)

        for _, path in ipairs(paths) do
            if gfs.dir_readable(path) then
                table.insert(t, path)
            end
        end
        return t
    end

    local function add_with_dir(t, paths, dir)
        t, paths = ensure_args(t, paths)
        dir = { nil, dir }

        for _, path in ipairs(paths) do
            dir[1] = path
            table.insert(t, glib.build_filenamev(dir))
        end
        return t
    end

    icon_lookup_path = {}
    local theme_priority = { 'hicolor' }
    if theme.icon_theme then table.insert(theme_priority, 1, theme.icon_theme) end

    local paths = add_with_dir({}, glib.get_home_dir(), '.icons')
    add_with_dir(paths, {
        glib.get_user_data_dir(),           -- $XDG_DATA_HOME, typically $HOME/.local/share
        unpack(glib.get_system_data_dirs()) -- $XDG_DATA_DIRS, typically /usr/{,local/}share
    }, 'icons')
    add_with_dir(paths, glib.get_system_data_dirs(), 'pixmaps')

    local icon_theme_paths = {}
    for _, theme_dir in ipairs(theme_priority) do
        add_if_readable(icon_theme_paths,
                        add_with_dir({}, paths, theme_dir))
    end

    local app_in_theme_paths = {}
    for _, icon_theme_directory in ipairs(icon_theme_paths) do
        for _, size in ipairs(all_icon_sizes) do
            table.insert(app_in_theme_paths,
                         glib.build_filenamev({ icon_theme_directory,
                                                size, 'apps' }))
            table.insert(app_in_theme_paths,
                         glib.build_filenamev({ icon_theme_directory,
                                                size, 'categories' }))
        end
    end
    add_if_readable(icon_lookup_path, app_in_theme_paths)

    return add_if_readable(icon_lookup_path, paths)
end

--- Remove CR newline from the end of the string.
-- @param s string to trim
function utils.rtrim(s)
    if not s then return end
    if string.byte(s, #s) == 13 then
        return string.sub(s, 1, #s - 1)
    end
    return s
end

--- Lookup an icon in different folders of the filesystem.
-- @tparam string icon_file Short or full name of the icon.
-- @treturn string|boolean Full name of the icon, or false on failure.
function utils.lookup_icon_uncached(icon_file)
    if not icon_file or icon_file == "" then
        return false
    end

    local icon_file_ext = icon_file:match(".*%.(.-)$")
    if icon_file:sub(1, 1) == '/' and supported_icon_formats[icon_file_ext] then
        -- If the path to the icon is absolute and its format is
        -- supported, do not perform a lookup.
        return gfs.file_readable(icon_file) and icon_file or nil
    else
        for _, directory in ipairs(get_icon_lookup_path()) do
            local directory_file = directory .. "/" .. icon_file
            if supported_icon_formats[icon_file_ext] and
                    gfs.file_readable(directory_file) then
                return directory_file
            else
                -- Icon is probably specified without path and format,
                -- like 'firefox'. Try to add supported extensions to
                -- it and see if such file exists.
                for format, _ in pairs(supported_icon_formats) do
                    local possible_file = directory_file .. "." .. format
                    if gfs.file_readable(possible_file) then
                        return possible_file
                    end
                end
            end
        end
        return false
    end
end

local lookup_icon_cache = {}
--- Lookup an icon in different folders of the filesystem (cached).
-- @param icon Short or full name of the icon.
-- @return full name of the icon.
function utils.lookup_icon(icon)
    if not lookup_icon_cache[icon] and lookup_icon_cache[icon] ~= false then
        lookup_icon_cache[icon] = utils.lookup_icon_uncached(icon)
    end
    return lookup_icon_cache[icon] or default_icon
end

--- Parse a .desktop file.
-- @param file The .desktop file.
-- @return A table with file entries.
function utils.parse_desktop_file(file)
    local program = { show = true, file = file }

    -- Parse the .desktop file.
    -- We are interested in [Desktop Entry] group only.
    local keyfile = glib.KeyFile()
    if not keyfile:load_from_file(file, glib.KeyFileFlags.NONE) then
        return nil
    end

    -- In case [Desktop Entry] was not found
    if not keyfile:has_group("Desktop Entry") then
        return nil
    end

    for _, key in pairs(keyfile:get_keys("Desktop Entry")) do
        local getter = keys_getters[key] or function(kf, k)
            return kf:get_string("Desktop Entry", k)
        end
        program[key] = getter(keyfile, key)
    end

    -- In case the (required) 'Name' entry was not found
    if not program.Name or program.Name == '' then return nil end

    -- Don't show program if NoDisplay attribute is true
    if program.NoDisplay then
        program.show = false
    else
        -- Only check these values is NoDisplay is true (or non-existent)

        -- Only show the program if there is no OnlyShowIn attribute
        -- or if it contains wm_name or wm_name is empty
        if utils.wm_name ~= "" then
            if program.OnlyShowIn then
                program.show = false -- Assume false until found
                for _, wm in ipairs(program.OnlyShowIn) do
                    if wm == utils.wm_name then
                        program.show = true
                        break
                    end
                end
            else
                program.show = true
            end
        end

        -- Only need to check NotShowIn if the program is being shown
        if program.show and program.NotShowIn then
            for _, wm in ipairs(program.NotShowIn) do
                if wm == utils.wm_name then
                    program.show = false
                    break
                end
            end
        end
    end

    -- Look up for a icon.
    if program.Icon then
        program.icon_path = utils.lookup_icon(program.Icon)
    end

    -- Make the variable lower-case like the rest of them
    if program.Categories then
        program.categories = program.Categories
    end

    if program.Exec then
        -- Substitute Exec special codes as specified in
        -- http://standards.freedesktop.org/desktop-entry-spec/1.1/ar01s06.html
        if program.Name == nil then
            program.Name = '['.. file:match("([^/]+)%.desktop$") ..']'
        end
        local cmdline = program.Exec:gsub('%%c', program.Name)
        cmdline = cmdline:gsub('%%[fuFU]', '')
        cmdline = cmdline:gsub('%%k', program.file)
        if program.icon_path then
            cmdline = cmdline:gsub('%%i', '--icon ' .. program.icon_path)
        else
            cmdline = cmdline:gsub('%%i', '')
        end
        if program.Terminal == true then
            cmdline = utils.terminal .. ' -e ' .. cmdline
        end
        program.cmdline = cmdline
    end

    return program
end

--- Parse a directory with .desktop files recursively.
-- @tparam string dir_path The directory path.
-- @tparam function callback Will be fired when all the files were parsed
-- with the resulting list of menu entries as argument.
-- @tparam table callback.programs Paths of found .desktop files.
function utils.parse_dir(dir_path, callback)

    local function get_readable_path(file)
        return file:get_path() or file:get_uri()
    end

    local function parser(file, programs)
        -- Except for "NONE" there is also NOFOLLOW_SYMLINKS
        local query = gio.FILE_ATTRIBUTE_STANDARD_NAME .. "," .. gio.FILE_ATTRIBUTE_STANDARD_TYPE
        local enum, err = file:async_enumerate_children(query, gio.FileQueryInfoFlags.NONE)
        if not enum then
            gdebug.print_warning(get_readable_path(file) .. ": " .. tostring(err))
            return
        end
        local files_per_call = 100 -- Actual value is not that important
        while true do
            local list, enum_err = enum:async_next_files(files_per_call)
            if enum_err then
                gdebug.print_error(get_readable_path(file) .. ": " .. tostring(enum_err))
                return
            end
            for _, info in ipairs(list) do
                local file_type = info:get_file_type()
                local file_child = enum:get_child(info)
                if file_type == 'REGULAR' then
                    local path = file_child:get_path()
                    if path then
                        local success, program = pcall(utils.parse_desktop_file, path)
                        if not success then
                            gdebug.print_error("Error while reading '" .. path .. "': " .. program)
                        elseif program then
                            table.insert(programs, program)
                        end
                    end
                elseif file_type == 'DIRECTORY' then
                    parser(file_child, programs)
                end
            end
            if #list == 0 then
                break
            end
        end
        enum:async_close()
    end

    gio.Async.start(do_protected_call)(function()
        local result = {}
        parser(gio.File.new_for_path(dir_path), result)
        call_callback(callback, result)
    end)
end

function utils.compute_textbox_width(textbox, s)
    gdebug.deprecate("Use 'width, _ = textbox:get_preferred_size(s)' directly.", {deprecated_in=4})
    s = screen[s or mouse.screen]
    local w, _ = textbox:get_preferred_size(s)
    return w
end

--- Compute text width.
-- @tparam str text Text.
-- @tparam number|screen s Screen
-- @treturn int Text width.
function utils.compute_text_width(text, s)
    local w, _ = w_textbox(gstring.xml_escape(text)):get_preferred_size(s)
    return w
end

return utils

-- vim: filetype=lua:expandtab:shiftwidth=4:tabstop=8:softtabstop=4:textwidth=80
