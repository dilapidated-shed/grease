-- Actions remain opaque C-owned data. Lua chooses their execution order.
local composition ← {}

composition.sequence ← λ(interpreter, actions)
  assert(type(actions) = "table", "actions must form a dense list")
  local length ← #actions
  local count ← 0
  for index, _ in pairs(actions) do
    assert(type(index) = "number" and index >= 1 and index <= length and index % 1 = 0,
           "actions must form a dense list")
    count ← count + 1
  end
  assert(count = length, "actions must form a dense list")
  local result ← { ok ← true }
  for index ← 1, length do
    result ← native.evaluate(interpreter, actions[index])
    if not result.ok then
      return result
    end
  end
  return result
end

return composition
