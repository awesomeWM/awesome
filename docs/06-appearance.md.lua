#! /usr/bin/lua
local args = {...}

local gio = require("lgi").Gio
local gobject = require("lgi").GObject
local glib = require("lgi").GLib

local name_attr = gio.FILE_ATTRIBUTE_STANDARD_NAME
local type_attr = gio.FILE_ATTRIBUTE_STANDARD_TYPE

-- Like pairs(), but iterate over keys in a sorted manner. Does not support
-- modifying the table while iterating.
local function sorted_pairs(t)
    -- Collect all keys
    local keys = {}
    for k in pairs(t) do
        table.insert(keys, k)
    end

    table.sort(keys)

    -- return iterator function
    local i = 0
    return function()
        i = i + 1
        if keys[i] then
            return keys[i], t[keys[i]]
        end
    end
end

-- Recursive file scanner
local function get_all_files(path, ext, ret)
    ret = ret or {}
    local enumerator = gio.File.new_for_path(path):enumerate_children(
        table.concat({name_attr, type_attr}, ",") , 0, nil, nil
    )

    for file in function() return enumerator:next_file() end do
        local file_name = file:get_attribute_as_string(name_attr)
        local file_type = file:get_file_type()
        if file_type == "REGULAR" and file_name:match(ext or "") then
            table.insert(ret, enumerator:get_child(file):get_path())
        elseif file_type == "DIRECTORY" then
            get_all_files(enumerator:get_child(file):get_path(), ext, ret)
        end
    end

    return ret
end

local function path_to_module(path)
    for _, module in ipairs {
        "awful", "wibox", "gears", "naughty", "menubar", "beautiful"
    } do
        local match = path:match("/"..module.."/([^.]+).lua")
        if match then
            return module.."."..match:gsub("/",".")
        end
    end

    error("Cannot figure out module for " .. tostring(path))
end

local function path_to_html(path)
    local mod = path_to_module(path):gsub(".init", "")
    local f = assert(io.open(path))
    while true do
        local line = f:read()
        if not line then break end
        if line:match("@classmod") then
            f:close()
            return "../classes/".. mod ..".html"
        end
        if line:match("@module") or line:match("@submodule") then
            f:close()
            return "../libraries/".. mod ..".html"
        end
    end
    f:close()

    error("Cannot figure out if module or class: " .. tostring(path))
end

local function get_link(file, element)
    return table.concat {
        "<a href='",
        path_to_html(file),
        "#",
        element,
        "'>",
        element:match("[. ](.+)"),
        "</a>"
    }
end

local all_files = get_all_files("./lib/", "lua")

local beautiful_vars = {}

-- Find all @beautiful doc entries
for _,file in ipairs(all_files) do
    local f = io.open(file)

    local buffer = ""

    for line in f:lines() do

        local var = line:gmatch("--[ ]*@beautiful ([^ \n]*)")()

        -- There is no backward/forward pattern in lua
        if #line <= 1 then
            buffer = ""
        elseif #buffer and not var then
            buffer = buffer.."\n"..line
        elseif line:sub(1,3) == "---" then
            buffer = line
        end


        if var then
            -- Get the @param, @see and @usage
            local params = ""
            for line in f:lines() do
                if line:sub(1,2) ~= "--" then
                    break
                else
                    params = params.."\n"..line
                end
            end

            local name = var:gmatch("[ ]*beautiful.(.+)")()
            if not name then
                print("WARNING:", var,
                    "seems to be misformatted. Use `beautiful.namespace_name`"
                )
            else
                table.insert(beautiful_vars, {
                    file = file,
                    name = name,
                    link = get_link(file, var),
                    desc = buffer:gmatch("[- ]+([^\n.]*)")() or "",
                    mod  = path_to_module(file),
                })
            end

            buffer = ""
        end
    end
end

local function create_table(entries, columns)
    local lines = {}

    for _, entry in ipairs(entries) do
        local line = "  <tr>"

        for _, column in ipairs(columns) do
            line = line.."<td>"..entry[column].."</td>"
        end

        table.insert(lines, line.."</tr>\n")
    end

    return [[<br \><br \><table class='widget_list' border=1>
 <tr style='font-weight: bold;'>
  <th align='center'>Name</th>
  <th align='center'>Description</th>
 </tr>]] .. table.concat(lines) .. "</table>\n"
end

local override_cats = {
    ["border"  ] = true,
    ["bg"      ] = true,
    ["fg"      ] = true,
    ["useless" ] = true,
    [""        ] = true,
}

local function categorize(entries)
    local ret = {}

    local cats = {
        ["Default variables"] = {}
    }

    for _, v in ipairs(entries) do
        local ns = v.name:match("([^_]+)_") or ""
        ns = override_cats[ns] and "Default variables" or ns
        cats[ns] = cats[ns] or {}
        table.insert(cats[ns], v)
    end

    return cats
end

local function create_sample(entries)
    local ret = {
        "    local theme = {}"
    }

    for name, cat in sorted_pairs(categorize(entries)) do
        table.insert(ret, "\n    -- "..name)
        for _, v in ipairs(cat) do
            table.insert(ret, "    -- theme."..v.name.." = nil")
        end
    end

    table.insert(ret, [[

    return theme

    -- vim: filetype=lua:expandtab:shiftwidth=4:tabstop=8:softtabstop=4:textwidth=80]]
    )

    return table.concat(ret, '\n')
end

-- Create the file
local filename = args[1]

local f = io.open(filename, "w")

f:write[[
# Change Awesome appearance

## The beautiful themes

Beautiful is where Awesome theme variables are stored.

]]
f:write(create_table(beautiful_vars, {"link", "desc"}))

f:write("\n\n## Sample theme file\n\n")

f:write(create_sample(beautiful_vars, {"link", "desc"}))

f:close()

--TODO add some linting to direct undeclared beautiful variables
--TODO re-generate all official themes
--TODO generate a complete sample theme
--TODO also parse C files.

-- vim: filetype=lua:expandtab:shiftwidth=4:tabstop=8:softtabstop=4:textwidth=80
