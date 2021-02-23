import ctypes

__all__ = [
    "configure_instance_desc",
    "load_and_configure",
    "get_build_hints",
    "check"
]


class CuBoolInstanceDescExt(ctypes.Structure):
    _fields_ = [("memoryType", ctypes.c_uint)]


_memory_type_generic = 0
_memory_type_unified = 1

_hint_none = 0x0
_hint_values_sorted = 0x1


def configure_instance_desc():
    desc = CuBoolInstanceDescExt()
    desc.memoryType = ctypes.c_uint(_memory_type_generic)
    return desc


def get_build_hints(is_sorted):
    return _hint_values_sorted if is_sorted else _hint_none


def load_and_configure(cubool_lib_path: str):
    lib = ctypes.cdll.LoadLibrary(cubool_lib_path)

    instance_p = ctypes.c_void_p
    p_to_instance_p = ctypes.POINTER(instance_p)

    matrix_p = ctypes.c_void_p
    p_to_matrix_p = ctypes.POINTER(matrix_p)

    lib.cuBool_Instance_NewExt.restype = ctypes.c_uint
    lib.cuBool_Instance_NewExt.argtypes = [ctypes.POINTER(CuBoolInstanceDescExt),
                                        p_to_instance_p]

    lib.cuBool_Instance_Free.restype = ctypes.c_uint
    lib.cuBool_Instance_Free.argtypes = [instance_p]

    lib.cuBool_Matrix_New.restype = ctypes.c_uint
    lib.cuBool_Matrix_New.argtypes = [instance_p,
                                      p_to_matrix_p,
                                      ctypes.c_uint,
                                      ctypes.c_uint]

    lib.cuBool_Matrix_Free.restype = ctypes.c_uint
    lib.cuBool_Matrix_Free.argtypes = [instance_p,
                                       matrix_p]

    lib.cuBool_Matrix_Resize.restype = ctypes.c_uint
    lib.cuBool_Matrix_Resize.argtypes = [instance_p,
                                         matrix_p,
                                         ctypes.c_uint,
                                         ctypes.c_uint]

    lib.cuBool_Matrix_Build.restype = ctypes.c_uint
    lib.cuBool_Matrix_Build.argtypes = [instance_p,
                                        matrix_p,
                                        ctypes.POINTER(ctypes.c_uint),
                                        ctypes.POINTER(ctypes.c_uint),
                                        ctypes.c_size_t,
                                        ctypes.c_uint]

    lib.cuBool_Matrix_ExtractPairs.restype = ctypes.c_uint
    lib.cuBool_Matrix_ExtractPairs.argtypes = [instance_p,
                                               matrix_p,
                                               ctypes.POINTER(ctypes.c_uint),
                                               ctypes.POINTER(ctypes.c_uint),
                                               ctypes.POINTER(ctypes.c_size_t)]

    lib.cuBool_Matrix_Duplicate.restype = ctypes.c_uint
    lib.cuBool_Matrix_Duplicate.argtypes = [instance_p,
                                            matrix_p,
                                            p_to_matrix_p]

    lib.cuBool_Matrix_Transpose.restype = ctypes.c_uint
    lib.cuBool_Matrix_Transpose.argtypes = [instance_p,
                                            matrix_p,
                                            matrix_p]

    lib.cuBool_Matrix_Nrows.restype = ctypes.c_uint
    lib.cuBool_Matrix_Nrows.argtype = [instance_p,
                                       matrix_p,
                                       ctypes.POINTER(ctypes.c_uint)]

    lib.cuBool_Matrix_Ncols.restype = ctypes.c_uint
    lib.cuBool_Matrix_Ncols.argtype = [instance_p,
                                       matrix_p,
                                       ctypes.POINTER(ctypes.c_uint)]

    lib.cuBool_Matrix_Nvals.restype = ctypes.c_uint
    lib.cuBool_Matrix_Nvals.argtype = [instance_p,
                                       matrix_p,
                                       ctypes.POINTER(ctypes.c_size_t)]

    lib.cuBool_EWise_Add.restype = ctypes.c_uint
    lib.cuBool_EWise_Add.argtypes = [instance_p,
                                      matrix_p,
                                      matrix_p]

    lib.cuBool_MxM.restype = ctypes.c_uint
    lib.cuBool_MxM.argtypes = [instance_p,
                               matrix_p,
                               matrix_p,
                               matrix_p]

    lib.cuBool_Kron.restype = ctypes.c_uint
    lib.cuBool_Kron.argtypes = [instance_p,
                                matrix_p,
                                matrix_p,
                                matrix_p]

    return lib


"""
/** Possible status codes that can be returned from cubool api */

/** Successful execution of the function */
CUBOOL_STATUS_SUCCESS,

/** Generic error code */
CUBOOL_STATUS_ERROR,

/** No cuda compatible device in the system */
CUBOOL_STATUS_DEVICE_NOT_PRESENT,

/** Device side error */
CUBOOL_STATUS_DEVICE_ERROR,

/** Failed to allocate memory on cpy or gpu side */
CUBOOL_STATUS_MEM_OP_FAILED,

/** Passed invalid argument to some function */
CUBOOL_STATUS_INVALID_ARGUMENT,

/** Call of the function is not possible for some context */
CUBOOL_STATUS_INVALID_STATE

/** Some library feature is not implemented */
CUBOOL_STATUS_NOT_IMPLEMENTED
"""
_status_codes_mappings = {
    0: "CUBOOL_STATUS_SUCCESS",
    1: "CUBOOL_STATUS_ERROR",
    2: "CUBOOL_STATUS_DEVICE_NOT_PRESENT",
    3: "CUBOOL_STATUS_DEVICE_ERROR",
    4: "CUBOOL_STATUS_MEM_OP_FAILED",
    5: "CUBOOL_STATUS_INVALID_ARGUMENT",
    6: "CUBOOL_STATUS_INVALID_STATE",
    7: "CUBOOL_STATUS_NOT_IMPLEMENTED"
}

_success = 0


def check(status_code):
    if status_code != _success:
        raise Exception(_status_codes_mappings[status_code])
