# minizinc-julia
A Julia function that solves minizinc models.

The minizinc code and data are to be passed as strings.

# Compiling the C wrapper
clang++ mzn2fzn.cpp -std=c++11 -lgecodeflatzinc -lgecodedriver -lgecodegist -lgecodesearch -lgecodeminimodel\
-lgecodeset -lgecodefloat -lgecodeint -lgecodekernel -lgecodesupport -L/usr/lib/x86_64-linux-gnu -lQtGui -lQtCore\
-lpthread -o mznmem  /usr/local/lib/libminizinc.a
