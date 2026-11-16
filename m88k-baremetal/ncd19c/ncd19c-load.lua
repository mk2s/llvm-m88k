-- MAME autoboot script for ncd19c bare-metal runs. Configured through environment variables:
--   NCD_MODE inject|netboot, NCD_IMAGE (COFF path), NCD_WAIT (emulated s), NCD_TIMEOUT (emulated s
--   after load before giving up), NCD_SHOT (screenshot PNG path), NCD_EOT_FLAG (file the runner
--   creates when it has seen EOT on the serial line).
local m = manager.machine
local cpu = m.devices[":cpu"]
local dsp = cpu.spaces["data"]
local MODE = os.getenv("NCD_MODE") or "inject"
local IMG = os.getenv("NCD_IMAGE")
local WAIT = tonumber(os.getenv("NCD_WAIT") or "20")
local TIMEOUT = tonumber(os.getenv("NCD_TIMEOUT") or "120")
local SHOT = os.getenv("NCD_SHOT")
local EOT = os.getenv("NCD_EOT_FLAG")

local function u16(s, o) return s:byte(o + 1) * 256 + s:byte(o + 2) end
local function u32(s, o) return u16(s, o) * 65536 + u16(s, o + 2) end

local function exists(p)
  local f = io.open(p, "rb")
  if f then f:close() return true end
  return false
end

local function shot()
  if SHOT and SHOT ~= "" then m.screens[":screen"]:snapshot(SHOT) end
end

-- returns entry, text_lo, text_hi
local function load_image()
  local f = assert(io.open(IMG, "rb"))
  local s = f:read("a")
  f:close()
  assert(u16(s, 0) == 0x016d, "bad COFF magic")
  local nscns, opt = u16(s, 2), u16(s, 16)
  local entry = u32(s, 20 + 16)
  local tlo, thi, dlo
  for i = 0, nscns - 1 do
    local h = 20 + opt + i * 44
    local name = s:sub(h + 1, h + 8):gsub("%z", "")
    local vaddr, size, ptr = u32(s, h + 12), u32(s, h + 16), u32(s, h + 20)
    if name == ".text" then tlo, thi = vaddr, vaddr + size end
    if name == ".data" then dlo = vaddr end
    for o = 0, size - 1 do
      dsp:write_u8(vaddr + o, (name ~= ".bss" and ptr > 0) and s:byte(ptr + o + 1) or 0)
    end
    print(string.format("NCD section %s addr=%08x size=%x", name, vaddr, size))
  end
  return entry, tlo, thi, dlo
end

local state, t_state, tries = 0, 0, 0
local entry, tlo, thi, dlo
local function xip() return cpu.state["XIP"].value end

local function done(msg)
  print(string.format("NCD %s (XIP=%08x r31=%08x)", msg, xip(), cpu.state["r31"].value))
  shot()
  m:exit()
end

emu.register_periodic(function()
  local t = m.time:as_double()
  if state == 0 then
    if MODE == "netboot" then
      state, t_state = 2, t
    elseif t > WAIT then
      local x = xip()
      if x >= 0x0400b240 and x < 0x0400bb00 then
        entry, tlo, thi, dlo = load_image()
        cpu.state["r1"].value = 0x0400a4e4
        cpu.state["FIP"].value = entry
        tries, state, t_state = 1, 1, t
        print(string.format("NCD jumped (XIP was %08x)", x))
      elseif t > WAIT + 30 then
        done("FAIL: monitor idle loop not reached")
      end
    end
  elseif state == 1 then
    -- crt0 stores the monitor r1 (0x0400a4e4) in the first .data word right away.
    local x = xip()
    if (x >= tlo and x < thi) or dsp:read_u32(dlo) == 0x0400a4e4 then
      state, t_state = 2, t
    elseif tries >= 5 then
      done("FAIL: FIP redirect did not take effect")
    else
      cpu.state["FIP"].value = entry
      tries = tries + 1
    end
  elseif state == 2 then
    if EOT and exists(EOT) then
      state, t_state = 3, t
    elseif t - t_state > TIMEOUT then
      done("FAIL: timeout without EOT")
    end
  elseif state == 3 and t - t_state > 1 then
    done("done")
  end
end)
