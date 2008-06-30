#!/usr/bin/lua
-- Translate the custom doxygen tags in c-source to a 
-- dummy lua source file that can be processed by luadoc.
-- Take a .c file in stdin

nparam = 0;
function string.replace_param(s)
    nparam = nparam + 1;
    return "@param arg" .. nparam
end

function string.comment_translate(s)
	local lua_comment = "";
        nparam = 0;
	for line in s:gmatch("[^\r\n]+") do
		line = line:gsub("/%*%*", "---")
		line = line:gsub("^.*%*", "--")
		line = line:gsub("\\lvalue", "")
		line = line:gsub("\\(lparam)", string.replace_param)
		line = line:gsub("\\lreturn", "@return")
		lua_comment = lua_comment .. line .. "\n"
	end
    return lua_comment
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
        if l then
            local fctname
            _, _, fctname = l:find("^(.+)%(lua_State")
            if fctname then
                function_doc[fctname] = comment
            end
        end
        comment = nil
    elseif comment_start then
        if not line:find("\\param") and not line:find("\\return") and not line:find("\\luastack") then
            comment = comment .. "\n" .. line
        end
    end
end

-- Get function list and print their documentation
capture = false
for i, line in ipairs(ilines) do
    if not libname then
        _, _, libname, libtype = line:find("const struct luaL_reg awesome_(%a+)_(%a+)%[%] ")
        -- Special case
        if not libname then _, _, libname, libtype = line:find("const struct luaL_reg (awesome)_(lib)%[%] =") end
    else
        if line:find("};") then
            libname = nil
        else
            local fctname, fctdef
            _, _, fctname, fctdef = line:find("\"(.+)\", (.+) },?")
            if fctname and (not fctname:find("^__") or fctname:find("^__call")) then
                if function_doc[fctdef] then
                    fctname = "." .. fctname
                    fctname = fctname:gsub("^.__call", "")
                    print(function_doc[fctdef]:comment_translate())
                    print("function " .. libname .. fctname .. "()")
                    print("end");
                else
                    print("This function is not yet documented.")
                end
                print()
            end
        end
    end
end
