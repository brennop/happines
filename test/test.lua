local json = require "test.json"
local ffi = require "ffi"

local bor, band, bxor, lshift, rshift = bit.bor, bit.band, bit.bxor, bit.lshift, bit.rshift

local lib = ffi.load("lib/libhappines.so")

local n_tests = tonumber(arg[2]) or 100
local opcode

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
  local mapper = ffi.new("Mapper")

  lib.mapper_init(mapper, 0xffffffff)
  lib.bus_init(bus, mapper, nil)
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
  local mapper = cpu.bus.mapper

  for _, v in ipairs(test.initial.ram) do
    address, value = unpack(v)
    address, value = tonumber(address), tonumber(value)
    mapper.ram[address] = value
  end

  -- set registers
  cpu.pc = test.initial.pc
  cpu.status = test.initial.p
  cpu.a = test.initial.a
  cpu.x = test.initial.x
  cpu.y = test.initial.y
  cpu.sp = test.initial.s

  local cycles = lib.cpu_step(cpu)

  local function assert_register(name, target)
    local expected = tonumber(test.final[target])
    local actual = cpu[name]

    local message = [[

    TEST FAILED
      test: %s (%s)
      register: %s
      expected: 0x%04x
      actual: 0x%04x
    ]]

    assert(actual == expected, message:format(test.name, opcode, name, expected, actual))
  end

  local function assert_flag(offset, name)
    local flags = tonumber(test.final.p)
    local expected = band(flags, lshift(1, offset))
    local actual = band(cpu.status, lshift(1, offset))

    local message = [[

    TEST FAILED
      test: %s (%s)
      flag: %s
      expected: 0x%04x
      actual: 0x%04x
    ]]

    assert(actual == expected, message:format(test.name, opcode, name, expected, actual))
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

    local actual = mapper.ram[address]

    local message = [[

    TEST FAILED
      test: %s (%s)
      address: 0x%04x
      expected: 0x%04x
      actual: 0x%04x
    ]]

    assert(actual == value, message:format(test.name, opcode, address, value, actual))
  end

  local message = [[
  TEST FAILED
    test: %s (%s)
    innacurate cycles
    expected: %d
    actual: %d
  ]]

  -- check cycles
  expected_cycles = #test.cycles
  assert(expected_cycles == cycles, message:format(test.name, opcode, expected_cycles, cycles))
end

local function run_tests(tests, cpu)
  local errors = 0
  for i = 1, n_tests do
    local test = tests[i]

    run_test(test, cpu)
  end

  print(string.format("Passing: %d/%d", n_tests - errors, n_tests))
end

local function main()
  def_header("mapper")
  def_header("bus")
  def_header("cpu")

  local filename = arg[1]
  opcode = filename:match("([%x]+).json")
  local is_legal = lib.is_opcode_legal(tonumber(opcode, 16))

  if not is_legal then
    print("illegal opcode " .. opcode)
    return
  end

  local tests = load_test()
  local cpu = load_cpu()
  run_tests(tests, cpu)
end

main()
