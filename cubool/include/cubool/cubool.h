/**********************************************************************************/
/*                                                                                */
/* MIT License                                                                    */
/*                                                                                */
/* Copyright (c) 2020 JetBrains-Research                                          */
/*                                                                                */
/* Permission is hereby granted, free of charge, to any person obtaining a copy   */
/* of this software and associated documentation files (the "Software"), to deal  */
/* in the Software without restriction, including without limitation the rights   */
/* to use, copy, modify, merge, publish, distribute, sublicense, and/or sell      */
/* copies of the Software, and to permit persons to whom the Software is          */
/* furnished to do so, subject to the following conditions:                       */
/*                                                                                */
/* The above copyright notice and this permission notice shall be included in all */
/* copies or substantial portions of the Software.                                */
/*                                                                                */
/* THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR     */
/* IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,       */
/* FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE    */
/* AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER         */
/* LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,  */
/* OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE  */
/* SOFTWARE.                                                                      */
/*                                                                                */
/**********************************************************************************/

#ifndef CUBOOL_CUBOOL_H
#define CUBOOL_CUBOOL_H

#ifdef __cplusplus
    #include <cinttypes>
#else
    #include <inttypes.h>
#endif

// Preserve C names in shared library
#define CUBOOL_EXPORT extern "C"

// Exporting/importing symbols for Microsoft Visual Studio
#if (_MSC_VER && !__INTEL_COMPILER)
    #ifdef CUBOOL_EXPORTS
        // Compile the library source code itself
        #define CUBOOL_API __declspec(dllexport)
    #else
        // Import symbols from library into user space
        #define CUBOOL_API __declspec(dllimport)
    #endif
#else
    // Default case
    #define CUBOOL_API
#endif

/** Possible status codes that can be returned from cubool api */
typedef enum cuBoolStatus {
    /** Successful execution of the function */
    CUBOOL_STATUS_SUCCESS = 0,
    /** Generic error code */
    CUBOOL_STATUS_ERROR = 1,
    /** No cuda compatible device in the system */
    CUBOOL_STATUS_DEVICE_NOT_PRESENT = 2,
    /** Device side error */
    CUBOOL_STATUS_DEVICE_ERROR = 3,
    /** Failed to allocate memory on cpy or gpu side */
    CUBOOL_STATUS_MEM_OP_FAILED = 4,
    /** Passed invalid argument to some function */
    CUBOOL_STATUS_INVALID_ARGUMENT = 5,
    /** Call of the function is not possible for some context */
    CUBOOL_STATUS_INVALID_STATE = 6,
    /** Failed to select supported backend for computations */
    CUBOOL_STATUS_BACKEND_ERROR = 7,
    /** Some library feature is not implemented */
    CUBOOL_STATUS_NOT_IMPLEMENTED = 8
} cuBoolStatus;

/** Generic lib hits for matrix processing */
typedef enum cuBoolHint {
    /** No hints passed */
    CUBOOL_HINT_NO = 0x0,
    /** Force Cpu based backend usage */
    CUBOOL_HINT_CPU_BACKEND = 0x1,
    /** Use managed gpu memory type instead of default (device) memory */
    CUBOOL_HINT_GPU_MEM_MANAGED = 0x2,
    /** Mark input data as row-col sorted */
    CUBOOL_HINT_VALUES_SORTED = 0x4,
    /** Accumulate result of the operation in the result matrix */
    CUBOOL_HINT_ACCUMULATE = 0x8
} cuBoolHint;

/** Hit mask */
typedef uint32_t                cuBoolHints;

/** Alias integer type for indexing operations */
typedef uint32_t                cuBoolIndex;

/** Cubool sparse boolean matrix handle */
typedef struct cuBoolMatrix_t*  cuBoolMatrix;

/**
 * @brief Memory allocate callback
 * Signature for user-provided function pointer, used to allocate CPU memory for library resources
 */
typedef void* (*CuBoolCpuMemAllocateFun)(
    cuBoolIndex                 size,
    void*                       userData
);

/**
 * @brief Memory deallocate callback
 * Signature for user-provided function pointer, used to deallocate CPU memory, previously allocated with CuBoolCpuMemAllocateFun
 */
typedef void (*CuBoolCpuMemDeallocateFun)(
    void*                       ptr,
    void*                       userData
);

/**
 * @brief Message callback
 * User provided message callback to observe library messages and errors
 */
typedef void (*CuBoolMsgFun)(
    cuBoolStatus                status,
    const char*                 message,
    void*                       userData
);

typedef struct CuBoolDeviceCaps {
    char                        name[256];
    int                         major;
    int                         minor;
    int                         warp;
    bool                        cudaSupported;
    cuBoolIndex                 globalMemoryKiBs;
    cuBoolIndex                 sharedMemoryPerMultiProcKiBs;
    cuBoolIndex                 sharedMemoryPerBlockKiBs;
} CuBoolDeviceCaps;

typedef struct CuBoolAllocationCallback {
    void*                       userData;
    CuBoolCpuMemAllocateFun     allocateFun;
    CuBoolCpuMemDeallocateFun   deallocateFun;
} CuBoolAllocationCallback;

typedef struct CuBoolMessageCallback {
    void*                       userData;
    CuBoolMsgFun                msgFun;
} CuBoolMessageCallback;

/**
 * Query human-readable text info about the project implementation
 * @return Read-only library about info
 */
CUBOOL_EXPORT CUBOOL_API const char* cuBool_About_Get(
);

/**
 * Query human-readable text info about the project implementation
 * @return Read-only library license info
 */
CUBOOL_EXPORT CUBOOL_API const char* cuBool_LicenseInfo_Get(
);

/**
 * Query library version number in form MAJOR.MINOR
 *
 * @param major Major version number part
 * @param minor Minor version number part
 * @param version Composite integer version
 *
 * @return Error if failed to query version info
 */
CUBOOL_EXPORT CUBOOL_API cuBoolStatus cuBool_Version_Get(
    int*                major,
    int*                minor,
    int*                version
);

/**
 * Query device capabilities/properties if cuda compatible device is present
 *
 * @param deviceCaps Pointer to device caps structure to store result
 *
 * @return Error if cuda device not present or if failed to query capabilities
 */
CUBOOL_EXPORT CUBOOL_API cuBoolStatus cuBool_DeviceCaps_Get(
    CuBoolDeviceCaps*   deviceCaps
);

/**
 * Initialize library instance object, which provides context to all library operations and primitives.
 *
 * @param hints Init hints.
 *
 * @return Error code on this operation
 */
CUBOOL_EXPORT CUBOOL_API cuBoolStatus cuBool_Initialize(
    cuBoolHints         hints
);

/**
 * Destroy library instance and all objects, which were created on this library context.
 *
 * @note Invalidates all handle to the resources, created within this library instance
 *
 * @return Error code on this operation
 */
CUBOOL_EXPORT CUBOOL_API cuBoolStatus cuBool_Finalize(
);

/**
 * Creates new sparse matrix with specified size.
 *
 * @param matrix Pointer where to store created matrix handle
 * @param nrows Matrix rows count
 * @param ncols Matrix columns count
 *
 * @return Error code on this operation
 */
CUBOOL_EXPORT CUBOOL_API cuBoolStatus cuBool_Matrix_New(
    cuBoolMatrix*       matrix,
    cuBoolIndex         nrows,
    cuBoolIndex         ncols
);

/**
 * Build sparse matrix from provided pairs array. Pairs are supposed to be stored
 * as (rows[i],cols[i]) for pair with i-th index.
 *
 * @param matrix Matrix handle to perform operation on
 * @param rows Array of pairs row indices
 * @param cols Array of pairs column indices
 * @param nvals Number of the pairs passed
 * @param hints Hits flags for processing. Pass VALUES_SORTED if values already in the proper order.
 *
 * @return Error code on this operation
 */
CUBOOL_EXPORT CUBOOL_API cuBoolStatus cuBool_Matrix_Build(
    cuBoolMatrix        matrix,
    const cuBoolIndex*  rows,
    const cuBoolIndex*  cols,
    cuBoolIndex         nvals,
    cuBoolHints         hints
);

/**
 * Reads matrix data to the host visible CPU buffer as an array of values pair.
 *
 * The arrays must be provided by the user and the size of this arrays must
 * be greater or equal the values count of the matrix.
 *
 * @param matrix Matrix handle to perform operation on
 * @param[in,out] rows Buffer to store row indices
 * @param[in,out] cols Buffer to store column indices
 * @param[in,out] nvals Total number of the pairs
 *
 * @return Error code on this operation
 */
CUBOOL_EXPORT CUBOOL_API cuBoolStatus cuBool_Matrix_ExtractPairs(
    cuBoolMatrix        matrix,
    cuBoolIndex*        rows,
    cuBoolIndex*        cols,
    cuBoolIndex*        nvals
);

/**
 * Creates new sparse matrix, duplicates content and stores handle in the provided pointer.
 * 
 * @param matrix Matrix handle to perform operation on
 * @param duplicated[out] Pointer to the matrix handle where to store created matrix
 * 
 * @return Error code on this operation
 */
CUBOOL_EXPORT CUBOOL_API cuBoolStatus cuBool_Matrix_Duplicate(
    cuBoolMatrix        matrix,
    cuBoolMatrix*       duplicated
);

/**
 * Transpose source matrix and store result of this operation in result matrix.
 * Formally: result = matrix ^ T
 *
 * @param result Matrix handle to store result of the operation
 * @param matrix The source matrix
 *
 * @return Error code on this operation
 */
CUBOOL_EXPORT CUBOOL_API cuBoolStatus cuBool_Matrix_Transpose(
    cuBoolMatrix        result,
    cuBoolMatrix        matrix
);

/**
 *
 * @param matrix Matrix handle to perform operation on
 * @param nvals[out] Pointer to  the place where to store number of the non-zero elements of the matrix
 * 
 * @return Error code on this operation
 */
CUBOOL_EXPORT CUBOOL_API cuBoolStatus cuBool_Matrix_Nvals(
    cuBoolMatrix        matrix,
    cuBoolIndex*        nvals
);

/**
 * 
 * @param matrix Matrix handle to perform operation on
 * @param nrows[out] Pointer to the place where to store number of matrix rows
 * 
 * @return Error code on this operation
 */
CUBOOL_EXPORT CUBOOL_API cuBoolStatus cuBool_Matrix_Nrows(
    cuBoolMatrix        matrix,
    cuBoolIndex*        nrows
);

/**
 * 
 * @param matrix Matrix handle to perform operation on
 * @param ncols[out] Pointer to the place where to store number of matrix columns
 * 
 * @return Error code on this operation
 */
CUBOOL_EXPORT CUBOOL_API cuBoolStatus cuBool_Matrix_Ncols(
    cuBoolMatrix        matrix,
    cuBoolIndex*        ncols
);

/**
 * Deletes sparse matrix object.
 *
 * @param matrix Matrix handle to delete the matrix
 *
 * @return Error code on this operation
 */
CUBOOL_EXPORT CUBOOL_API cuBoolStatus cuBool_Matrix_Free(
    cuBoolMatrix        matrix
);

/**
 * Performs result += left, where '+' is boolean semiring operation.
 *
 * @note Matrices must be compatible
 *      dim(result) = M x N
 *      dim(left) = M x N
 *
 * @param result Destination matrix for add-and-assign operation
 * @param left Source matrix to be added
 * @param right Source matrix to be added
 *
 * @return Error code on this operation
 */
CUBOOL_EXPORT CUBOOL_API cuBoolStatus cuBool_Matrix_EWiseAdd(
    cuBoolMatrix        result,
    cuBoolMatrix        left,
    cuBoolMatrix        right
);

/**
 * Performs result (accum)= left x right evaluation, where source '+' and 'x' are boolean semiring operations.
 * If accum hint passed, the the result of the multiplication is added to the result matrix.
 *
 * @note to perform this operation matrices must be compatible
 *       dim(left) = M x T
 *       dim(right) = T x N
 *       dim(result) = M x N
 *
 * @param result Matrix handle where to store operation result
 * @param left Input left matrix
 * @param right Input right matrix
 * @param hints Pass CUBOOL_HINT_ACCUMULATE hint to add result of the left x right operation.
 *
 * @return Error code on this operation
 */
CUBOOL_EXPORT CUBOOL_API cuBoolStatus cuBool_MxM(
    cuBoolMatrix        result,
    cuBoolMatrix        left,
    cuBoolMatrix        right,
    cuBoolHints         hints
);

/**
 * Performs result = left `kron` right, where `kron` is a Kronecker product for boolean semiring.
 *
 * @note when the operation is performed, the result matrix has the following dimension
 *      dim(left) = M x N
 *      dim(right) = K x T
 *      dim(result) = MK x NT
 *
 * @param result Matrix handle where to store operation result
 * @param left Input left matrix
 * @param right Input right matrix
 *
 * @return Error code on this operation
 */
CUBOOL_EXPORT CUBOOL_API cuBoolStatus cuBool_Kronecker(
    cuBoolMatrix        result,
    cuBoolMatrix        left,
    cuBoolMatrix        right
);

#endif //CUBOOL_CUBOOL_H