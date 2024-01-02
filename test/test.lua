local json = require "test.json"
local ffi = require "ffi"

local bor, band, bxor, lshift, rshift = bit.bor, bit.band, bit.bxor, bit.lshift, bit.rshift

local lib = ffi.load("lib/libhappines.so")

local function load_header(name)
  local file = io.open("include/" .. name .. ".h", "r")
  local data = ""
  for line in file:lines() do
    if not line:match("^#") then
      data = data .. line .. "\n"
    end
  end
  return data
end

local function def_header(name)
  local data = load_header(name)
  ffi.cdef(data)
end

local function load_cpu()
  local cpu = ffi.new("CPU")
  local bus = ffi.new("Bus")

  lib.cpu_init(cpu, bus)

  return cpu
end

local function load_test()
  local testsuite = arg[1]
  local file = io.open(testsuite, "r")
  local data = file:read "*all"
  file:close()
  return json.decode(data)
end

local function run_test(test, cpu)
  for _, v in ipairs(test.initial.ram) do
    address, value = unpack(v)
    address, value = tonumber(address), tonumber(value)
    lib.cpu_bus_write(cpu, address, value)
  end

  -- set registers
  cpu.pc = test.initial.pc
  cpu.status = test.initial.p
  cpu.a = test.initial.a
  cpu.x = test.initial.x
  cpu.y = test.initial.y
  cpu.sp = test.initial.s

  lib.cpu_step(cpu)

  local function assert_register(name, target)
    local expected = tonumber(test.final[target])
    local actual = cpu[name]

    local message = [[

    TEST FAILED
      test: %s
      register: %s
      expected: 0x%04x
      actual: 0x%04x
    ]]

    assert(actual == expected, message:format(test.name, name, expected, actual))
  end

  local function assert_flag(offset, name)
    local flags = tonumber(test.final.p)
    local expected = band(flags, lshift(1, offset))
    local actual = band(cpu.status, lshift(1, offset))

    local message = [[

    TEST FAILED
      test: %s
      flag: %s
      expected: 0x%04x
      actual: 0x%04x
    ]]

    assert(actual == expected, message:format(test.name, name, expected, actual))
  end

  -- check registers
  assert_register("pc", "pc")
  assert_register("a", "a")
  assert_register("x", "x")
  assert_register("y", "y")
  assert_register("sp", "s")

  -- check flags
  assert_flag(0, "carry")
  assert_flag(1, "zero")
  assert_flag(2, "interrupt")
  assert_flag(4, "break")
  assert_flag(6, "overflow")
  assert_flag(7, "negative")

  -- check memory
  for _, v in ipairs(test.final.ram) do
    address, value = unpack(v)
    address, value = tonumber(address), tonumber(value)

    local actual = lib.cpu_bus_read(cpu, address, false)

    local message = [[

    TEST FAILED
      test: %s
      address: 0x%04x
      expected: 0x%04x
      actual: 0x%04x
    ]]

    assert(actual == value, message:format(test.name, address, value, actual))
  end
end

local function run_tests(tests, cpu)
  local errors = 0
  for i = 1, 10000 do
    local test = tests[i]

    local ok, err = pcall(run_test, test, cpu)

    if not ok then
      print(err)
      errors = errors + 1
    end

    if errors > 100 then
      print("stopping, too many errors")
      break
    end
  end

  print(string.format("Passing: %d/%d", 10000 - errors, 10000))
end

local function main()
  def_header("bus")
  def_header("cpu")

  local tests = load_test()
  local cpu = load_cpu()
  run_tests(tests, cpu)
end

main()
