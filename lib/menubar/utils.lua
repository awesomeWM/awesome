---------------------------------------------------------------------------
--- Utility module for menubar
--
-- @author Antonio Terceiro
-- @copyright 2009, 2011-2012 Antonio Terceiro, Alexander Yakushev
-- @module menubar.utils
---------------------------------------------------------------------------

-- Grab environment
local io = io
local table = table
local ipairs = ipairs
local string = string
local screen = screen
local gfs = require("gears.filesystem")
local theme = require("beautiful")
local lgi = require("lgi")
local gio = lgi.Gio
local glib = lgi.GLib
local wibox = require("wibox")
local gdebug = require("gears.debug")
local protected_call = require("gears.protected_call")
local gstring = require("gears.string")

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

--- Name of the WM for the OnlyShownIn entry in the .desktop file.
utils.wm_name = "awesome"

-- Private section

local all_icon_sizes = {
    '128x128' ,
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
local icon_formats = { "png", "xpm", "svg" }

--- Check whether the icon format is supported.
-- @param icon_file Filename of the icon.
-- @return true if format is supported, false otherwise.
local function is_format_supported(icon_file)
    for _, f in ipairs(icon_formats) do
        if icon_file:match('%.' .. f) then
            return true
        end
    end
    return false
end

local icon_lookup_path = nil
--- Get a list of icon lookup paths.
-- @treturn table A list of directories, without trailing slash.
local function get_icon_lookup_path()
    if not icon_lookup_path then
        local add_if_readable = function(t, path)
            if gfs.dir_readable(path) then
                table.insert(t, path)
            end
        end
        icon_lookup_path = {}
        local icon_theme_paths = {}
        local icon_theme = theme.icon_theme
        local paths = glib.get_system_data_dirs()
        table.insert(paths, 1, glib.get_user_data_dir())
        table.insert(paths, 1, glib.build_filenamev({glib.get_home_dir(),
                                                     '.icons'}))
        for _,dir in ipairs(paths) do
            local icons_dir = glib.build_filenamev({dir, 'icons'})
            if gfs.dir_readable(icons_dir) then
                if icon_theme then
                    add_if_readable(icon_theme_paths,
                                    glib.build_filenamev({icons_dir,
                                                         icon_theme}))
                end
                -- Fallback theme.
                add_if_readable(icon_theme_paths,
                                glib.build_filenamev({icons_dir, 'hicolor'}))
            end
        end
        for _, icon_theme_directory in ipairs(icon_theme_paths) do
            for _, size in ipairs(all_icon_sizes) do
                add_if_readable(icon_lookup_path,
                                glib.build_filenamev({icon_theme_directory,
                                                      size, 'apps'}))
            end
        end
        for _,dir in ipairs(paths)do
            -- lowest priority fallbacks
            add_if_readable(icon_lookup_path,
                            glib.build_filenamev({dir, 'pixmaps'}))
            add_if_readable(icon_lookup_path,
                            glib.build_filenamev({dir, 'icons'}))
        end
    end
    return icon_lookup_path
end

--- Lookup an icon in different folders of the filesystem.
-- @tparam string icon_file Short or full name of the icon.
-- @treturn string|boolean Full name of the icon, or false on failure.
function utils.lookup_icon_uncached(icon_file)
    if not icon_file or icon_file == "" then
        return false
    end

    if icon_file:sub(1, 1) == '/' and is_format_supported(icon_file) then
        -- If the path to the icon is absolute and its format is
        -- supported, do not perform a lookup.
        return gfs.file_readable(icon_file) and icon_file or nil
    else
        for _, directory in ipairs(get_icon_lookup_path()) do
            if is_format_supported(icon_file) and
                    gfs.file_readable(directory .. "/" .. icon_file) then
                return directory .. "/" .. icon_file
            else
                -- Icon is probably specified without path and format,
                -- like 'firefox'. Try to add supported extensions to
                -- it and see if such file exists.
                for _, format in ipairs(icon_formats) do
                    local possible_file = directory .. "/" .. icon_file .. "." .. format
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
    local desktop_entry = false

    -- Parse the .desktop file.
    -- We are interested in [Desktop Entry] group only.
    for line in io.lines(file) do
        if line:find("^%s*#") then
            -- Skip comments.
            (function() end)() -- I haven't found a nice way to silence luacheck here
        elseif not desktop_entry and line == "[Desktop Entry]" then
            desktop_entry = true
        else
            if line:sub(1, 1) == "[" and line:sub(-1) == "]" then
                -- A declaration of new group - stop parsing
                break
            end

            -- Grab the values
            for key, value in line:gmatch("(%w+)%s*=%s*(.+)") do
                program[key] = value
            end
        end
    end

    -- In case [Desktop Entry] was not found
    if not desktop_entry then return nil end

    -- In case the (required) 'Name' entry was not found
    if not program.Name or program.Name == '' then return nil end

    -- Don't show program if NoDisplay attribute is false
    if program.NoDisplay and string.lower(program.NoDisplay) == "true" then
        program.show = false
    end

    -- Only show the program if there is no OnlyShowIn attribute
    -- or if it's equal to utils.wm_name
    if program.OnlyShowIn ~= nil and not program.OnlyShowIn:match(utils.wm_name) then
        program.show = false
    end

    -- Look up for a icon.
    if program.Icon then
        program.icon_path = utils.lookup_icon(program.Icon)
    end

    -- Split categories into a table. Categories are written in one
    -- line separated by semicolon.
    if program.Categories then
        program.categories = {}
        for category in program.Categories:gmatch('[^;]+') do
            table.insert(program.categories, category)
        end
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
        if program.Terminal == "true" then
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

    local function parser(dir, programs)
        local f = gio.File.new_for_path(dir)
        -- Except for "NONE" there is also NOFOLLOW_SYMLINKS
        local query = gio.FILE_ATTRIBUTE_STANDARD_NAME .. "," .. gio.FILE_ATTRIBUTE_STANDARD_TYPE
        local enum, err = f:async_enumerate_children(query, gio.FileQueryInfoFlags.NONE)
        if not enum then
            gdebug.print_error(err)
            return
        end
        local files_per_call = 100 -- Actual value is not that important
        while true do
            local list, enum_err = enum:async_next_files(files_per_call)
            if enum_err then
                gdebug.print_error(enum_err)
                return
            end
            for _, info in ipairs(list) do
                local file_type = info:get_file_type()
                local file_path = enum:get_child(info):get_path()
                if file_type == 'REGULAR' then
                    local program = utils.parse_desktop_file(file_path)
                    if program then
                        table.insert(programs, program)
                    end
                elseif file_type == 'DIRECTORY' then
                    parser(file_path, programs)
                end
            end
            if #list == 0 then
                break
            end
        end
        enum:async_close()
    end

    gio.Async.start(protected_call.call)(function()
        local result = {}
        parser(dir_path, result)
        callback(result)
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
    local w, _ = wibox.widget.textbox(gstring.xml_escape(text)):get_preferred_size(s)
    return w
end

return utils

-- vim: filetype=lua:expandtab:shiftwidth=4:tabstop=8:softtabstop=4:textwidth=80
