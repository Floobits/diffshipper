dmp = require('src/lua/diff_match_patch')

_G.apply_patch = function(buf_text, patch_text)
  local patches = dmp.patch_fromText(patch_text)
  local text, results = dmp.patch_apply(patches, buf_text)
  local clean_patch = true

  for k,v in ipairs(results) do
    if v == false then
      clean_patch = false
      break
    end
  end

  return clean_patch, text
end

-- print(apply_patch([[dmp = require('diff_match_patch')

-- _G.apply_patch = function(buf_text, patch_text)
--   local patches = dmp.patch_fromText(patch_text)
--   local text, results = dmp.patch_apply(patches, buf_text)
--   local clean_patch = true

--   for k,v in ipairs(results) do
--     if v == false then
--       clean_patch = false
--       break
--     end
--   end

--   return clean_patch, text
-- end

-- apply_patch("123", "e")"]], "@@ -376,11 +376,9 @@\n %22, %22\n-1x3\n+e\n %22)%0A%0A\n"))
