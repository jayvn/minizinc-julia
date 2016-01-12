module mznmem
using Glob

type RetString
    length::Csize_t
    content::Ptr{UInt8}
end

function readalldzn()
    ret = ""
    for f in glob("*.dzn")
        tmp = strip(readall(f))
        # Replace EOF with ';' when appending .dzn files together
        tmp[end] == ';' ? ret = string(ret, tmp) : ret = string(ret, tmp, ";\n")
    end
    ret
end

"""Read the minizinc code from a file without any seperate data"""
solvefile(filename::AbstractString, numsolutions = 1, verbose = false) =
    solve(readall(filename), numsolutions, verbose)

"""Read the minizinc code from a file and pass the data"""
function solvefile(filename::AbstractString, data::AbstractString, numsolutions = 1,
        verbose = false)
    tmp = strip(readall(filename))
    # Replace EOF with ';' when appending files together.
    # FIXME: Fails if last char is part of a comment
    tmp[end] == ';' ? tmp = string(tmp, data) : tmp = string(tmp, ";\n", data)
    solve(tmp, numsolutions, verbose)
end

"""Read the minizinc code and data seperately"""
function solve(mzn::AbstractString, data::AbstractString, numsolutions = 1, verbose = false)
    mzn[end] == ';' ? mzn = string(mzn, data) : mzn = string(mzn, ";\n", data)
    solve(mzn, numsolutions, verbose)
end

"""Read the minizinc code and solve it"""
function solve(data::AbstractString,  numsolutions = 1, verbose = false)
    ret = ccall((:solve, "libmznmem.so"), RetString, (Ptr{UInt8}, Cint, Bool, Ptr{UInt8}), data,
        numsolutions, verbose, std_lib_dir)
    str = utf8(pointer_to_array(ret.content, ret.length))
    ccall((:release_retstring, "libmznmem.so"), Void, (RetString, ), ret)
    str
end

function __init__()
    global std_lib_dir
    try
        std_lib_dir = ENV["MZN_STDLIB_DIR"]
        isfile(joinpath(std_lib_dir, "std/builtins.mzn")) || throw()
    catch
        mypath = "/usr/local"
        if isfile(joinpath(mypath, "share/minizinc/std/builtins.mzn"))
            std_lib_dir = joinpath(mypath,  "share/minizinc")
        elseif isfile(string(mypath, "../share/minizinc/std/builtins.mzn"))
            std_lib_dir = joinpath(mypath, "../share/minizinc")
        else
            throw("Error: unknown minizinc standard library directory. Set the MZN_STDLIB_DIR
                environment variable.")
        end
    end

end

end
