---------------------------------------------------------------------------
--- Functions for reading icon-theme.cache files
-- @author Uli Schlachter
-- @copyright 2017 Uli Schlachter
-- @classmod gears.icon_theme_cache
---------------------------------------------------------------------------

local Gio = require("lgi").Gio -- Only needed to get mtime

local theme = {}
local theme_mt = {
    __index = theme,
    __newindex = error
}

-- A quick introduction to the file format (which is supposed to be mmap()'d and
-- so actually requires quite a lot of jumping around):
--
-- Numbers (card16/card32) are written in big endian
--
-- The file begins with the following header:
-- c16: Major version (1)
-- c16: Minor version (0)
-- c32: hash table offset
-- c32: directory list offset
--
-- The hash table begins with its size as c32, then comes [size] times c32
-- offsets that point to the beginning of that specific hash bucket chain.
-- c32: size
-- c32*[size]: Offsets for hash bucket chain beginnings
--
-- Given an icon name as a const signed char* p, the underlying hash function
-- here is (beware, signed char!):
--
-- uint32 h = 0;
-- while (*p != 0) {
--   h = (h << 5) - h + *p;
--   p++;
-- }
-- return h;
--
-- And the hash table is then indexed by (h % hash-table-size).
--
--
-- Each hash bucket chain entry has the following format:
-- c32: next offset
-- c32: name offset (string)
-- c32: image list offset
--
-- The [next offset] points to the next entry in this hash bucket chain, or is
-- 0xffffffff for the last entry. The [name offset] points to a string
-- identifying this entry.
--
-- A string is saved by writing it to the file, then at least one zero byte and
-- then as many zero bytes as needed to reach a divisible-by-four offset.
--
-- An image list begins with a c32 describing its size and then that many image
-- list entries, where each entry looks like:
-- c16: dir index offset
-- c16: flags
-- c32: image data offset (zero if not present)
--
-- The directory index begins with a c32 describing the number of entries and
-- then a list of directory string offsets with that length.

-- Read a single card16 from the given file handle.
local function read16(handle)
    local bytes = handle:read(2)
    local a, b = bytes:byte(1, 2)
    return a * 0x100 + b
end

-- Read a single card32 from the given file handle.
local function read32(handle)
    local bytes = handle:read(4)
    local a, b, c, d = bytes:byte(1, 4)
    return a * 0x1000000 + b * 0x10000 + c * 0x100 + d
end

-- Read a single, \0-terminated string from the given file handle.
local function read_string(handle)
    local result = {}
    local c = handle:read(1)
    while c ~= '\0' do
        table.insert(result, c)
        c = handle:read(1)
    end
    return table.concat(result)
end

-- Hash the given string in the way specified for icon_theme.cache.
local function hash(name)
    local result = 0
    for i = 1, #name do
        local char = name:sub(i, i):byte()
        result = result * 32 - result + char
        -- Simulate uint32
        result = result % 0x100000000
    end
    return result
end

-- Mask must have just a single bit set as an integer!
local function is_bit_set(number, mask)
    return number % (2*mask) >= mask
end

-- Get the mtime of a file; this is the only use of lgi / Gio in this file.
-- Returns nil on error (e.g. file does not exist)
local function get_mtime(path)
    local info = Gio.File.new_for_path(path)
        :query_info("time::modified", Gio.FileQueryInfoFlags.NONE)
    return info and info:get_attribute_uint64("time::modified")
end

--- Open an icon_theme.cache file.
-- Compared to @{open_direct}, this compares the mtime of the file and the
-- directory to check if the `icon-theme.cache` file is outdated.
-- @tparam string dirname Path to the directory containing the icon theme.
-- @return[1] An icon theme instance
-- @return[2] nil
-- @treturn[2] String Error message
-- @see close
function theme.open(dirname)
    local filename = dirname .. "/icon-theme.cache"

    local dir_mtime = get_mtime(dirname)
    local file_mtime = get_mtime(filename)
    if not file_mtime or file_mtime < dir_mtime then
        return nil, "icon-theme.cache does not exist or is not up to date"
    end
    return theme.open_direct(filename)
end

--- Open an icon_theme.cache file.
-- @tparam string filename File to open.
-- @return[1] An icon theme instance
-- @return[2] nil
-- @treturn[2] String Error message
-- @see close
function theme.open_direct(filename)
    local f, err = io.open(filename, "r")
    if not f then
        return nil, err
    end

    -- Check file header
    local major = read16(f)
    local minor = read16(f)
    if major ~= 1 or minor ~= 0 then
        return nil, string.format("Invalid file header, expected version 1.0,"
                .. "but got %d.%d", major, minor)
    end

    local hash_offset = read32(f)
    local dir_list_offset = read32(f)

    -- read hash list size
    f:seek("set", hash_offset)
    local hash_size = read32(f)

    -- read hash list size
    f:seek("set", dir_list_offset)
    local dir_list_size = read32(f)

    return setmetatable({
        f,
        hash_offset = hash_offset,
        hash_size = hash_size,
        dir_list_offset = dir_list_offset,
        dir_list_size = dir_list_size,
    }, theme_mt)
end

--- Close an icon theme file.
-- Each instance of this class keeps a file descriptor open. Since there is a
-- limit on the number of open files, you should close icon themes when you no
-- longer use them.
function theme:close()
    self[1]:close()
end

local function read_next_bucket_entry(f, next_offset)
    if next_offset == 0xffffffff then
        -- End of the hash chain
        return
    end

    f:seek("set", next_offset)
    next_offset = read32(f)
    local name_offset = read32(f)
    local image_list_offset = read32(f)

    f:seek("set", name_offset)
    local name = read_string(f)

    return next_offset, name, image_list_offset
end

-- Iterator function to iterate over the entries of a hash bucket.
-- Use as `for _, name, image_list_offset in theme:_read_bucket(42)`
function theme:_read_bucket(i)
    local f = self[1]
    assert(i >= 0 and i < self.hash_size)

    -- First entry in the hash table is size of table, then skip to i-th entry
    local chain_begin_offset = self.hash_offset + 4 + 4*i
    f:seek("set", chain_begin_offset)
    local next_offset = read32(f)

    return read_next_bucket_entry, f, next_offset
end

local function read_directory(self, index)
    assert(index >= 0 and index < self.dir_list_size)
    local f = self[1]
    f:seek("set", self.dir_list_offset + 4 + index * 4)
    local name_offset = read32(f)
    f:seek("set", name_offset)
    local name = read_string(f)
    return name
end

local function flags_to_file_extensions(flags)
    local res = {}
    if is_bit_set(flags, 2^3) then
        table.insert(res, "icon")
        flags = flags - 2^3
    end
    if is_bit_set(flags, 2^2) then
        table.insert(res, "png")
        flags = flags - 2^2
    end
    if is_bit_set(flags, 2^1) then
        table.insert(res, "svg")
        flags = flags - 2^1
    end
    if is_bit_set(flags, 2^0) then
        table.insert(res, "xpm")
        flags = flags - 2^0
    end
    assert(flags == 0, string.format("Unexpected flags remaining: 0x%x", flags))
    return res
end

local function read_next_image_list_entry(arg, var)
    local self = arg[1]
    local f = self[1]
    if var >= arg[2] then
        return
    end
    f:seek("set", var)
    local dir = read16(f)
    local flags = read16(f)
    local img_data_offset = read32(f)
    return var+8, read_directory(self, dir), flags_to_file_extensions(flags), img_data_offset
end

-- Iterator function to iterate over the entries of an image list.
-- Use as `for _, dir, extensions, img_data_offset in theme:_read_image_list(42)`
function theme:_read_image_list(image_list_offset)
    local f = self[1]
    f:seek("set", image_list_offset)
    local num_entries = read32(f)
    local offset = image_list_offset + 4

    return read_next_image_list_entry, { self, offset + 8 * num_entries }, offset
end

--- Lookup all the icons for a given name in this icon theme cache.
-- @tparam string name Name of the icon to find
-- @return Iterator function to iterate over the entries for this name.
-- @usage
--    for _, dir, extensions in self:lookup_icons("appointment-soon") do
--            local prefix = "./" .. dir .. "/" .. name .. "."
--            for _, suffix in ipairs(extensions) do
--                print(prefix .. suffix)
--            end
--    end
function theme:lookup_icons(name)
    local h = hash(name) % self.hash_size
    for _, n, image_list_offset in self:_read_bucket(h) do
        if n == name then
            return self:_read_image_list(image_list_offset)
        end
    end
    -- Not found
    return function() end
end

--- Dump the complete contents of the hash part
-- @tparam[opt] function log `print`-like function logging details of the structure
-- of the file.
-- @tparam[opt] function path `print`-like function logging existing files.
function theme:dump_hash(log, path)
    local function nop() end
    log = log or nop
    path = path or nop

    -- For each hash bucket
    for i = 0, self.hash_size-1 do
        log()
        log(i)
        -- For each entry in this bucket's hash chain
        for _, name, image_list_offset in self:_read_bucket(i) do
            log(name)
            assert(hash(name) % self.hash_size == i, name) -- Check that the hash table is intact

            -- For each entry in the list of images for this name
            for _, dir, extensions, img_data_offset in self:_read_image_list(image_list_offset) do
                -- Format the list of file extensions
                local extensions_str = table.concat(extensions, ", ")
                extensions_str = extensions_str == "" and "none" or extensions_str
                extensions_str = string.format("%30s", extensions_str or "none")
                extensions_str = string.rep(" ", 30 - #extensions_str) .. extensions_str

                -- Is there an inline image present?
                local img = img_data_offset ~= 0 and "inline image" or ""
                log("", extensions_str, dir, img)

                -- Log all existing files.
                local prefix = "./" .. dir .. "/" .. name
                for _, ext in pairs(extensions) do
                    path(prefix .. "." .. ext)
                end
            end
        end
    end
end

return theme

-- vim: filetype=lua:expandtab:shiftwidth=4:tabstop=8:softtabstop=4:textwidth=80
