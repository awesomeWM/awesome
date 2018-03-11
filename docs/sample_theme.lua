--- Take the default theme and add it to ldoc as a sample file.
local output_path = ...

local path = debug.getinfo(1,"S").source:gsub("sample_theme.*",""):gsub("@","")
package.path = path .. '?.lua;' .. package.path

require("sample_files")(
    "theme.lua",
    path.."../themes/default/theme.lua",
    output_path.."/theme.lua",
[[---------------------------------------------------------------------------
--- The default theme file.
--
-- If you wish to create a custom theme, copy this file to
-- `~/.config/awesome/mytheme.lua`
-- and replace:
--
--    beautiful.init(gears.filesystem.get_themes_dir() .. "default/theme.lua")
--
-- with
--
--    beautiful.init(gears.filesystem.get_configuration_dir() .. "mytheme.lua")
--
-- in your `rc.lua`.
--
-- Here is the default theme content:
--
--]]
)
