/**********************************************************************************/
/*                                                                                */
/* MIT License                                                                    */
/*                                                                                */
/* Copyright (c) 2020, 2021 JetBrains-Research                                    */
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

#include <testing/testing.hpp>

void testMatrixMultiplyAdd(cuBoolIndex m, cuBoolIndex t, cuBoolIndex n, float density) {
    cuBoolMatrix a, b, r;

    // Generate test data with specified density
    testing::Matrix ta = std::move(testing::Matrix::generate(m, t, testing::Condition3(density)));
    testing::Matrix tb = std::move(testing::Matrix::generate(t, n, testing::Condition3(density)));
    testing::Matrix tr = std::move(testing::Matrix::generate(m, n, testing::Condition3(density)));

    // Allocate input matrices and resize to fill with input data
    ASSERT_EQ(cuBool_Matrix_New(&a, m, t), CUBOOL_STATUS_SUCCESS);
    ASSERT_EQ(cuBool_Matrix_New(&b, t, n), CUBOOL_STATUS_SUCCESS);
    ASSERT_EQ(cuBool_Matrix_New(&r, m, n), CUBOOL_STATUS_SUCCESS);

    // Transfer input data into input matrices
    ASSERT_EQ(cuBool_Matrix_Build(a, ta.mRowsIndex.data(), ta.mColsIndex.data(), ta.mNvals, CUBOOL_HINT_VALUES_SORTED), CUBOOL_STATUS_SUCCESS);
    ASSERT_EQ(cuBool_Matrix_Build(b, tb.mRowsIndex.data(), tb.mColsIndex.data(), tb.mNvals, CUBOOL_HINT_VALUES_SORTED), CUBOOL_STATUS_SUCCESS);
    ASSERT_EQ(cuBool_Matrix_Build(r, tr.mRowsIndex.data(), tr.mColsIndex.data(), tr.mNvals, CUBOOL_HINT_VALUES_SORTED), CUBOOL_STATUS_SUCCESS);

    // Evaluate r += a x b
    ASSERT_EQ(cuBool_MxM(r, a, b, CUBOOL_HINT_ACCUMULATE), CUBOOL_STATUS_SUCCESS);

    // Evaluate naive r += a x b on the cpu to compare results
    testing::MatrixMultiplyFunctor functor;
    tr = std::move(functor(ta, tb, tr));

    // Compare results
    ASSERT_EQ(tr.areEqual(r), true);

    // Deallocate matrices
    ASSERT_EQ(cuBool_Matrix_Free(a), CUBOOL_STATUS_SUCCESS);
    ASSERT_EQ(cuBool_Matrix_Free(b), CUBOOL_STATUS_SUCCESS);
    ASSERT_EQ(cuBool_Matrix_Free(r), CUBOOL_STATUS_SUCCESS);
}

void testRun(cuBoolIndex m, cuBoolIndex t, cuBoolIndex n, cuBoolHints setup) {
    // Setup library
    ASSERT_EQ(cuBool_Initialize(setup), CUBOOL_STATUS_SUCCESS);

    for (size_t i = 0; i < 5; i++) {
        testMatrixMultiplyAdd(m, t, n, 0.1f + (0.05f) * ((float) i));
    }

    // Finalize library
    ASSERT_EQ(cuBool_Finalize(), CUBOOL_STATUS_SUCCESS);
}

TEST(MatrixCsr, MultiplyAddSmall) {
    cuBoolIndex m = 60, t = 100, n = 80;
    testRun(m, t, n, CUBOOL_HINT_NO);
}

TEST(MatrixCsr, MultiplyAddMedium) {
    cuBoolIndex m = 500, t = 1000, n = 800;
    testRun(m, t, n, CUBOOL_HINT_NO);

}

TEST(MatrixCsr, MultiplyAddLarge) {
    cuBoolIndex m = 1000, t = 2000, n = 500;
    testRun(m, t, n, CUBOOL_HINT_NO);
}

CUBOOL_GTEST_MAIN