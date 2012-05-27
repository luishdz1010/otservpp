local task
task = callAfter(1, function() error() end)
assert(task:cancel())

task = callAfter(20, function() 
    assert(not task:cancel()) -- executed tasks can't be canceled
end)

local functor = {}
setmetatable(functor, {__call=function() end})
callAfter(1, functor)

local count, interval = 1
interval = callEvery(3, function()
	if count == 10 then
		 -- interval tasks can and should be canceled within the task body
		return assert(interval:cancel())
	end
	count = count+1
end)

-- volalite interval precision 
callAfter(37, function()
	assert(count == 10)
end)

callEvery(1, function() error() end):cancel()

-- triangular number generation
local MAX, verified = 25, {0} 
for v=1, MAX do
	verified[v+1] = v*(v+1)/2
end

local n, i, parent = {}, 1
parent = callEvery(1, function() 
	-- stop the schedule so we get dispatched after our childs
	parent:cancel()
	
	local child
	child = callEvery(1, function() 
		if #n > MAX then			
			assert(child:cancel())
			
			-- this will be false for every child but one
			if parent:cancel() then
				for i,v in ipairs(verified) do
					assert(n[i] == v)
				end
			end
		else
			i = i+1		
		end				
	end)
	
	n[#n+1] = i-1
	
	-- reenable	
	parent:reschedule()
end)