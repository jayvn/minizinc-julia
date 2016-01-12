#ifdef _MSC_VER
#define _CRT_SECURE_NO_WARNINGS
#endif

#include <gecode/flatzinc.hh>

#include <iostream>
#include <fstream>
#include <iomanip>
#include <cerrno>

#include <minizinc/model.hh>
#include <minizinc/parser.hh>
#include <minizinc/prettyprinter.hh>
#include <minizinc/typecheck.hh>
#include <minizinc/astexception.hh>

#include <minizinc/flatten.hh>
#include <minizinc/optimize.hh>
#include <minizinc/builtins.hh>
#include <minizinc/file_utils.hh>
#include <minizinc/timer.hh>

using namespace MiniZinc;
using namespace std;

string stoptime(Timer &start) {
    ostringstream oss;
    oss << setprecision(0) << fixed << start.ms() << " ms";
    start.reset();
    return oss.str();
}

bool beginswith(string s, string t) {
    return s.compare(0, t.length(), t) == 0;
}

int flattenmem(string in, stringstream &outstream, bool flag_verbose, string include)
{
    vector<string> includePaths;

    bool flag_ignoreStdlib = false;
    bool flag_optimize = true;
    bool flag_werror = false;

    Timer starttime;
    Timer lasttime;

    bool flag_instance_check_only = false;
    FlatteningOptions fopts;

    includePaths.push_back(include + "/std/");

    for (unsigned int i = 0; i < includePaths.size(); i++) {
        if (!FileUtils::directory_exists(includePaths[i])) {
            cerr << "Cannot access include directory " << includePaths[i] << "\n";
            return 1;
        }
    }

    stringstream errstream;
    if (flag_verbose) {
        cerr << "Parsing ..." << endl;
    }
    Model *m;

    m = parseFromString(in, "mznfile", includePaths, flag_ignoreStdlib, false, flag_verbose,
                        errstream);

    if (m) {
        try {
            Env env(m);
            if (flag_verbose) {
                cerr << "Done parsing (" << stoptime(lasttime) << ")" << endl << "Typechecking ...";
            }
            vector<TypeError> typeErrors;
            MiniZinc::typecheck(env, m, typeErrors);
            if (typeErrors.size() > 0) {
                for (unsigned int i = 0; i < typeErrors.size(); i++) {
                    cerr << endl;
                    cerr << typeErrors[i].loc() << ":" << endl;
                    cerr << typeErrors[i].what() << ": " << typeErrors[i].msg() << endl;
                }
                return 1;
            }
            MiniZinc::registerBuiltins(env, m);
            if (flag_verbose) {
                cerr << " done (" << stoptime(lasttime) << ")" << endl;
            }

            if (!flag_instance_check_only) {
                if (flag_verbose) {
                    cerr << "Flattening ...";
                }
                try {
                    flatten(env, fopts);
                } catch (LocationException &e) {
                    cerr << endl;
                    cerr << e.what() << ": " << endl;
                    env.dumpErrorStack(cerr);
                    cerr << "  " << e.msg() << endl;
                    return 1;
                }
                for (unsigned int i = 0; i < env.warnings().size(); i++) {
                    cerr << (flag_werror ? "Error: " : "Warning: ") << env.warnings()[i];
                }
                if (flag_werror && env.warnings().size() > 0) {
                    return 1;
                }
                env.clearWarnings();
                Model *flat = env.flat();

                if (flag_verbose) {
                    cerr << " done (" << stoptime(lasttime) << ", max stack depth " <<
                         env.maxCallStack() << ")" << endl;
                }

                if (flag_optimize) {
                    if (flag_verbose) {
                        cerr << "Optimizing ...";
                    }
                    optimize(env);
                    for (unsigned int i = 0; i < env.warnings().size(); i++) {
                        cerr << (flag_werror ? "Error: " : "Warning: ") << env.warnings()[i];
                    }
                    if (flag_werror && env.warnings().size() > 0) {
                        return 1;
                    }
                    if (flag_verbose) {
                        cerr << " done (" << stoptime(lasttime) << ")" << endl;
                    }
                }

                if (flag_verbose) {
                    cerr << "Converting to old FlatZinc ...";
                }
                oldflatzinc(env);
                if (flag_verbose) {
                    cerr << " done (" << stoptime(lasttime) << ")" << endl;
                }

                Printer p(outstream, 0);
                p.print(flat);

                if (flag_verbose) {
                    cerr << " done (" << stoptime(lasttime) << ")" << endl;
                }

            }

        } catch (LocationException &e) {
            cerr << endl;
            cerr << e.loc() << ":" << endl;
            cerr << e.what() << ": " << e.msg() << endl;
            return 1;
        } catch (Exception &e) {
            cerr << endl;
            cerr << e.what() << ": " << e.msg() << endl;
            return 1;
        }
        delete m;
    } else {
        cerr << endl;
        copy(istreambuf_iterator<char>(errstream), istreambuf_iterator<char>(),
             ostreambuf_iterator<char>(cerr));
        return 1;
    }

    if (flag_verbose) {
        cerr << "Flattening Done (overall time " << stoptime(starttime) << ", ";
        size_t mem = GC::maxMem();
        if (mem < 1024) {
            cerr << "maximum memory " << mem << " bytes";
        }
        else if (mem < 1024 * 1024) {
            cerr << "maximum memory " << mem / 1024 << " Kbytes";
        }
        else {
            cerr << "maximum memory " << mem / (1024 * 1024) << " Mbytes";
        }
        cerr << ")." << endl;

    }
    return 0;
}

using namespace Gecode;

// Solve the flatzinc input
string fznsolver(stringstream &in, int numsolutions) {

    string solutions;

    Support::Timer t_total;
    t_total.start();
    FlatZinc::FlatZincOptions opt("Gecode/FlatZinc");

    if (numsolutions != 1) {  // 1 is the default number of solutions for fzn-gecode
        int ar = 3;
        char str[10], nflag[] = "-n";
        sprintf(str, "%d", numsolutions);
        char *flag_array[] = {NULL, nflag, str};
        opt.parse(ar, flag_array);
    }

    FlatZinc::Printer p;
    FlatZinc::FlatZincSpace *fg = NULL;
    FlatZinc::FznRnd rnd(opt.seed());

    ostringstream out;

    try {
        fg = FlatZinc::parse(in, p, cerr, NULL, &rnd);

        if (fg) {  // If FlatZincSpace object has contents after parsing, then solve it.
            fg->createBranchers(fg->solveAnnotations(), opt.seed(), opt.decay(), false, cerr);
            fg->shrinkArrays(p);
            fg->run(out, p, opt, t_total);

        } else {
            exit(EXIT_FAILURE);
        }
        delete fg;
        solutions = out.str();
        return solutions;

    } catch (FlatZinc::Error &e) {
        cerr << "Error: " << e.toString() << endl;
        return e.toString();
    }
}

extern "C" {
typedef struct {
    size_t length;
    const char *content;
} RetString;

RetString solve(const char *mzn, int numsolutions, bool flag_verbose, const char *include)
{
    stringstream fz;
    string temp;
    if (flattenmem(mzn, fz, flag_verbose, include) == 1) {
        temp = "Flattening Error";
    } else {
        temp = fznsolver(fz, numsolutions);
    }
    char *solutions = new char[temp.length() + 1];
    strncpy(solutions, temp.c_str(), temp.length());
    solutions[temp.length()] = '\0';
    return {temp.length(), solutions};
}

void release_retstring(RetString r)
{
    delete[] r.content;
}
}
