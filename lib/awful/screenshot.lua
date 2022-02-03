---------------------------------------------------------------------------
--- Screenshots and related configuration settings
--
-- @author Brian Sobulefsky &lt;brian.sobulefsky@protonmail.com&gt;
-- @copyright 2021 Brian Sobulefsky
-- @inputmodule awful.screenshot
---------------------------------------------------------------------------

-- Grab environment we need
local capi = { root = root,
               screen = screen,
               client = client,
               mousegrabber = mousegrabber }
local aw_screen = require("awful.screen")
local gears = require("gears")
local beautiful = require("beautiful")
local wibox = require("wibox")
local cairo = require("lgi").cairo
local naughty = require("naughty")

-- The module to be returned
local module = { mt = {} }
-- The screenshow object created by a call to the screenshow module
local screenshot = {}
-- The various methods of taking a screenshow (root window, client, etc).
local screenshot_methods = {}

-- Configuration data
local ss_dir = nil
local ss_prfx = nil
local frame_color = gears.color("#000000")
local initialized = nil


-- Internal function to get the default screenshot directory
-- 
-- Can be expanded to read X environment variables or configuration files.
local function get_default_dir()
  -- This can be expanded
  local d = os.getenv("HOME")
  
  if d then
    d = string.gsub(d, '/*$', '/') .. 'Images/'
    if os.execute("bash -c \"if [ -d \\\"" .. d .. "\\\" -a -w \\\"" .. d ..
                    "\\\" ] ; then exit 0 ; else exit 1 ; fi ;\"") then
      return d
    end
  end

  return nil
end

-- Internal function to get the default filename prefix
local function get_default_prefix()
  return "Screenshot-"
end

-- Internal function to check a directory string for existence and writability.
-- This only checks if the requested directory exists and is writeable, not if
-- such a directory is legal (i.e. it behaves as 'mkdir' and not 'mkdir -p').
-- Adding 'mkdir -p' functionality can be considered in the future.
local function check_directory(directory)

  if directory and type(directory) == "string"  then

    -- Fully qualify a "~/" path or a relative path to $HOME. One might argue
    -- that the relative path should fully qualify to $PWD, but for a window
    -- manager this should be the same thing to the extent that anything else
    -- is arguably unexpected behavior.
    if string.find(directory, "^~/") then
      directory = string.gsub(directory, "^~/",
                              string.gsub(os.getenv("HOME"), "/*$", "/"))
    elseif string.find(directory, "^[^/]") then
      directory = string.gsub(os.getenv("HOME"), "/*$", "/") .. directory
    end

    -- Assure that we return exactly one trailing slash
    directory = string.gsub(directory, '/*$', '/')

    -- If we use single quotes, we only need to deal with single quotes - (I
    -- promise that's meaningful if you think about it from a bash perspective)
    local dir = string.gsub(directory, "'", "'\\'\\\\\\'\\''")

    if os.execute("bash -c 'if [ -d '\\''" .. dir .. "'\\'' -a -w '\\''" ..
                  dir .. "'\\'' ] ; then exit 0 ; else exit 1 ; fi'") then
      return directory
    else
      -- Currently returns nil if the requested directory string cannot be used.
      -- This can be swapped to a silent fallback to the default directory if
      -- desired. It is debatable which way is better.
      return nil
    end

  else
    -- No directory argument means use the default. Technically an outrageously
    -- invalid argument (i.e. not even a string) currently falls back to the
    -- default as well.
    return get_default_dir()
  end

end

-- Internal function to sanitize a prefix string
--
-- Currently only strips all path separators ('/'). Allows for empty prefix.
local function check_prefix(prefix)
  -- Maybe add more sanitizing eventually
  if prefix and type(prefix) == "string" then
    prefix = string.gsub(prefix, '/', '')
    return prefix
  end
  return get_default_prefix()
end

-- Internal routine to verify that a filepath is valid.
local function check_filepath(filepath)

  -- This module is forcing png for now. In the event of adding more
  -- options, this function is basically unchanged, except for trying
  -- to match a regex for each supported format (e.g. (png|jpe?g|gif|bmp))
  -- NOTE: we should maybe make this case insensitive too?
  local fname_start, fname_end = string.find(filepath,'/[^/]+%.png$')

  if fname_start and fname_end then
    local directory = string.sub(filepath, 1, fname_start)
    local file_name = string.sub(filepath, fname_start + 1, fname_end)
    directory = check_directory(directory)
    if directory then
      return directory .. file_name
    else
      return nil
    end
  end

  return nil

end

-- Internal function to attempt to parse a filepath into directory and prefix.
-- Returns directory, prefix if both, directory if no prefix, or nil if invalid
local function parse_filepath(filepath)

  -- Same remark as above about adding more image formats in the future
  local fname_start, fname_end = string.find(filepath,'/[^/]+%.png$')

  if fname_start and fname_end then

    local directory = string.sub(filepath, 1, fname_start)
    local file_name = string.sub(filepath, fname_start + 1, fname_end)

    if fname_end - fname_start > 14 + 4 then

      local base_name = string.sub(file_name, 1, #file_name - 4)
      -- Is there a repeat count in Lua patterns, like {14} for most regexes?
      local date_start, date_end = string.find(base_name, '%d%d%d%d%d%d%d%d%d%d%d%d%d%d$')

      if date_start and date_end then
        return directory, string.sub(base_name, 1, date_start - 1)
      end

    end

    return directory

  end

  return nil

end

-- Internal function to configure the directory/prefix pair for any call to the
-- screenshot API.
--
-- This supports using a different directory or filename prefix for a particular
-- use by passing the optional arguments. In the case of an initalized object
-- with  no arguments passed, the stored configuration parameters are quickly
-- returned. This also technically allows the user to take a screenshot in an
-- unitialized state and without passing any arguments.
local function configure_path(directory, prefix)

  local d, p

  if not initialized or directory then
    d = check_directory(directory)
    if not d then
      return
    end
  else
    d = ss_dir
  end


  if not initialized or prefix then
    p = check_prefix(prefix)
    if not p then 
      return
    end
  else
    p = ss_prfx
  end

  -- In the case of taking a screenshot in an unitialized state, store the
  -- configuration parameters and toggle to initialized going forward.
  if not initialized then
    ss_dir = d
    ss_prfx = p
    initilialized = true
  end
  
  return d, p

end

local function make_filepath(directory, prefix)
  local dir, prfx
  local date_time = tostring(os.date("%Y%m%d%H%M%S"))
  dir, prfx = configure_path(directory, prefix)
  local filepath = dir .. prfx .. date_time .. ".png"
  return dir, prfx, filepath
end

-- Internal function to do the actual work of taking a cropped screenshot
--
-- This function does not do any data checking. Public functions should do
-- all data checking. This function was made to combine the interactive sipper
-- run by the mousegrabber and the programmatically defined snip function,
-- though there may be other uses.
local function crop_shot(x, y, width, height)

  local source, target, cr

  source = gears.surface(root.content())
  target = source:create_similar(cairo.Content.COLOR,
                         width, height)

  cr = cairo.Context(target)
  cr:set_source_surface(source, -x, -y)
  cr:rectangle(0, 0, width, height)
  cr:fill()

  return target

end


-- Internal function used by the snipper mousegrabber to update the frame outline
-- of the current state of the cropped screenshot
--
-- This function is largely a copy of awful.mouse.snap.show_placeholder(geo),
-- so it is probably a good idea to create a utility routine to handle both use
-- cases. It did not seem appropriate to make that change as a part of the pull
-- request that was creating the screenshot API, as it would clutter and
-- confuse the change.
function show_frame(ss, geo)

  if not geo then
      if ss._private.frame then
          ss._private.frame.visible = false
      end
      return
  end

  ss._private.frame = ss._private.frame or wibox {
      ontop = true,
      bg    = frame_color
  }

  local frame = ss._private.frame

  frame:geometry(geo)

  -- Perhaps the preexisting image of frame can be reused? I tried but could
  -- not get it to work. I am not sure if there is a performance penalty
  -- incurred by making a new Cairo ImageSurface each execution.
  local img = cairo.ImageSurface(cairo.Format.A1, geo.width, geo.height)
  local cr = cairo.Context(img)

  cr:set_operator(cairo.Operator.CLEAR)
  cr:set_source_rgba(0,0,0,1)
  cr:paint()
  cr:set_operator(cairo.Operator.SOURCE)
  cr:set_source_rgba(1,1,1,1)

  local line_width = 1
  cr:set_line_width(beautiful.xresources.apply_dpi(line_width))

  cr:translate(line_width,line_width)
  gears.shape.partially_rounded_rect(cr, geo.width - 2*line_width,
                                     geo.height - 2*line_width,
                                     false, false, false, false, nil)

  cr:stroke()

  frame.shape_bounding = img._native
  img:finish()

  frame.visible = true

end

-- Internal function that generates the callback to be passed to the
-- mousegrabber that implements the snipper.
--
-- The snipper tool is basically a mousegrabber, which takes a single function
-- of one argument, representing the mouse state data. This is a simple
-- starting point that hard codes the snipper tool as being a two press design
-- using button 1, with button 3 functioning as the cancel. These aspects of
-- the interface can be made into parameters at some point passed as arguments.
local function mk_mg_callback(ss)

  ss._private.mg_first_pnt = {}

  local function ret_mg_callback(mouse_data)

    if mouse_data["buttons"][3] then
      if ss._private.frame and ss._private.frame.visible then
        show_frame(ss)
      end
      ss._private.mg_first_pnt = nil
      ss._private.frame = nil
      return false
    end
  
    if ss._private.mg_first_pnt[1] then
  
      local min_x, max_x, min_y, max_y
      min_x = math.min(ss._private.mg_first_pnt[1], mouse_data["x"])
      max_x = math.max(ss._private.mg_first_pnt[1], mouse_data["x"])
      min_y = math.min(ss._private.mg_first_pnt[2], mouse_data["y"])
      max_y = math.max(ss._private.mg_first_pnt[2], mouse_data["y"])
  
      -- Force a minimum size to the box
      if  max_x - min_x < 4 or max_y - min_y < 4 then
        if frame and frame.visible then
          show_frame(ss)
        end
      elseif not mouse_data["buttons"][1] then
        show_frame(ss, {x = min_x, y = min_y, width = max_x - min_x, height = max_y - min_y})
      end
  
      if mouse_data["buttons"][1] then
  
        local snip_surf
        local date_time
  
        if ss._private.frame and ss._private.frame.visible then
          show_frame(ss)
        end
  
        ss._private.frame = nil
        ss._private.mg_first_pnt = nil
  
        -- This may fail gracefully anyway but require a minimum 3x3 of pixels
        if min_x >= max_x-1 or min_y >= max_y-1 then
          return false
        end
  
        snip_surf = crop_shot(min_x, min_y, max_x - min_x, max_y - min_y)
        ss:filepath_builder()
        ss._private.surface = gears.surface(snip_surf) -- surface has no setter
  
        if ss._private.on_success_cb then
          ss._private.on_success_cb(ss)
        end
  
        return false
  
      end
  
    else
      if mouse_data["buttons"][1] then
        ss._private.mg_first_pnt[1] = mouse_data["x"]
        ss._private.mg_first_pnt[2] = mouse_data["y"]
      end
    end
  
    return true

  end

  return ret_mg_callback

end

-- Internal function to be passed as the default callback upon completion of
-- the mousgrabber for the snipper if the user does not pass one.
function default_on_success_cb(ss)
  ss:save()
end

-- Internal function exected when a root window screenshot is taken.
function root_screenshot(ss)
  local w, h = root.size()
  ss._private.geometry = {x = 0, y = 0, width = w, height = h}
  ss:filepath_builder()
  ss._private.surface = gears.surface(capi.root.content()) -- surface has no setter
  return ss
end

-- Internal function executed when a physical screen screenshot is taken.
function screen_screenshot(ss)

  -- note the use of _private because screen has no setter
  if ss.screen then
    if type(ss.screen) == "number" then
      ss._private.screen = capi.screen[ss.screen] or aw_screen.focused()
    elseif ss.screen.index and type(ss.screen.index) == "number" then
      ss._private.screen = capi.screen[ss.screen.index] or aw_screen.focused()
    end
  else
    ss._private.screen = aw_screen.focused()
  end

  ss._private.geometry = ss.screen.geometry
  ss:filepath_builder()
  ss._private.surface = gears.surface(ss.screen.content) -- surface has no setter

  return ss

end

-- Internal function executed when a client window screenshot is taken.
function client_screenshot(ss)
  -- note the use of _private becuase client has no setter
  if ss.client and ss.client.content then
    -- If the passed client arg has a content property we'll take it.
  else
    ss._private.client = capi.client.focus 
  end
  ss._private.geometry = ss.client:geometry()
  ss:filepath_builder()
  ss._private.surface = gears.surface(ss.client.content) -- surface has no setter
  return ss
end

-- Internal function executed when a snipper screenshot tool is launched.
function snipper_screenshot(ss)

  if type(ss._private.on_success_cb) ~= "function" then
    ss._private.on_success_cb = default_on_success_cb -- the cb has no setter
  end

  local mg_callback = mk_mg_callback(ss)
  capi.mousegrabber.run(mg_callback, "crosshair")

  return true

end

-- Internal function executed when a snip screenshow (a defined geometry) is
-- taken.
function snip_screenshot(ss)

  local root_w, root_h
  local root_intrsct
  local snip_surf

  root_w, root_h = root.size()

  if not(ss.geometry and
         type(ss.geometry.x) == "number" and
         type(ss.geometry.y) == "number" and
         type(ss.geometry.width) == "number" and
         type(ss.geometry.height) == "number") then

    -- Default to entire root window. Also geometry has no setter.
    ss._private.geometry = {x = 0, y = 0, width = root_w, height = root_h}

  end

  root_intrsct = gears.geometry.rectangle.get_intersection(ss.geometry,
                                                           {x = 0, y = 0,
                                                            width = root_w,
                                                            height = root_h})

  snip_surf = crop_shot(root_intrsct.x, root_intrsct.y,
                          root_intrsct.width, root_intrsct.height)

  ss:filepath_builder()
  ss._private.surface = gears.surface(snip_surf) -- surface has no setter

  return ss

end

-- Screenshot methods
screenshot_methods.root = root_screenshot
screenshot_methods.screen = screen_screenshot
screenshot_methods.client = client_screenshot
screenshot_methods.snipper = snipper_screenshot
screenshot_methods.snip = snip_screenshot
-- Default method is root
screenshot_methods.default = root_screenshot
local default_method_name = "root"

-- Module routines

--- Function to initialize the screenshot library
--
-- Currently only sets the screenshot directory, the filename, prefix and the
-- snipper tool outline color. More initialization can be added as the API
-- expands.
-- @staticfct awful.screenshot.set_defaults
-- @tparam[opt] table args Table passed with the configuration data.
-- @treturn boolean true or false depending on success
function module.set_defaults(args)
  local tmp

  tmp = check_directory(args.directory)

  if tmp then
    ss_dir = tmp
  else
    initialized = nil
    return false
  end

  tmp = check_prefix(args.prefix)

  if tmp then
    ss_prfx = tmp
  else
    -- Should be unreachable as the default will always be taken
    initialized = nil
    return false
  end

  -- Don't throw out prior init data if only color is misformed
  initialized = true

  if args.frame_color then
    tmp = gears.color(args.frame_color)
    if tmp then
      frame_color = tmp
    end
  end

  return true
end

--- Take root window screenshots.
--
-- This is a wrapper constructor for a root window screenshot. See the main
-- constructor, new(), for details about the arguments.
--
-- @constructorfct awful.screenshot.root
-- @tparam[opt] table args Table of arguments to pass to the constructor
-- @treturn screenshot The screenshot object
function module.root(args)
  local args = (type(args) == "table" and args) or {}
  args.method = "root"
  return module(args)
end

--- Take physical screen screenshots
--
-- This is a wrapper constructor for a physical screen screenshot. See the main
-- constructor, new(), for details about the arguments.
--
-- @staticfct awful.screenshot.screen
-- @tparam[opt] table args Table of arguments to pass to the constructor
-- @treturn screenshot The screenshot object
function module.screen(args)
  local args = (type(args) == "table" and args) or {}
  args.method = "screen"
  return module(args)
end

--- Take client window screenshots
--
-- This is a wrapper constructor for a client window screenshot. See the main
-- constructor, new(), for details about the arguments.
--
-- @staticfct awful.screenshot.client
-- @tparam[opt] table args Table of arguments to pass to the constructor
-- @treturn screenshot The screenshot object
function module.client(args)
  -- Looking at the properties and functions available, I'm not sure it is
  -- wise to allow a "target" client argument, but if we want to add it as
  -- arg 3 (which will match the screen ordering), we can.
  local args = (type(args) == "table" and args) or {}
  args.method = "client"
  return module(args)
end

--- Launch an interactive snipper tool to take cropped shots
--
-- This is a wrapper constructor for a snipper tool screenshot. See the main
-- constructor, new(), for details about the arguments.
--
-- @staticfct awful.screenshot.snipper
-- @tparam[opt] table args Table of arguments to pass to the constructor
-- @treturn screenshot The screenshot object
function module.snipper(args)
  local args = (type(args) == "table" and args) or {}
  args.method = "snipper"
  return module(args)
end

--- Take a cropped screenshot of a defined geometry
--
-- This is a wrapper constructor for a snip screenshot (defined geometry). See
-- the main constructor, new(), for details about the arguments.
--
-- @staticfct awful.screenshot.snip
-- @tparam[opt] table args Table of arguments to pass to the constructor
-- @treturn screenshot The screenshot object
function module.snip(args)
  local args = (type(args) == "table" and args) or {}
  args.method = "snip"
  return module(args)
end

-- Various accessors for the screenshot object returned by any public
-- module method.

--- Get screenshot directory property
--
-- @property directory
function screenshot:get_directory()
  return self._private.directory
end

--- Set screenshot directory property
--
-- @property directory
-- @tparam string directory The path to the screenshot directory
function screenshot:set_directory(directory)
  if type(directory) == "string" then
    local dir = check_directory(directory)
    if dir then
      self._private.directory = dir
    end
  end
end

--- Get screenshot prefix property
--
-- @property prefix
function screenshot:get_prefix()
  return self._private.prefix
end

--- Set screenshot prefix property
--
-- @property prefix
-- @tparam string prefix The prefix prepended to screenshot files names.
function screenshot:set_prefix(prefix)
  if type(prefix) == "string" then
    local prfx = check_prefix(prefix)
    if prfx then
      self._private.prefix = prfx
    end
  end
end

--- Get screenshot filepath
--
-- @property filepath
function screenshot:get_filepath()
  return self._private.filepath
end

--- Set screenshot filepath
--
-- @property filepath
-- @tparam[opt] string fp The full path to the filepath
function screenshot:set_filepath(fp)
  self:filepath_builder({filepath = fp})
end

--- Get screenshot method name
--
-- @property method_name
function screenshot:get_method_name()
  return self._private.method_name
end

--- Get screenshot screen
--
-- @property screen
function screenshot:get_screen()
  if self.method_name == "screen" then
    return self._private.screen
  else
    return nil
  end
end

--- Get screenshot client
--
-- @property client
function screenshot:get_client()
  if self.method_name == "client" then
    return self._private.client
  else
    return nil
  end
end

--- Get screenshot geometry
--
-- @property geometry
function screenshot:get_geometry()
  return self._private.geometry
end

--- Get screenshot surface
--
-- @property surface
function screenshot:get_surface()
  return self._private.surface
end

-- Methods for the screenshot object returned from taking a screenshot.

--- Set the filepath to save a screenshot
--
-- @function awful.screenshot:filepath_builder
-- @tparam[opt] table args Table with the filepath parameters
function screenshot:filepath_builder(args)

  local args = (type(args) == "table" and args) or {}

  local filepath = args.filepath
  local directory = args.directory
  local prefix = args.prefix


  if filepath and check_filepath(filepath) then

    directory, prefix = parse_filepath(filepath)

  elseif directory or prefix then

    if directory and type(directory) == "string" then
      directory = check_directory(directory)
    elseif self.directory then
      directory = self._private.directory -- The setter ran check_directory()
    else
      directory = get_default_dir()
    end

    if prefix and type(prefix) == "string" then
      prefix = check_prefix(prefix)
    elseif self.prefix then
      prefix = self._private.prefix -- The setter ran check_prefix()
    else
      prefix = get_default_prefix()
    end

    directory, prefix, filepath = make_filepath(directory, prefix) 

  elseif self.filepath and check_filepath(self.filepath) then

    filepath = self.filepath
    directory, prefix = parse_filepath(filepath)

  else

    if directory and type(directory) == "string" then
      directory = check_directory(directory)
    elseif self.directory then
      directory = self._private.directory -- The setter ran check_directory()
    else
      directory = get_default_dir()
    end

    if prefix and type(prefix) == "string" then
      prefix = check_prefix(prefix)
    elseif self.prefix then
      prefix = self._private.prefix -- The setter ran check_prefix()
    else
      prefix = get_default_prefix()
    end

    directory, prefix, filepath = make_filepath(directory, prefix) 

  end

  if filepath then
    self._private.directory = directory -- These have already
    self._private.prefix = prefix       -- been checked.
    self._private.filepath = filepath
  end

end

--- Save screenshot
--
-- @function awful.screenshot:save
-- @tparam[opt] table args Table with the filepath parameters
function screenshot:save(args)

  self:filepath_builder(args)

  if self._private.surface and self.filepath then
    self._private.surface:write_to_png(self.filepath)
  end

end

--- Screenshot constructor - it is possible to call this directly, but it is
-- recommended to use the helper constructors, such as awful.screenshot.root
--
-- Possible args include:
--   directory [string]: the path to the screenshot directory
--   prefix [string]: the prefix to prepend to screenshot file names
--   frame_color: the color of the frame for a snipper tool
--   method: the method of screenshot to take (i.e. root window, etc).
--   screen: the screen for a physical screen screenshot
--   on_success_cb: the callback to run on the screenshot taken with
--                  a snipper tool
--   geometry: a geometry object for a snip screenshot
--
-- @constructorfct awful.screenshot.new
-- @tparam[opt] table args Table with the constructor parameters
local function new(_, args)

  local args = (type(args) == "table" and args) or {}
  local ss = gears.object({
    enable_auto_signals = true,
    enable_properties = true
  })

  dir, prfx = configure_path(args.directory, args.prefix)

  local fclr = nil
  if args.frame_color then
    fclr = gears.color(args.frame_color)
    if not fclr then
      fclr = frame_color
    end
  else
    fclr = frame_color
  end

  local mthd = nil
  local mthd_name = ""
  if screenshot_methods[args.method] then
    mthd = screenshot_methods[args.method]
    mthd_name = args.method
  else
    mthd = screenshot_methods.default
    mthd_name = default_method_name
  end

  local scrn, clnt, mg_cb, geom
  if mthd_name == "screen" then
    scrn = args.screen
  elseif mthd_name == "client" then
    clnt = args.client
  elseif mthd_name == "snipper" then
    mg_cb = args.on_success_cb 
  elseif mthd_name == "snip" then
    geom = args.geometry
  end

  if type(args.on_success_cb) == "function" then
    mg_cb = args.on_success_cb
  else
    mg_cb = default_on_success_cb
  end

  ss._private = {
    directory = dir,
    prefix = prfx,
    filepath = nil,
    method_name = mthd_name,
    frame_color = fclr,
    screen = scrn,
    client = clnt,
    on_success_cb = mg_cb,
    geometry = geom,
    surface = nil
  }

  gears.table.crush(ss, screenshot, true)

  return mthd(ss)

end

return setmetatable(module, {__call = new})
