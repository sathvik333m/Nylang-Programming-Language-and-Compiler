func add(a,b)
result=a+b
return result
endfunc

func classify(value)
level=0
if(value>=25)
level=3
elseif(value>=15)
level=2
else
level=1
endif
return level
endfunc

func show_separator()
println("--------------------")
endfunc

println("Nylang Integrated Demo")
show_separator()
print("Mode: ")
println("FULL DEMO")
println("Enter your name")
read(username)
println("Welcome")
println(username)
println("Enter two integers")
a=read()
b=read()


sum=add(a,b)

difference=a-b

product=a*b

quotient=a/b
remainder=a%b
negValue=-a
logicFlag=(a>b and b>0) or not(a==b)

println("Arithmetic results")
print("Sum:")
println(sum)
print("Difference:")
println(difference)
print("Product:")
println(product)
print("Quotient:")
println(quotient)
print("Remainder:")
println(remainder)
println(negValue)
println(logicFlag)

array nums[5]
for(i=0; i<5; i=i+1)
nums[i]=i+sum
endfor

println("Array contents")
for(i=0; i<5; i=i+1)
println(nums[i])
endfor

filteredTotal=0
for(i=0; i<5; i=i+1)
if(i==1)
continue
endif
filteredTotal=filteredTotal+nums[i]
if(filteredTotal>60)
break
endif
endfor

println("Filtered total")
println(filteredTotal)

level=classify(filteredTotal)
println("Classification")
if(level==3)
println("HIGH")
elseif(level==2)
println("MEDIUM")
else
println("LOW")
endif

msg="Compilation complete"
println(msg)
show_separator()
