func square(n)
return n*n
endfunc

println("Small Nylang Demo")

x=4
y=square(x)
println(y)

if(y>10)
println("big")
else
println("small")
endif

array nums[3]
for(i=0; i<3; i=i+1)
nums[i]=i+y
endfor

i=0
while(i<3)
println(nums[i])
i=i+1
endwhile
