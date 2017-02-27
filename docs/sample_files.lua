-- Take a Lua file and add it to the documentation sample files.
-- Also add \` to generate all links

-- Tell ldoc to generate links
    local function add_links(line)
        for _, module in ipairs {
            "awful", "wibox", "gears", "naughty", "menubar", "beautiful"
        } do
            if line:match(module.."%.") then
                line = line:gsub("("..module.."[.a-zA-Z]+)", "`%1`")
            end
        end

        return "--    "..line
    end

return function(name, input_path, output_path, header)
    local input = assert(io.open(input_path))
    local output_script = {header}

    -- Escape all lines
    for line in input:lines() do
        table.insert(output_script, add_links(line))
    end

    -- Add the script name
    table.insert(output_script, "-- @script "..name)

    output_path = assert(io.open(output_path, "w"))
    output_path:write(table.concat(output_script, "\n"))
    output_path:close()
end
