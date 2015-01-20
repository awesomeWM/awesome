---------------------------------------------------------------------------
-- @author Antonio Terceiro
-- @copyright 2009, 2011-2012 Antonio Terceiro, Alexander Yakushev
-- @release @AWESOME_VERSION@
---------------------------------------------------------------------------

-- Grab environment
local io = io
local table = table
local ipairs = ipairs
local string = string
local awful_util = require("awful.util")
local theme = require("beautiful")

-- Utility module for menubar
-- menubar.utils
local utils = {}

-- NOTE: This icons/desktop files module was written according to the
-- following freedesktop.org specifications:
-- Icons: http://standards.freedesktop.org/icon-theme-spec/icon-theme-spec-0.11.html
-- Desktop files: http://standards.freedesktop.org/desktop-entry-spec/desktop-entry-spec-1.0.html

-- Options section

--- Terminal which applications that need terminal would open in.
utils.terminal = 'xterm'

-- The default icon for applications that don't provide any icon in
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

-- List of supported icon formats. Ignore SVG because Awesome doesn't
-- support it.
local icon_formats = { "png", "xpm" }

-- Check whether the icon format is supported.
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

--- Lookup an icon in different folders of the filesystem.
-- @param icon_file Short or full name of the icon.
-- @return full name of the icon.
function utils.lookup_icon(icon_file)
    if not icon_file or icon_file == "" then
        return default_icon
    end

    if icon_file:sub(1, 1) == '/' and is_format_supported(icon_file) then
        -- If the path to the icon is absolute and its format is
        -- supported, do not perform a lookup.
        return icon_file
    else
        local icon_path = {}
        local icon_theme_paths = {}
        local icon_theme = theme.icon_theme
        if icon_theme then
            table.insert(icon_theme_paths, '/usr/share/icons/' .. icon_theme .. '/')
            -- TODO also look in parent icon themes, as in freedesktop.org specification
        end
        table.insert(icon_theme_paths, '/usr/share/icons/hicolor/') -- fallback theme

        for i, icon_theme_directory in ipairs(icon_theme_paths) do
            for j, size in ipairs(all_icon_sizes) do
                table.insert(icon_path, icon_theme_directory .. size .. '/apps/')
                table.insert(icon_path, icon_theme_directory .. size .. '/actions/')
                table.insert(icon_path, icon_theme_directory .. size .. '/devices/')
                table.insert(icon_path, icon_theme_directory .. size .. '/places/')
                table.insert(icon_path, icon_theme_directory .. size .. '/categories/')
                table.insert(icon_path, icon_theme_directory .. size .. '/status/')
            end
        end
        -- lowest priority fallbacks
        table.insert(icon_path, '/usr/share/pixmaps/')
        table.insert(icon_path, '/usr/share/icons/')

        for i, directory in ipairs(icon_path) do
            if is_format_supported(icon_file) and awful_util.file_readable(directory .. icon_file) then
                return directory .. icon_file
            else
                -- Icon is probably specified without path and format,
                -- like 'firefox'. Try to add supported extensions to
                -- it and see if such file exists.
                for _, format in ipairs(icon_formats) do
                    local possible_file = directory .. icon_file .. "." .. format
                    if awful_util.file_readable(possible_file) then
                        return possible_file
                    end
                end
            end
        end
        return default_icon
    end
end

--- Parse a .desktop file.
-- @param file The .desktop file.
-- @return A table with file entries.
function utils.parse(file)
    local program = { show = true, file = file }
    local desktop_entry = false

    -- Parse the .desktop file.
    -- We are interested in [Desktop Entry] group only.
    for line in io.lines(file) do
        if not desktop_entry and line == "[Desktop Entry]" then
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

--- Parse a directory with .desktop files
-- @param dir The directory.
-- @param icons_size, The icons sizes, optional.
-- @return A table with all .desktop entries.
function utils.parse_dir(dir)
    local programs = {}
    local files = io.popen('find '.. dir ..' -maxdepth 1 -name "*.desktop" 2>/dev/null')
    for file in files:lines() do
        local program = utils.parse(file)
        if program then
            table.insert(programs, program)
        end
    end
    return programs
end

return utils

-- vim: filetype=lua:expandtab:shiftwidth=4:tabstop=8:softtabstop=4:textwidth=80
