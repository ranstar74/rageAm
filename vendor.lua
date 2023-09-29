-- Includes system directories & files
function system_files(name)
	return function(f)
		externalincludedirs { name }
	
		local t = {}
		for k, file in ipairs(f) do
			table.insert(t, name .. file)
		end
		files(t)
	end
end

-- Includes vendor directories & files
function vendor_files(name)
	return function(f)
		includedirs { name }
	
		local t = {}
		for k, file in ipairs(f) do
			table.insert(t, name .. file)
		end
		files(t)
	end
end

function include_vendor(name)
	dofile("projects/app/vendor/" .. name .. ".lua")
end

function include_vendors(names)
	for k, name in pairs(names) do
		include_vendor(name)
	end
end
