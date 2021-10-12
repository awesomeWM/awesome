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

local frame = nil 
local frame_color = gears.color("#000000")

local screenshot = { mt = {} }

-- Configuration data
local ss_dir = nil
local ss_prfx = nil
local initialized = nil

-- Parameters for the mousegrabber for the snipper tool
local mg_dir = nil
local mg_prfx = nil
local mg_first_pnt = {}
local mg_onsuccess_cb = nil

-- Routine to get the default screenshot directory
-- 
-- Can be expanded to read X environment variables or configuration files.
local function get_default_dir()
  -- This can be expanded
  local d = os.getenv("HOME")
  
  if d then
    d = string.gsub(d, '/$', '') .. '/Images'
    if os.execute("bash -c \"if [ -d \\\"" .. d .. "\\\" -a -w \\\"" .. d ..
                    "\\\" ] ; then exit 0 ; else exit 1 ; fi ;\"") then
      return d .. '/'
    end
  end

  return nil
end

-- Routine to get the default filename prefix
local function get_default_prefix()
  return "Screenshot-"
end

-- Routine to check a directory string for existence and writability
local function check_directory(directory)
  if directory and type(directory) == "string"  then

    -- By putting everything in dquotes, we should only need to escape
    -- dquotes and dollar signs. Exclamation must be escaped outside the quote.
    directory = string.gsub(directory, '"', '\\\\\\"')
    directory = string.gsub(directory, '%$', '\\$')
    directory = string.gsub(directory, '!', '"\\!"')
    -- Assure that we return exactly one trailing slash
    directory = string.gsub(directory, '/+$', '')

    if os.execute("bash -c \"if [ -d \\\"" .. directory .. "\\\" -a -w \\\"" ..
               directory ..  "\\\" ] ; then exit 0 ; else exit 1 ; fi ;\"") then
      return directory .. '/'
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

-- Routine to sanitize a prefix string
--
-- Currently only strips all path separators ('/') and assures non empty,
-- although we may consider dropping the nonempty requirement (allow for
-- screenshots to  be named 'date_time.png')
local function check_prefix(prefix)
  -- Maybe add more sanitizing eventually
  if prefix and type(prefix) == "string" then
    prefix = string.gsub(prefix, '/', '')
    if string.len(prefix) ~= 0 then
      return prefix
    end
  end
  return get_default_prefix()
end

-- Routine to initialize the screenshot object
--
-- Currently only sets the screenshot directory, the filename, prefix and the
-- snipper tool outline color. More initialization can be added as the API
-- expands.
-- @function screenshot.init
-- @tparam string The directory path in which to save screenshots
-- @tparam string The prefix prepended to the screenshot filenames
-- @return true or nil depending on success
function screenshot.init(directory, prefix, color)
  local tmp

  tmp = check_directory(directory)

  if tmp then
    ss_dir = tmp
  else
    initialized = nil
    return nil
  end

  tmp = check_prefix(prefix)

  if tmp then
    ss_prfx = tmp
  else
    -- Should be unreachable as the default will always be taken
    initialized = nil
    return nil
  end

  -- Don't throw out prior init data if only color is misformed
  initialized = true

  if color then
    tmp = gears.color(color)
    if tmp then
      frame_color = tmp
    end
  end

  return true
end

-- Routine to configure the directory/prefix pair for any call to the
-- screenshot API.
--
-- This supports using a different directory or filename prefix for a particular
-- use by passing the optional arguments. In the case of an initalized object
-- with  no arguments passed, the stored configuration parameters are quickly
-- returned. This also technically allows the user to take a screenshot in an
-- unitialized state and without passing any arguments.
local function configure_call(directory, prefix)

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

-- Internal routine to do the actual work of taking a cropped screenshot
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

-- Routine used by the snipper mousegrabber to update the frame outline of the
-- current state of the cropped screenshot
--
-- This function is largely a copy of awful.mouse.snap.show_placeholder(geo),
-- so it is probably a good idea to create a utility routine to handle both use
-- cases. It did not seem appropriate to make that change as a part of the pull
-- request that was creating the screenshot API, as it would clutter and
-- confuse the change.
local function show_frame(geo)

  if not geo then
      if frame then
          frame.visible = false
      end
      return
  end

  frame = frame or wibox {
      ontop = true,
      bg    = frame_color
  }

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
  gears.shape.partially_rounded_rect(cr,geo.width-2*line_width,geo.height-2*line_width, false, false, false, false, nil)
  
  cr:stroke()

  frame.shape_bounding = img._native
  img:finish()

  frame.visible = true

end

-- Internal routine functioning as the callback for implementing the snipper.
--
-- The snipper tool is basically a mousegrabber, which takes a single function
-- of one argument, representing the mouse state data. This is a simple
-- starting point that hard codes the snipper tool as being a two press design
-- using button 1, with button 3 functioning as the cancel. These aspects of
-- the interface can be made into parameters at some point (and also
-- incorporated into the init() configuration routine).
local function mg_callback(mouse_data)
  if mouse_data["buttons"][3] then
    if frame and frame.visible then
      show_frame()
    end
    return false
  end

  if mg_first_pnt[1] then

    local min_x, max_x, min_y, max_y
    min_x = math.min(mg_first_pnt[1], mouse_data["x"])
    max_x = math.max(mg_first_pnt[1], mouse_data["x"])
    min_y = math.min(mg_first_pnt[2], mouse_data["y"])
    max_y = math.max(mg_first_pnt[2], mouse_data["y"])

    -- Force a minimum size to the box
    if  max_x - min_x < 4 or max_y - min_y < 4 then
      if frame and frame.visible then
        show_frame()
      end
    elseif not mouse_data["buttons"][1] then
      show_frame({x = min_x, y = min_y, width = max_x - min_x, height = max_y - min_y})
    end

    if mouse_data["buttons"][1] then

      local dir, prfx
      local date_time, fname
      local snip_surf

      if frame and frame.visible then
        show_frame()
      end

      frame = nil
      mg_first_pnt = {}

      -- This may fail gracefully anyway but require a minimum 2x2 of pixels
      if min_x >= max_x-1 or min_y >= max_y-1 then
        return false
      end

      dir, prfx = configure_call(mg_dir, mg_prfx)
      snip_surf = crop_shot(min_x, min_y, max_x - min_x, max_y - min_y)

      if dir and prfx and snip_surf then
        date_time = tostring(os.date("%Y%m%d%H%M%S"))
        fname = dir .. prfx .. date_time .. ".png"

        gears.surface(snip_surf):write_to_png(fname)

        if mg_onsuccess_cb then
          mg_onsuccess_cb(fname)  -- This should probably be a separate thread
        end
      end

      mg_onsuccess_cb = nil       -- Revert callback unconditionally
      return false

    end

  else
    if mouse_data["buttons"][1] then
      mg_first_pnt[1] = mouse_data["x"]
      mg_first_pnt[2] = mouse_data["y"]
    end
  end

  return true
end

-- Take root window screenshots
--
-- @function screenshot.root
-- @tparam[opt] string The directory path in which to save screenshots
-- @tparam[opt] string The prefix prepended to the screenshot filenames
-- @treturn string The full path to the successfully written screenshot file
function screenshot.root(directory, prefix)
  local dir, prfx
  local date_time, fname

  dir, prfx = configure_call(directory, prefix)

  if dir and prfx then
    date_time = tostring(os.date("%Y%m%d%H%M%S"))
    fname = dir .. prfx .. date_time .. ".png"
    gears.surface(capi.root.content()):write_to_png(fname)
    return fname
  end
end

-- Take physical screen screenshots
--
-- @function screenshot.screen
-- @tparam[opt] string The directory path in which to save screenshots
-- @tparam[opt] string The prefix prepended to the screenshot filenames
-- @tparam[opt] number or screen The index of the screen, or the screen itself
-- @treturn string The full path to the successfully written screenshot file
function screenshot.screen(directory, prefix, target)
  local s
  local dir, prfx
  local date_time, fname

  if target then
    if type(target) == "number" then
      s = capi.screen[target] and aw_screen.focused()
    elseif target.index and type(target.index) == "number" then
      s = capi.screen[target.index] and aw_screen.focused()
    end
  else
    s = aw_screen.focused()
  end

  dir, prfx = configure_call(directory, prefix)

  if s and dir and prfx then
    date_time = tostring(os.date("%Y%m%d%H%M%S"))
    fname = dir .. prfx .. date_time .. ".png"
    gears.surface(s.content):write_to_png(fname)
    return fname
  end
end

-- Take client window screenshots
--
-- @function screenshot.client
-- @tparam[opt] string The directory path in which to save screenshots
-- @tparam[opt] string The prefix prepended to the screenshot filenames
-- @treturn string The full path to the successfully written screenshot file
function screenshot.client(directory, prefix)
  -- Looking at the properties and functions available, I'm not sure it is
  -- wise to allow a "target" client argument, but if we want to add it as
  -- arg 3 (which will match the screen ordering), we can.

  local c
  local dir, prfx
  local date_time, fname

  dir, prfx = configure_call(directory, prefix)
  c = capi.client.focus

  if c and dir and prfx then
    date_time = tostring(os.date("%Y%m%d%H%M%S"))
    fname = dir .. prfx .. date_time .. ".png"
    gears.surface(c.content):write_to_png(fname)
    return fname
  end
end

-- Launch an interactive snipper tool to take cropped shots
--
-- @function screenshot.snipper
-- @tparam[opt] string The directory path in which to save screenshots
-- @tparam[opt] string The prefix prepended to the screenshot filenames
-- @tparam[opt] function A callback to be run upon successful writing of the
--                       screenshot file with a single argument of the path to
--                       the screenshot file.
function screenshot.snipper(directory, prefix, onsuccess_cb)
  local dir, prfx
  local date_time

  dir, prfx = configure_call(directory, prefix)

  if dir and prfx then

    mg_dir = dir
    mg_prfx = prfx

    capi.mousegrabber.run(mg_callback, "crosshair")

    if onsuccess_cb and type(onsuccess_cb) == "function" then
      mg_onsuccess_cb = onsuccess_cb
    end

    return

  end  
end

-- Take a cropped screenshot of a defined geometry
--
-- @function screenshot.snip
-- @tparam geometry The geometry of the cropped screenshot
-- @tparam[opt] string The directory path in which to save screenshots
-- @tparam[opt] string The prefix prepended to the screenshot filenames
-- @treturn string The full path to the successfully written screenshot file
function screenshot.snip(geom, directory, prefix)
  local dir, prfx
  local root_w, root_h
  local root_intrsct
  local date_time, fname
  local snip_surf

  root_w, root_h = root.size()
  dir, prfx = configure_call(directory, prefix)

  if dir and prfx and geom and
       geom.x and geom.y and geom.width and geom.height then

    if type(geom.x) == "number" and type(geom.y) == "number" and
         type(geom.width) == "number" and type(geom.height) == "number" then

      root_intrsct = gears.geometry.rectangle.get_intersection(geom,
                                               {x = 0, y = 0,
                                               width = root_w, height = root_h})
      snip_surf = crop_shot(root_intrsct.x, root_intrsct.y,
                            root_intrsct.width, root_intrsct.height)

      date_time = tostring(os.date("%Y%m%d%H%M%S"))
      fname = dir .. prfx .. date_time .. ".png"
      gears.surface(snip_surf):write_to_png(fname)

      return fname

    end

  end 
end

return screenshot
