# minizinc-julia
A Julia function that solves minizinc models.

The minizinc code and data are to be passed as strings.

# Compiling the C wrapper
clang++ mznmem.cpp -std=c++11 -lgecodeflatzinc -lgecodedriver -lgecodegist -lgecodesearch -lgecodeminimodel\
-lgecodeset -lgecodefloat -lgecodeint -lgecodekernel -lgecodesupport -L/usr/lib/x86_64-linux-gnu -lQtGui -lQtCore\
-lpthread -o mznmem  /usr/local/lib/libminizinc.a

# Usage
```
julia> s =

       """% Colouring Australia using nc colours
       int: nc = 3;

       var 1..nc: wa;   var 1..nc: nt;  var 1..nc: sa;   var 1..nc: q;
       var 1..nc: nsw;  var 1..nc: v;   var 1..nc: t;

       constraint wa != nt;
       constraint wa != sa;
       constraint nt != sa;
       constraint nt != q;
       constraint sa != q;
       constraint sa != nsw;
       constraint sa != v;
       constraint q != nsw;
       constraint nsw != v;
       solve satisfy;

       """
julia> include("/media/disk2/juliawrapper(minizinc)-obsolete/mznmem.jl")
mznmem
julia> mznmem.solve(s)
9-element Array{SubString{UTF8String},1}:
 "nsw = 2;"  
 "nt = 2;"   
 "q = 3;"    
 "sa = 1;"   
 "t = 1;"    
 "v = 3;"    
 "wa = 3;"   
 "----------"
 ""     
 ```
