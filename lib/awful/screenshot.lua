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
local module_default_directory = nil
local module_default_prefix = nil
local module_default_frame_color = gears.color("#000000")
local initialized = nil


-- Internal function to get the default screenshot directory
-- 
-- Can be expanded to read X environment variables or configuration files.
local function get_default_dir()
  -- This can be expanded
  local home_dir = os.getenv("HOME")
  
  if home_dir then
    home_dir = string.gsub(home_dir, '/*$', '/') .. 'Images/'
    if gears.filesystem.dir_writable(home_dir) then
      return home_dir
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

  print("Enter check_directory()")
  print(directory)
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

    print("After first sanitation -- " .. directory)

    -- Assure that we return exactly one trailing slash
    directory = string.gsub(directory, '/*$', '/')
    print("After second sanitation -- " .. directory)

    if gears.filesystem.dir_writable(directory) then
      print("Returning directory")
      return directory
    else
      -- Currently returns nil if the requested directory string cannot be used.
      -- This can be swapped to a silent fallback to the default directory if
      -- desired. It is debatable which way is better.
      print("Returning nil")
      return nil
    end

  else
    -- No directory argument means use the default. Technically an outrageously
    -- invalid argument (i.e. not even a string) currently falls back to the
    -- default as well.
    print("Returning default directory")
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
  local filename_start, filename_end = string.find(filepath,'/[^/]+%.png$')

  if filename_start and filename_end then
    local directory = string.sub(filepath, 1, filename_start)
    local file_name = string.sub(filepath, filename_start + 1, filename_end)
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
  local filename_start, filename_end = string.find(filepath,'/[^/]+%.png$')

  if filename_start and filename_end then

    if filename_end - filename_start > 14 + 4 then

      local directory = string.sub(filepath, 1, filename_start)
      local file_name = string.sub(filepath, filename_start + 1, filename_end)

      local base_name = string.sub(file_name, 1, #file_name - 4)
      -- Is there a repeat count in Lua, like %d{14}, as for most regexes?
      local date_start, date_end =
                      string.find(base_name, '%d%d%d%d%d%d%d%d%d%d%d%d%d%d$')

      if date_start and date_end then
        return directory, string.sub(base_name, 1, date_start - 1)
      end

      return directory

    end

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

  local _directory, _prefix

  if not initialized or directory then
    _directory = check_directory(directory)
    if not _directory then
      return
    end
  else
    _directory = module_default_directory
  end


  if not initialized or prefix then
    _prefix = check_prefix(prefix)
    if not _prefix then 
      return
    end
  else
    _prefix = module_default_prefix
  end

  -- In the case of taking a screenshot in an unitialized state, store the
  -- configuration parameters and toggle to initialized going forward.
  if not initialized then
    module_default_directory = _directory
    module_default_prefix = _prefix
    initilialized = true
  end
  
  return _directory, _prefix

end

local function make_filepath(directory, prefix)
  local _directory, _prefix
  local date_time = tostring(os.date("%Y%m%d%H%M%S"))
  _directory, _prefix = configure_path(directory, prefix)
  local filepath = _directory .. _prefix .. date_time .. ".png"
  return _directory, _prefix, filepath
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
local function show_frame(ss, geo)

  if not geo then
      if ss._private.frame then
          ss._private.frame.visible = false
      end
      return
  end

  ss._private.frame = ss._private.frame or wibox {
      ontop = true,
      bg    = ss._private.frame_color
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
local function default_on_success_cb(ss)
  ss:save()
end

-- Internal function exected when a root window screenshot is taken.
function screenshot_methods.root(ss)
  local w, h = root.size()
  ss._private.geometry = {x = 0, y = 0, width = w, height = h}
  ss:filepath_builder()
  ss._private.surface = gears.surface(capi.root.content()) -- surface has no setter
  return ss
end

-- Internal function executed when a physical screen screenshot is taken.
function screenshot_methods.screen(ss)

  -- note the use of _private because screen has no setter
  if ss.screen then
    if type(ss.screen) == "number" then
      ss._private.screen = capi.screen[ss.screen] or aw_screen.focused()
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
function screenshot_methods.client(ss)
	--
  -- note the use of _private becuase client has no setter
  if not ss.client then
    ss._private.client = capi.client.focus 
  end

  ss._private.geometry = ss.client:geometry()
  ss:filepath_builder()
  ss._private.surface = gears.surface(ss.client.content) -- surface has no setter
  return ss
end

-- Internal function executed when a snipper screenshot tool is launched.
function screenshot_methods.snipper(ss)

  if type(ss._private.on_success_cb) ~= "function" then
    ss._private.on_success_cb = default_on_success_cb -- the cb has no setter
  end

  local mg_callback = mk_mg_callback(ss)
  capi.mousegrabber.run(mg_callback, "crosshair")

  return true

end

-- Internal function executed when a snip screenshow (a defined geometry) is
-- taken.
function screenshot_methods.snip(ss)

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

-- Default method is root
screenshot_methods.default = screenshot_methods.root
local default_method_name = "root"

-- Module routines

--- Function to initialize the screenshot library.
--
-- Currently only sets the screenshot directory, the filename, prefix and the
-- snipper tool outline color. More initialization can be added as the API
-- expands.
-- @staticfct awful.screenshot.set_defaults
-- @tparam[opt] table args Table passed with the configuration data.
-- @treturn boolean true or false depending on success
function module.set_defaults(args)

  local args = (type(args) == "table" and args) or {}
  local tmp = check_directory(args.directory)

  if tmp then
    module_default_directory = tmp
  else
    initialized = nil
    return false
  end

  tmp = check_prefix(args.prefix)

  if tmp then
    module_default_prefix = tmp
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

--- Take physical screen screenshots.
--
-- This is a wrapper constructor for a physical screen screenshot. See the main
-- constructor, new(), for details about the arguments.
--
-- @constructorfct awful.screenshot.screen
-- @tparam[opt] table args Table of arguments to pass to the constructor
-- @treturn screenshot The screenshot object
function module.screen(args)
  local args = (type(args) == "table" and args) or {}
  args.method = "screen"
  return module(args)
end

--- Take client window screenshots.
--
-- This is a wrapper constructor for a client window screenshot. See the main
-- constructor, new(), for details about the arguments.
--
-- @constructorfct awful.screenshot.client
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

--- Launch an interactive snipper tool to take cropped shots.
--
-- This is a wrapper constructor for a snipper tool screenshot. See the main
-- constructor, new(), for details about the arguments.
--
-- @constructorfct awful.screenshot.snipper
-- @tparam[opt] table args Table of arguments to pass to the constructor
-- @treturn screenshot The screenshot object
function module.snipper(args)
  local args = (type(args) == "table" and args) or {}
  args.method = "snipper"
  return module(args)
end

--- Take a cropped screenshot of a defined geometry.
--
-- This is a wrapper constructor for a snip screenshot (defined geometry). See
-- the main constructor, new(), for details about the arguments.
--
-- @constructorfct awful.screenshot.snip
-- @tparam[opt] table args Table of arguments to pass to the constructor
-- @treturn screenshot The screenshot object
function module.snip(args)
  local args = (type(args) == "table" and args) or {}
  args.method = "snip"
  return module(args)
end

-- Various accessors for the screenshot object returned by any public
-- module method.

--- Get screenshot directory property.
--
-- @property directory
function screenshot:get_directory()
  return self._private.directory
end

--- Set screenshot directory property.
--
-- @tparam string directory The path to the screenshot directory
function screenshot:set_directory(directory)
  if type(directory) == "string" then
    local checked_dir = check_directory(directory)
    if checked_dir then
      self._private.directory = checked_dir
    end
  end
end

--- Get screenshot prefix property.
--
-- @property prefix
function screenshot:get_prefix()
  return self._private.prefix
end

--- Set screenshot prefix property.
--
-- @tparam string prefix The prefix prepended to screenshot files names.
function screenshot:set_prefix(prefix)
  if type(prefix) == "string" then
    local checked_prefix = check_prefix(prefix)
    if checked_prefix then
      self._private.prefix = checked_prefix
    end
  end
end

--- Get screenshot filepath.
--
-- @property filepath
function screenshot:get_filepath()
  return self._private.filepath
end

--- Set screenshot filepath.
--
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

--- Get screenshot screen.
--
-- @property screen
function screenshot:get_screen()
  if self.method_name == "screen" then
    return self._private.screen
  else
    return nil
  end
end

--- Get screenshot client.
--
-- @property client
function screenshot:get_client()
  if self.method_name == "client" then
    return self._private.client
  else
    return nil
  end
end

--- Get screenshot geometry.
--
-- @property geometry
function screenshot:get_geometry()
  return self._private.geometry
end

--- Get screenshot surface.
--
-- @property surface
function screenshot:get_surface()
  return self._private.surface
end

-- Methods for the screenshot object returned from taking a screenshot.

--- Set the filepath to save a screenshot.
--
-- @method awful.screenshot:filepath_builder
-- @tparam[opt] table args Table with the filepath parameters
function screenshot:filepath_builder(args)

  local args = (type(args) == "table" and args) or {}

  local filepath = args.filepath
  local directory = args.directory
  local prefix = args.prefix

  print("Entering filepath_builder(args)")
  print(args.filepath)
  print(args.directory)
  print(args.prefix)

  if filepath and check_filepath(filepath) then

    print("First if in filepath_builder")
    directory, prefix = parse_filepath(filepath)

  elseif directory or prefix then

    print("Second if in filepath_builder")
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

    print("Third if in filepath_builder")
    filepath = self.filepath
    directory, prefix = parse_filepath(filepath)

  else

    print("Else in filepath_builder")
    if self.directory then
      directory = self._private.directory -- The setter ran check_directory()
    else
      directory = get_default_dir()
    end

    if self.prefix then
      prefix = self._private.prefix -- The setter ran check_prefix()
    else
      prefix = get_default_prefix()
    end

    directory, prefix, filepath = make_filepath(directory, prefix) 
    print("Finished building filepath:")
    print(directory)
    print(prefix)
    print(filepath)

  end

  if filepath then
    self._private.directory = directory -- These have already
    self._private.prefix = prefix       -- been checked.
    self._private.filepath = filepath
  end

end

--- Save screenshot.
--
-- @method awful.screenshot:save
-- @tparam[opt] table args Table with the filepath parameters
function screenshot:save(args)

  self:filepath_builder(args)

  if self._private.surface and self.filepath then
    self._private.surface:write_to_png(self.filepath)
  end

end

--- Screenshot constructor - it is possible to call this directly, but it is.
--  recommended to use the helper constructors, such as awful.screenshot.root
--
-- @constructorfct awful.screenshot
-- @tparam[opt] string args.directory The path to the screenshot directory.
-- @tparam[opt] string args.prefix The prefix to prepend to screenshot file names.
-- @tparam[opt] color args.frame_color The color of the frame for a snipper tool as
--              a gears color.
-- @tparam[opt] string args.method The method of screenshot to take (i.e. root window, etc).
-- @tparam[opt] screen args.screen The screen for a physical screen screenshot. Can be a
--              screen object of number.
-- @tparam[opt] function args.on_success_cb: the callback to run on the screenshot taken
--              with a snipper tool.
-- @tparam[opt] geometry args.geometry A gears geometry object for a snip screenshot.
local function new(_, args)

  local args = (type(args) == "table" and args) or {}
  local ss = gears.object({
    enable_auto_signals = true,
    enable_properties = true
  })

  local directory, prefix = configure_path(args.directory, args.prefix)

  local method = nil
  local method_name = ""
  if screenshot_methods[args.method] then
    method = screenshot_methods[args.method]
    method_name = args.method
  else
    method = screenshot_methods.default
    method_name = default_method_name
  end

  local screen, client, on_success_cb, frame_color, geometry
  if method_name == "screen" then
    screen = args.screen
  elseif method_name == "client" then
    client = args.client
  elseif method_name == "snipper" then

    if args.frame_color then
      frame_color = gears.color(args.frame_color)
      if not frame_color then
        frame_color = module_default_frame_color
      end
    else
      frame_color = module_default_frame_color
    end

    if type(args.on_success_cb) == "function" then
      on_success_cb = args.on_success_cb
    else
      on_success_cb = default_on_success_cb
    end

  elseif method_name == "snip" then
    geometry = args.geometry
  end

  ss._private = {
    directory = directory,
    prefix = prefix,
    filepath = nil,
    method_name = method_name,
    frame_color = frame_color,
    screen = screen,
    client = client,
    on_success_cb = on_success_cb,
    geometry = geometry,
    surface = nil
  }

  gears.table.crush(ss, screenshot, true)

  return method(ss)

end

return setmetatable(module, {__call = new})
