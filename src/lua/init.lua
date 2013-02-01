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


_G.make_patch = function(a, b)
  local patches = dmp.patch_make(a, b)
  return dmp.patch_toText(patches)
end
