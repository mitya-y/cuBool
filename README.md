![Project logo](https://github.com/JetBrains-Research/cuBool/blob/master/docs/logo/logo_white_back_medium.png?raw=true)

# CUBOOL

[![JB Research](https://jb.gg/badges/research-flat-square.svg)](https://research.jetbrains.org/)
[![Ubuntu](https://github.com/JetBrains-Research/cuBool/workflows/Ubuntu/badge.svg?branch=master)](https://github.com/JetBrains-Research/cuBool/actions)
[![License](https://img.shields.io/badge/license-MIT-orange)](https://github.com/JetBrains-Research/cuBool/blob/master/LICENSE)

CuBool is a linear boolean algebra library primitives and operations for 
work with dense and sparse matrices written on the NVIDIA CUDA platform. The primary 
goal of the library is implementation, testing and profiling algorithms for
solving *formal-language-constrained problems*, such as *context-free* 
and *regular* path queries with various semantics for graph databases.
The library provides C-compatible API, written in the GraphBLAS style,
as well as python high-level wrapper with automated resources management.

> The name of the library is formed by a combination of words *Cuda* and *Boolean*,
> what literally means *Cuda with Boolean* and sounds very similar to the name of 
> the programming language *COBOL*.

## Features

- [X] Library C interface
- [X] Library instance/context
- [X] CSR matrix
- [X] CSR multiplication
- [X] CSR addition
- [X] CSR kronecker
- [X] CSR transpose
- [ ] CSR submatrix
- [ ] CSR matrix reduce
- [ ] CSR slicing
- [X] Wrapper for Python API
- [ ] Wrapper tests in Python 
- [ ] User guide
- [X] Unit Tests collection
- [X] Dummy library implementation for testing
- [ ] Publish built artifacts and shared libs
- [ ] Publish stable source code archives

## Requirements

- Linux Ubuntu (tested on 20.04)
- CMake Version 3.17 or higher
- CUDA Compatible GPU device
- GCC Compiler 
- NVIDIA CUDA toolkit
- Python 3 (for `pycubool` library)

## Setup

Before the CUDA setup process, validate your system NVIDIA driver with `nvidia-smi`
command. if it is need, install required driver via `ubuntu-drivers devices` and 
`apt install <driver>` commands respectively.

The following commands grubs the required GCC compilers for the CC and CXX compiling 
respectively. CUDA toolkit, shipped in the default Ubuntu package manager, has version 
number 10 and supports only GCC of the version 8.4 or less.  

```shell script
$ sudo apt update
$ sudo apt install gcc-8 g++-8
$ sudo apt install nvidia-cuda-toolkit
$ sudo apt install nvidia-cuda-dev 
$ nvcc --version
```

If everything successfully installed, the last version command will output 
something like this:

```shell script
$ nvcc: NVIDIA (R) Cuda compiler driver
$ Copyright (c) 2005-2019 NVIDIA Corporation
$ Built on Sun_Jul_28_19:07:16_PDT_2019
$ Cuda compilation tools, release 10.1, V10.1.243
```

**Bonus Step:** In order to have CUDA support in the CLion IDE, you will have to
overwrite global alias for the `gcc` and `g++` compilers:

```shell script
$ sudo rm /usr/bin/gcc
$ sudo rm /usr/bin/g++
$ sudo ln -s /usr/bin/gcc-8 /usr/bin/gcc
$ sudo ln -s /usr/bin/g++-8 /usr/bin/g++
```

This step can be easily undone by removing old aliases and creating new one 
for the desired gcc version on your machine. Also you can safely omit this step
if you want to build library from the command line only. 

**Useful links:**
- [NVIDIA Drivers installation Ubuntu](https://linuxconfig.org/how-to-install-the-nvidia-drivers-on-ubuntu-20-04-focal-fossa-linux)
- [CUDA Linux installation guide](https://docs.nvidia.com/cuda/cuda-installation-guide-linux/index.html)
- [CUDA Hello world program](https://developer.nvidia.com/blog/easy-introduction-cuda-c-and-c/)
- [CUDA CMake tutorial](https://developer.nvidia.com/blog/building-cuda-applications-cmake/)

## Get and run

Run the following commands in the command shell to download the repository,
make `build` directory, configure `cmake build` and run compilation process:

```shell script
$ git clone https://github.com/JetBrains-Research/cuBool.git
$ cd cuBool
$ git submodule update --init --recursive
$ mkdir build
$ cd build
$ cmake .. -DCUBOOL_BUILD_TESTS=ON
$ cmake --build . --target all -j `nproc`
$ sh ./scripts/tests_run_all.sh
```

> Note: in order to provide correct GCC version for CUDA sources compiling,
> you will have to provide custom paths to the CC and CXX compilers before 
> the actual compilation process as follows:
>
> ```shell script
> $ export CC=/usr/bin/gcc-8
> $ export CXX=/usr/bin/g++-8
> $ export CUDAHOSTCXX=/usr/bin/g++-8
> ```

## Python Wrapper

After the build process, the shared library object `libcubool.so` is placed
inside the build directory. Export into the environment or add into bash
profile the variable `CUBOOL_PATH=/path/to/the/libcubool.so` with appropriate
path to your setup. Then you will be able to use `pycubool` python wrapper,
which uses this variable in order to located library object.

## Directory structure

```
cuBool
├── .github - GitHub Actions CI setup 
├── docs - documents, text files and various helpful stuff
├── scripts - short utility programs 
├── cubool - library core source code
│   ├── include - library C API 
│   ├── sources - source-code for implementation
│   │   ├── backend - common interfaces
│   │   ├── core - library core and state management
│   │   ├── cpu - fallback cpu implementation
│   │   └── cuda - cuda backend
│   ├── utils - testing utilities
│   └── tests - gtest-based unit-tests collection
├── pycubool - pycubool related source
│   ├── pycubool - cubool library wrapper for python (similar to pygraphblas)
│   └── tests - tests for python wrapper
├── deps - project dependencies
│   ├── cub - cuda utility, required for nsparse
│   ├── gtest - google test framework for unit testing
│   ├── naive - GEMM implementation for squared dense boolean matrices
│   ├── nsparse - SpGEMM implementation for csr matrices
│   └── nsparse-um - SpGEMM implementation for csr matrices with unified memory (configurable)
└── CMakeLists.txt - library cmake config, add this as sub-directory to your project
```

## Example

The following C++ code snipped demonstrates, how library functions and
primitives can be used for the transitive closure evaluation of the directed
graph, represented as an adjacency matrix with boolean values. The transitive
closure provides info about reachable vertices in the graph:

```c++
/**
 * Performs transitive closure for directed graph
 *
 * @param A Adjacency matrix of the graph
 * @param T Reference to the handle where to allocate and store result
 *
 * @return Status on this operation
 */
cuBoolStatus TransitiveClosure(cuBoolMatrix A, cuBoolMatrix* T) {
    cuBool_Matrix_Duplicate(A, T);                       /* Duplicate A to result T */

    cuBoolIndex total = 0;
    cuBoolIndex current;

    cuBool_Matrix_Nvals(*T, &current);                   /* Query current nvals value */

    while (current != total) {                           /* Iterate, while new values are added */
        total = current;
        cuBool_MxM(*T, *T, *T, CUBOOL_HINT_ACCUMULATE);  /* T += T x T */
        cuBool_Matrix_Nvals(*T, &current);
    }

    return CUBOOL_STATUS_SUCCESS;
}
```

The following Python code snippet demonstrates, how the library python
wrapper can be used to compute the same transitive closure problem for the
directed graph within python environment:

```python
from python import pycubool


def transitive_closure(a: pycubool.Matrix):
    """
    Evaluates transitive closure for the provided
    adjacency matrix of the graph

    :param a: Adjacency matrix of the graph
    :return: The transitive closure adjacency matrix
    """

    t = a.duplicate()                 # Duplicate matrix where to store result
    total = 0                         # Current number of values

    while total != t.nvals:
        total = t.nvals
        pycubool.mxm(t, t, t)         # t += t * t

    return t
```

## Basic application

The following code snippet demonstrates, how to create basic cubool based application
for sparse matrix-matrix multiplication and matrix-matrix element-wise addition
with cubool C API usage.

```c++
/************************************************/
/* Evaluate transitive closure for some graph G */
/************************************************/

/* Actual cubool C API */
#include <cubool/cubool.h>
#include <stdio.h>

/* Macro to check result of the function call */
#define CHECK(f) { cuBoolStatus s = f; if (s != CUBOOL_STATUS_SUCCESS) return s; }

int main() {
    cuBoolMatrix A;
    cuBoolMatrix TC;

    /* System may not provide Cuda compatible device */
    CHECK(cuBool_Initialize(CUBOOL_HINT_NO));

    /* Input graph G */

    /*  -> (1) ->           */
    /*  |       |           */
    /* (0) --> (2) <--> (3) */

    /* Adjacency matrix in sparse format  */
    cuBoolIndex n = 4;
    cuBoolIndex e = 5;
    cuBoolIndex rows[] = { 0, 0, 1, 2, 3 };
    cuBoolIndex cols[] = { 1, 2, 2, 3, 2 };

    /* Create matrix */
    CHECK(cuBool_Matrix_New(&A, n, n));

    /* Fill the data */
    CHECK(cuBool_Matrix_Build(A, rows, cols, e, CUBOOL_HINT_VALUES_SORTED));

    /* Now we have created the following matrix */

    /*    [0][1][2][3]
    /* [0] .  1  1  .  */
    /* [1] .  .  1  .  */
    /* [2] .  .  .  1  */
    /* [3] .  .  1  .  */

    /* Create result matrix from source as copy */
    CHECK(cuBool_Matrix_Duplicate(A, &TC));

    /* Query current number on non-zero elements */
    cuBoolIndex total = 0;
    cuBoolIndex current;
    CHECK(cuBool_Matrix_Nvals(TC, &current));

    /* Loop while values are added */
    while (current != total) {
        total = current;

        /** Transitive closure step */
        CHECK(cuBool_MxM(TC, TC, TC, CUBOOL_HINT_ACCUMULATE));
        CHECK(cuBool_Matrix_Nvals(TC, &current));
    }

    /** Get result */
    cuBoolIndex tc_rows[16], tc_cols[16];
    CHECK(cuBool_Matrix_ExtractPairs(TC, tc_rows, tc_cols, &total));

    /** Now tc_rows and tc_cols contain (i,j) pairs of the result G_tc graph */

    /*    [0][1][2][3]
    /* [0] .  1  1  1  */
    /* [1] .  .  1  1  */
    /* [2] .  .  1  1  */
    /* [3] .  .  1  1  */

    /* Output result size */
    printf("Nnz(tc)=%lli\n", (unsigned long long) total);

    for (cuBoolIndex i = 0; i < total; i++)
        printf("(%u,%u) ", tc_rows[i], tc_cols[i]);

    /* Release resources */
    CHECK(cuBool_Matrix_Free(A));
    CHECK(cuBool_Matrix_Free(TC));

    /* Release library */
    return cuBool_Finalize() != CUBOOL_STATUS_SUCCESS;
}
```

Export path to library cubool, compile the file and run with the following
command (assuming, that source code is placed into tc.cpp file):

```shell script
$ export LD_LIBRARY_PATH="$LD_LIBRARY_PATH:path/to/folder/with/libcubool/"
$ gcc tc.cpp -o tc -I/path/to/cubool/include/dir/ -L/path/to/folder/with/libcubool/ -lcubool
$ ./tc 
```

The program will print the following output:

```
Nnz(tc)=9
(0,1) (0,2) (0,3) (1,2) (1,3) (2,2) (2,3) (3,2) (3,3)
```

## License

This project is licensed under MIT License. License text can be found in the 
[license file](https://github.com/JetBrains-Research/cuBool/blob/master/LICENSE).

## Contributors

- Egor Orachyov (Github: [EgorOrachyov](https://github.com/EgorOrachyov))
- Pavel Alimov (Github : [Krekep](https://github.com/Krekep))
- Semyon Grigorev (Github: [gsvgit](https://github.com/gsvgit))