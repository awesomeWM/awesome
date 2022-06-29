---------------------------------------------------------------------------
--- Define easily new key and button objects.
--
-- Built on top awful.key and awful.button and also aims
-- at decluttering your config.
--
-- Use awful.EzKey to define a keybinding
--
-- This example shows how to define a basic EzKey object:
--
-- awful.EzKey("C-M-s",function() awful.util.spawn("brave") end,"some description","some group name")
--
-- This example shows how to define a basic EzButton object:
--
-- awful.EzButton("M-1",function() .... end)
--
-- @author neon_arch neon_arch@gitlab.com;
-- @copyright 2022 neon_arch
-- @inputmodule awful.Ezkey
---------------------------------------------------------------------------


-------------imports---------------------
local key = require("key");
local easykey = {};

---------------------Ezkey---------------------


--#region
-- A function to handle spliting of key group like "C-M-s" into
-- a table of keys something like this {"C","M","s"}
-- #endregion
local function split(inputstr, sep)
    if sep == nil then
        sep = "%s"
    end
    local split_string = {}
    for str in string.gmatch(inputstr, "([^" .. sep .. "]+)") do
        table.insert(split_string, str)
    end
    return split_string
end

--#region
--function to check whether the last key is a number or not
--if it is then convert it into a number and return it
--#endregion
local function check_if_number(key)
    if (key == "1") then
        return 1
    elseif (key == "2") then
        return 2
    elseif (key == "3") then
        return 3
    elseif (key == "4") then
        return 4
    elseif (key == "5") then
        return 5
    elseif (key == "6") then
        return 6
    elseif (key == "7") then
        return 7
    elseif (key == "8") then
        return 8
    elseif (key == "9") then
        return 9
    elseif (key == "0") then
        return 0
    else
        return key
    end
end

--#region
--modifier key table
--#endregion
easykey.modifier_keys = {
    C = "Control",
    M = "Mod4",
    A = "Mod1",
    S = "Shift",
}

--#region
--internal function to convert modifier keys to their actual key names
--and put it as a table and also to retrieve the last value and return both of
--them seperately
--for example
-- if this is passed in "C-M-s"
--
-- then it returns
--
-- {"Control", "Mod4"} and "s"
--#endregion
local function Ezkey_to_key_convertor(keychord)
    local splited_string = split(keychord, "-");
    local last = check_if_number(splited_string[#splited_string]);
    table.remove(splited_string, #splited_string);
    print(tostring(splited_string[3]), last)
    local key_table = {};
    for i in ipairs(splited_string) do
        table.insert(key_table, easykey.modifier_keys[ splited_string[i] ])
    end
    return key_table, last, 3
end

--#region
--function built on top of awful.keys and takes the values from
--the internal function Ezkey_to_key_convertor() and plugs it into
--awful.keys
--#endregion
function easykey.EzKey(keychord, callback, desc, grp)
    local keyTable, last = Ezkey_to_key_convertor(keychord)
    return awful.key(keyTable, last, callback,
        { description = desc, group = grp });
end

--#region
--function built on top of awful.button and takes the values from
--the internal function Ezkey_to_key_convertor() and plugs it into
--awful.button
--#endregion
function easykey.EzButton(keychord, callback)
    local keyTable, last = Ezkey_to_key_convertor(keychord)
    return awful.button(keyTable, last, callback)
end

return easykey
