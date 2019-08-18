#! /usr/bin/lua
local args = {...}
local typename = args[2]
local parser = require("docs._parser")

assert(typename)

local files = {
    "./objects/"..typename..".c",
    "./lib/awful/"..typename..".lua",
    "./lib/naughty/"..typename..".lua"
}

local matcher, matcher2 = "(.*)", ".*"

-- The client function comes from 5 different files, but all of those are
-- merged into one documentation page (aka, awful.client doesn't have content
-- anymore). This override the path so the parser doesn't have to be aware of it
function parser.path_to_html()
    return "../core_components/"..typename..".html#"
end

local clientruleproperty = parser.parse_files(files, "clientruleproperty", matcher, matcher2)

for _, prop in ipairs(parser.parse_files(files, "property", matcher, matcher2)) do
    table.insert(clientruleproperty, prop)
end

-- Create the file
local filename = args[1]

local f = io.open(filename, "w")

f:write(parser.create_table(clientruleproperty, {"link", "desc"}, "-- "))

f:close()

-- vim: filetype=lua:expandtab:shiftwidth=4:tabstop=8:softtabstop=4:textwidth=80
