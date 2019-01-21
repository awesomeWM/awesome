---------------------------------------------------------------------------
--- Menu generation module for menubar
--
-- @author Antonio Terceiro
-- @copyright 2009, 2011-2012 Antonio Terceiro, Alexander Yakushev
-- @module menubar.menu_gen
---------------------------------------------------------------------------

-- Grab environment
local gtable = require("gears.table")
local gfilesystem = require("gears.filesystem")
local utils = require("menubar.utils")
local pairs = pairs
local ipairs = ipairs
local table = table

local menu_gen = {}

-- Options section

--- Get the path to the directories where XDG menu applications are installed.
local function get_xdg_menu_dirs()
    local dirs = gfilesystem.get_xdg_data_dirs()
    table.insert(dirs, 1, gfilesystem.get_xdg_data_home())
    return gtable.map(function(dir) return dir .. 'applications/' end, dirs)
end

--- Specifies all directories where menubar should look for .desktop
-- files. The search is recursive.
menu_gen.all_menu_dirs = get_xdg_menu_dirs()

--- Specify the mapping of .desktop Categories section to the
-- categories in the menubar. If "use" flag is set to false then any of
-- the applications that fall only to this category will not be shown.
menu_gen.all_categories = {
    multimedia = { app_type = "AudioVideo", name = "Multimedia",
                     icon_name = "applications-multimedia", use = true },
    development = { app_type = "Development", name = "Development",
                    icon_name = "applications-development", use = true },
    education = { app_type = "Education", name = "Education",
                  icon_name = "applications-science", use = true },
    games = { app_type = "Game", name = "Games",
              icon_name = "applications-games", use = true },
    graphics = { app_type = "Graphics", name = "Graphics",
                 icon_name = "applications-graphics", use = true },
    office = { app_type = "Office", name = "Office",
               icon_name = "applications-office", use = true },
    internet = { app_type = "Network", name = "Internet",
                icon_name = "applications-internet", use = true },
    science = { app_type = "Science", name="Science",
                icon_name = "applications-science", use = true },
    settings = { app_type = "Settings", name = "Settings",
                 icon_name = "applications-utilities", use = true },
    tools = { app_type = "System", name = "System Tools",
               icon_name = "applications-system", use = true },
    utility = { app_type = "Utility", name = "Accessories",
                icon_name = "applications-accessories", use = true }
}

--- Find icons for category entries.
function menu_gen.lookup_category_icons()
    for _, v in pairs(menu_gen.all_categories) do
        v.icon = utils.lookup_icon(v.icon_name)
    end
end

--- Get category key name and whether it is used by its app_type.
-- @param app_type Application category as written in .desktop file.
-- @return category key name in all_categories, whether the category is used
local function get_category_name_and_usage_by_type(app_type)
    for k, v in pairs(menu_gen.all_categories) do
        if app_type == v.app_type then
            return k, v.use
        end
    end
end

--- Generate an array of all visible menu entries.
-- @tparam function callback Will be fired when all menu entries were parsed
-- with the resulting list of menu entries as argument.
-- @tparam table callback.entries All menu entries.
function menu_gen.generate(callback)
    -- Update icons for category entries
    menu_gen.lookup_category_icons()

    local result = {}
    local unique_entries = {}
    local dirs_parsed = 0

    for _, dir in ipairs(menu_gen.all_menu_dirs) do
        utils.parse_dir(dir, function(entries)
            entries = entries or {}
            for _, entry in ipairs(entries) do
                -- Check whether to include program in the menu
                if entry.show and entry.Name and entry.cmdline then
                    local unique_key = entry.Name .. '\0' .. entry.cmdline
                    if not unique_entries[unique_key] then
                        local target_category = nil
                        -- Check if the program falls into at least one of the
                        -- usable categories. Set target_category to be the id
                        -- of the first category it finds.
                        if entry.categories then
                            for _, category in pairs(entry.categories) do
                                local cat_key, cat_use =
                                get_category_name_and_usage_by_type(category)
                                if cat_key and cat_use then
                                    target_category = cat_key
                                    break
                                end
                            end
                        end

                        local name = utils.rtrim(entry.Name) or ""
                        local cmdline = utils.rtrim(entry.cmdline) or ""
                        local icon = entry.icon_path or nil
                        table.insert(result, { name = name,
                                     cmdline = cmdline,
                                     icon = icon,
                                     category = target_category })
                        unique_entries[unique_key] = true
                    end
                end
            end
            dirs_parsed = dirs_parsed + 1
            if dirs_parsed == #menu_gen.all_menu_dirs then
                callback(result)
            end
        end)
    end
end

return menu_gen

-- vim: filetype=lua:expandtab:shiftwidth=4:tabstop=8:softtabstop=4:textwidth=80
