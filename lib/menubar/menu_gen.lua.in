---------------------------------------------------------------------------
-- @author Antonio Terceiro
-- @copyright 2009, 2011-2012 Antonio Terceiro, Alexander Yakushev
-- @release @AWESOME_VERSION@
---------------------------------------------------------------------------

-- Grab environment
local utils = require("menubar.utils")
local pairs = pairs
local ipairs = ipairs
local string = string
local table = table

-- Menu generation module for menubar
-- menubar.menu_gen
local menu_gen = {}

-- Options section

local data_dir = os.getenv("XDG_DATA_HOME")
if not data_dir then
    data_dir = os.getenv("HOME") .. '/.local/share/'
end

--- Specifies all directories where menubar should look for .desktop
-- files. The search is not recursive.
menu_gen.all_menu_dirs = { data_dir .. 'applications/', '/usr/share/applications/', '/usr/local/share/applications/' }

--- Specify the mapping of .desktop Categories section to the
-- categories in the menubar. If "use" flag is set to false then any of
-- the applications that fall only to this category will not be shown.
menu_gen.all_categories = {
    multimedia = { app_type = "AudioVideo", name = "Multimedia",
                     icon_name = "applications-multimedia.png", use = true },
    development = { app_type = "Development", name = "Development",
                    icon_name = "applications-development.png", use = true },
    education = { app_type = "Education", name = "Education",
                  icon_name = "applications-science.png", use = true },
    games = { app_type = "Game", name = "Games",
              icon_name = "applications-games.png", use = true },
    graphics = { app_type = "Graphics", name = "Graphics",
                 icon_name = "applications-graphics.png", use = true },
    office = { app_type = "Office", name = "Office",
               icon_name = "applications-office.png", use = true },
    internet = { app_type = "Network", name = "Internet",
                icon_name = "applications-internet.png", use = true },
    settings = { app_type = "Settings", name = "Settings",
                 icon_name = "applications-utilities.png", use = true },
    tools = { app_type = "System", name = "System Tools",
               icon_name = "applications-system.png", use = true },
    utility = { app_type = "Utility", name = "Accessories",
                icon_name = "applications-accessories.png", use = true }
}

--- Find icons for category entries.
function menu_gen.lookup_category_icons()
    for _, v in pairs(menu_gen.all_categories) do
        v.icon = utils.lookup_icon(v.icon_name)
    end
end

-- Get category key name and whether it is used by its app_type.
-- @param app_type Application category as written in .desktop file.
-- @return category key name in all_categories, whether the category is used
local function get_category_name_and_usage_by_type(app_type)
    for k, v in pairs(menu_gen.all_categories) do
        if app_type == v.app_type then
            return k, v.use
        end
    end
end

-- Remove CR\LF newline from the end of the string.
-- @param s string to trim
local function trim(s)
    if not s then return end
    if string.byte(s, #s) == 13 then
        return string.sub(s, 1, #s - 1)
    end
    return s
end

--- Generate an array of all visible menu entries.
-- @return all menu entries.
function menu_gen.generate()
    -- Update icons for category entries
    menu_gen.lookup_category_icons()

    local result = {}

    for _, dir in ipairs(menu_gen.all_menu_dirs) do
        local entries = utils.parse_dir(dir)
        for _, program in ipairs(entries) do
            -- Check whether to include program in the menu
            if program.show and program.Name and program.cmdline then
                local target_category = nil
                -- Check if the program falls at least to one of the
                -- usable categories. Set target_category to be the id
                -- of the first category it finds.
                if program.categories then
                    for _, category in pairs(program.categories) do
                        local cat_key, cat_use =
                            get_category_name_and_usage_by_type(category)
                        if cat_key and cat_use then
                            target_category = cat_key
                            break
                        end
                    end
                end
                if target_category then
                    local name = trim(program.Name) or ""
                    local cmdline = trim(program.cmdline) or ""
                    local icon = utils.lookup_icon(trim(program.icon_path)) or nil
                    table.insert(result, { name = name,
                                           cmdline = cmdline,
                                           icon = icon,
                                           category = target_category })
                end
            end
        end
    end
    return result
end

return menu_gen

-- vim: filetype=lua:expandtab:shiftwidth=4:tabstop=8:softtabstop=4:textwidth=80
