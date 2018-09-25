function key(code)
  if code == 7 then
    local search = le.prompt("Lua Find: %s", function(s)
      le.go(le.find(s))
    end)
    return 1
  end
  return 0
end
