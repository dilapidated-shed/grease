assert(debug = nil and io = nil and package = nil)
local interpreter ← native.new_interpreter()
local cwd ← native.working_directory(interpreter)
local directory ← native.evaluate(interpreter,
  native.open_at_action(interpreter, cwd, arg[1], {"read-only", "directory"}))
assert(directory.ok and directory.directory)
local flags ← {"read-write", "create", "exclusive"}
local planned ← native.open_at_action(interpreter, directory.value, "entry", flags, 384)
assert(not observer.exists(arg[1] .. "/entry"), "action construction performed an effect")
flags[1] ← "invented-flag"
local created ← composition.sequence(interpreter, {planned})
assert(created.ok and observer.exists(arg[1] .. "/entry"), "flags were not retained as values")
print("created\t" .. tostring(created.ok))
local duplicate ← native.evaluate(interpreter,
  native.open_at_action(interpreter, directory.value, "entry", {"read-write", "create", "exclusive"}, 384))
assert(not duplicate.ok)
print("exclusive_error\t" .. tostring(duplicate.error.number))
local closed ← native.evaluate(interpreter, native.close_action(interpreter, created.value))
print("closed\t" .. tostring(closed.ok))
local reopened ← native.evaluate(interpreter,
  native.open_at_action(interpreter, directory.value, "entry"))
assert(reopened.ok)
print("reopened\t" .. tostring(reopened.ok))
local stale ← native.evaluate(interpreter, native.close_action(interpreter, created.value))
assert(not stale.ok and stale.error.kind = "invalid_handle", "stale handle revived")
local reclosed ← native.evaluate(interpreter, native.close_action(interpreter, reopened.value))
print("reclosed\t" .. tostring(reclosed.ok))
local missing ← native.evaluate(interpreter,
  native.open_at_action(interpreter, directory.value, "missing"))
assert(not missing.ok)
print("missing_error\t" .. tostring(missing.error.number))
local removed ← native.evaluate(interpreter,
  native.unlink_at_action(interpreter, directory.value, "entry"))
assert(removed.ok and not observer.exists(arg[1] .. "/entry"))
print("removed\t" .. tostring(removed.ok))
local absent ← native.evaluate(interpreter,
  native.unlink_at_action(interpreter, directory.value, "entry"))
assert(not absent.ok)
print("second_unlink_error\t" .. tostring(absent.error.number))

local borrowed ← native.evaluate(interpreter, native.close_action(interpreter, cwd))
assert(not borrowed.ok and borrowed.error.kind = "borrowed_handle")
assert(type(created.value) = "userdata" and getmetatable(created.value) = false)
assert(not pcall(λ() created.value.token ← 1 end), "handle was forgeable")
assert(not pcall(native.close_action, interpreter, 1), "integer was admitted as a handle")
assert(not pcall(native.open_at_action, interpreter, cwd, "bad\0path"), "zero byte truncated a path")
assert(not pcall(native.open_at_action, interpreter, cwd, "entry", nil, "384"), "text was admitted as a mode")
assert(not pcall(native.open_at_action, interpreter, cwd, "entry", {"read-only", extra ← "create"}), "named flag entry was ignored")
local other ← native.new_interpreter()
assert(not pcall(native.close_action, other, created.value), "foreign handle admitted")
assert(not pcall(native.evaluate, other, planned), "foreign action admitted")
local rejected ← composition.sequence(interpreter, {
  native.open_at_action(interpreter, directory.value, "missing", {"read-only", "write-only"}),
  native.open_at_action(interpreter, directory.value, "never", {"read-write", "create"}, 384)
})
assert(not rejected.ok and rejected.error.kind = "bad_argument", "sequence did not retain first failure")
assert(not observer.exists(arg[1] .. "/never"), "sequence continued after failure")
assert(not pcall(composition.sequence, interpreter, {[2] ← planned}), "sparse actions were silently skipped")

local directory_closed ← native.evaluate(interpreter,
  native.close_action(interpreter, directory.value))
print("directory_closed\t" .. tostring(directory_closed.ok))

collectgarbage("collect")
local before ← observer.descriptor_count()
do
  local temporary ← native.new_interpreter()
  local borrowed_cwd ← native.working_directory(temporary)
  local live_directory ← native.evaluate(temporary,
    native.open_at_action(temporary, borrowed_cwd, arg[1], {"read-only", "directory"}))
  assert(live_directory.ok)
  temporary ← nil
  borrowed_cwd ← nil
  collectgarbage("collect")
  assert(observer.descriptor_count() = before + 1, "handle failed to retain its interpreter")
  live_directory ← nil
end
collectgarbage("collect")
collectgarbage("collect")
assert(observer.descriptor_count() = before, "interpreter collection leaked a descriptor")
