local core = require "core"
local command = require "core.command"
local keymap = require "core.keymap"
local console = require "plugins.console"

command.add(nil, {
  ["project:build-project"] = function()
    core.log "Building..."
    console.run {
      command = "wbuild",
      file_pattern = "(.*):(%d+):(%d+): (.*)$",
      on_complete = function() core.log "Build complete" end,
    }
  end
})

command.add(nil, {
  ["project:run-project"] = function()
    core.log "Running..."
    console.run {
      command = ".\\build\\host.exe",
      file_pattern = "(.*):(%d+):(%d+): (.*)$",
      on_complete = function() core.log "Run complete" end,
    }
  end
})

keymap.add { ["ctrl+b"] = "project:build-project" }
keymap.add { ["ctrl+e"] = "project:run-project" }
