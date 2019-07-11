local gio = require("lgi").Gio
local gobject = require("lgi").GObject
local glib = require("lgi").GLib

local name_attr = gio.FILE_ATTRIBUTE_STANDARD_NAME
local type_attr = gio.FILE_ATTRIBUTE_STANDARD_TYPE

local module = {}

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
        local match_ext = file_name:match("[.]"..ext.."$" or "")
        local fpath     = enumerator:get_child(file):get_path()
        local is_test   = fpath:match("/tests/")
        if file_type == "REGULAR" and match_ext and not is_test then
            table.insert(ret, fpath)
        elseif file_type == "DIRECTORY" then
            get_all_files(enumerator:get_child(file):get_path(), ext, ret)
        end
    end

    return ret
end

local function path_to_module(path)
    if path:match("[.]c$") then
        return path:gmatch("/([^./]+)[.]c$")()
    end

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

local modtypes = {
    classmod     = "classes",
    widgetmod    = "widgets",
    containermod = "widget_containers",
    layoutmod    = "widget_layouts",
    coreclassmod = "core_components",
    popupmod     = "popups_and_bars",
    module       = "libraries",
    submodule    = "libraries",
    utillib      = "utility_libraries",
    themelib     = "theme_related_libraries",
}

local libtypes = {
}

function module.path_to_html(path)
    local mod = path_to_module(path):gsub(".init", "")
    local f = assert(io.open(path))
    while true do
        local line = f:read()
        if not line then break end

        local tag = line:gmatch("@([^ ]+) ")() or ""

        if modtypes[tag] then
            f:close()
            return "../"..modtypes[tag].."/".. mod ..".html#"
        end
    end
    f:close()

    error("Cannot figure out if module or class: " .. tostring(path))
end

local function get_link(file, element, element_name)
    return table.concat {
        "<a href='",
        module.path_to_html(file),
        element,
        "'>",
        element_name,
        "</a>"
    }
end

local function parse_files(paths, property_name, matcher, name_matcher)
    local exp1 = "[-*]*[ ]*@".. property_name .." ([^ \n]*)"
    local exp2 = matcher or "[-*]*[ ]*".. property_name ..".(.+)"
    local exp3 = name_matcher or "[. ](.+)"

    local ret = {}

    table.sort(paths)

    -- Find all @beautiful doc entries
    for _,file in ipairs(paths) do
        local f = io.open(file)

        local buffer = ""

        for line in f:lines() do

            local var = line:gmatch(exp1)()

            -- There is no backward/forward pattern in lua
            if #line <= 1 then
                buffer = ""
            elseif #buffer and not var then
                buffer = buffer.."\n"..line
            elseif line:sub(1,3) == "---" or line:sub(1,3) == "/**" then
                buffer = line
            end

            if var then
                -- Get the @param, @see and @usage
                local params = ""
                for line in f:lines() do
                    if line:sub(1,2) ~= "--" and line:sub(1,2) ~= " *" then
                        break
                    else
                        params = params.."\n"..line
                    end
                end

                local name = var:gmatch(exp2)()
                if not name then
                    print("WARNING:", var,
                        "seems to be misformatted. Use `beautiful.namespace_name`"
                    )
                else
                    table.insert(ret, {
                        file = file,
                        name = name:gsub("_", "\\_"),
                        link = get_link(file, var, var:match(exp3):gsub("_", "\\_")),
                        desc = buffer:gmatch("[-*/ \n]+([^\n.]*)")() or "",
                        mod  = path_to_module(file),
                    })
                end

                buffer = ""
            end
        end
    end

    return ret
end

local function create_table(entries, columns, prefix)
    prefix = prefix or ""
    local lines = {}

    for _, entry in ipairs(entries) do
        local line = "  <tr>"

        for _, column in ipairs(columns) do
            line = line.."<td>"..entry[column].."</td>"
        end

        table.insert(lines, prefix..line.."</tr>\n")
    end

    return [[--<table class='widget_list' border=1>
]]..prefix..[[<tr>
]]..prefix..[[ <th align='center'>Name</th>
]]..prefix..[[ <th align='center'>Description</th>
]]..prefix..[[</tr>
]] .. table.concat(lines) .. prefix .."</table>\n"
end

module.create_table  = create_table
module.parse_files   = parse_files
module.sorted_pairs  = sorted_pairs
module.get_all_files = get_all_files

return module
