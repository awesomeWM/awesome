#!/usr/bin/lua
-- Generate documentation for awesome Lua functions
-- Take a .c file in stdin

function string.comment_clean(str)
    local s = str:gsub("/%*%* ", "    ")
    s = s:gsub(" %* ", " ")
    s = s:gsub("\\param", "\n\n    Parameter:")
    s = s:gsub("\\return", "\n\n    Return:")
    return s
end

-- Read all the files in lines
lines = io.read("*all")

ilines = {}

-- read the lines in table `ilines'
for line in lines:gmatch("[^\r\n]+") do
    table.insert(ilines, line)
end

-- Store function documentations in an array
function_doc = {}
for i, line in ipairs(ilines) do
    if line:find("^/%*%*") then
        comment_start = true
        comment = line
    elseif line:find("%*/") then
        comment_start = false
        local l = ilines[i + 2]
        local fctname
        _, _, fctname = l:find("^(.+)%(lua_State")
        if fctname then
            function_doc[fctname] = comment
        end
        comment = nil
    elseif comment_start then
        comment = comment .. line
    end
end

-- Get function list and print their documentation
capture = false
for i, line in ipairs(ilines) do
    if not libname then
        _, _, libname, libtype = line:find("const struct luaL_reg awesome_(%a+)_(%a+)%[%] =")
        -- Special case
        if not libname then _, _, libname, libtype = line:find("const struct luaL_reg (awesome)_(lib)%[%] =") end
    else
        if line:find("};") then
            libname = nil
        else
            local fctname, fctdef
            _, _, fctname, fctdef = line:find("\"(.+)\", (.+) },")
            if fctname and not fctname:find("^__") then
                if libtype == "meta" then sep = ":" else sep = "." end
                print("*" .. libname .. sep .. fctname ..  "*::")
                if function_doc[fctdef] then
                    print(function_doc[fctdef]:comment_clean())
                else
                    print("This function is not yet documented.")
                end
                print()
            end
        end
    end
end
