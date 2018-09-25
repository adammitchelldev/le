require("lua/find")

le.status("Hello from lua")

function old_key(code)
  le.status("Code: " .. code)
  if code == 25 then
    local lastString = ""
    local lastK = 0
    local n = 0
    local name = le.prompt("What is your name? %s", function(s, k)
        n = n + 1
        lastK = k
        lastString = s
    end)
    le.status("Hi " .. name .. "! n:"..n.." k:"..lastK.." s:"..lastString);
  end
end
