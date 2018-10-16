#! /usr/bin/lua
local args = {...}
local parser = require("docs._parser")

local gio = require("lgi").Gio
local gobject = require("lgi").GObject
local glib = require("lgi").GLib

local paths = parser.get_all_files("./lib/", "lua", parser.get_all_files("./", "c"))

local beautiful_vars = parser.parse_files(paths, "beautiful")

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

    for name, cat in parser.sorted_pairs(categorize(entries)) do
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

<br /><br />
]]
f:write(parser.create_table(beautiful_vars, {"link", "desc"}))

f:write("\n\n## Sample theme file\n\n")

f:write(create_sample(beautiful_vars, {"link", "desc"}))

f:close()

--TODO add some linting to direct undeclared beautiful variables
--TODO re-generate all official themes
--TODO generate a complete sample theme

-- vim: filetype=lua:expandtab:shiftwidth=4:tabstop=8:softtabstop=4:textwidth=80
