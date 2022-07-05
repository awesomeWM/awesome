#!/usr/bin/env lua
-- This script locate and post process all "dangling" files CMake
-- didn't track. This allows the `tests/examples/` file to save more
-- than one output artifact.

pcall(require, "luarocks.loader")

local gio = require("lgi").Gio

local name_attr = gio.FILE_ATTRIBUTE_STANDARD_NAME
local type_attr = gio.FILE_ATTRIBUTE_STANDARD_TYPE

local raw_path, processed_path, script_path = ...

local function list_svg(path)
    local ret = {}

    local enumerator = gio.File.new_for_path(path):enumerate_children(
        table.concat({name_attr, type_attr}, ",") , 0, nil, nil
    )

    for file in function() return enumerator:next_file() end do
        local file_name = file:get_attribute_as_string(name_attr)
        local file_type = file:get_file_type()
        local match_ext = file_name:match("[.]svg$")

        if file_type == "REGULAR" and match_ext then
            ret[file_name] = true
        end
    end

    return ret
end

local raw_files, processed_files = list_svg(raw_path), list_svg(processed_path)

for file in pairs(raw_files) do
    if not processed_files[file] then
        os.execute(table.concat({
            script_path,
            raw_path .. "/" .. file,
            processed_path .. "/" .. file
        }, " "))
    end
end
