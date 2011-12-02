
-- Dummy file used by Knights Map Editor

function Graphic(name, r, g, b, x, y)
  if (r==nil) then return {name, kvalue_tag="g"}
  elseif x==nil then return {name, r, g, b, kvalue_tag="g"}
  else return {name, r, g, b, x, y, kvalue_tag="g"}
  end
end

function Sound()
  return nil
end

function kconfig_itemtype()
  return nil
end

function kconfig_tile()
  return nil
end

function kconfig_control()
  return nil
end

function user_table()
  return {}
end
