/**********************************************************************************/
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
/**********************************************************************************/

#include <testing/testing.hpp>

using DataMatrix = std::vector<std::vector<bool>>;

static void printTestingMatrix(const testing::Matrix &matrix, std::string name = "") {
    if (name != "") {
        std::cout << name << std::endl;
    }

    for (int i = 0; i < matrix.nvals; i++) {
        printf("(%d, %d)\n", matrix.colsIndex[i], matrix.rowsIndex[i]);
    }
}

static void testMatrixMultiply(const DataMatrix &left_data, const DataMatrix &right_data) {
    ASSERT_EQ(left_data[0].size(), right_data.size());

    // create testing matrices for expecting values
    cuBool_Index nrows, ncols;
    nrows = left_data.size();
    ncols = left_data[0].size();
    testing::Matrix test_left = testing::Matrix::generatet(nrows, ncols,
      [&left_data](cuBool_Index i, cuBool_Index j) { return left_data[i][j]; });
    nrows = right_data.size();
    ncols = right_data[0].size();
    testing::Matrix test_right = testing::Matrix::generatet(nrows, ncols,
      [&right_data](cuBool_Index i, cuBool_Index j) { return right_data[i][j]; });
    nrows = left_data.size();
    ncols = right_data[0].size();
    testing::Matrix test_result = testing::Matrix::empty(nrows, ncols);

    // printTestingMatrix(test_left, "left");
    // printTestingMatrix(test_right, "right");

    // create cubool matrices
    cuBool_Matrix left, right, result;
    ASSERT_EQ(cuBool_Matrix_New(&left, test_left.nrows, test_left.ncols), CUBOOL_STATUS_SUCCESS);
    ASSERT_EQ(cuBool_Matrix_New(&right, test_right.nrows, test_right.ncols), CUBOOL_STATUS_SUCCESS);
    ASSERT_EQ(cuBool_Matrix_New(&result, test_result.nrows, test_result.ncols), CUBOOL_STATUS_SUCCESS);

    // set data
    ASSERT_EQ(cuBool_Matrix_Build(left, test_left.rowsIndex.data(), test_left.colsIndex.data(), test_left.nvals,
        CUBOOL_HINT_VALUES_SORTED & CUBOOL_HINT_NO_DUPLICATES), CUBOOL_STATUS_SUCCESS);
    ASSERT_EQ(cuBool_Matrix_Build(right, test_right.rowsIndex.data(), test_right.colsIndex.data(), test_right.nvals,
        CUBOOL_HINT_VALUES_SORTED & CUBOOL_HINT_NO_DUPLICATES), CUBOOL_STATUS_SUCCESS);
    // get expceted result
    testing::MatrixMultiplyFunctor functor;
    test_result = std::move(functor(test_left, test_right, test_result, false));

    printTestingMatrix(test_result, "result");

    // get actual result
    ASSERT_EQ(cuBool_MxM(result, left, right, CUBOOL_HINT_NO), CUBOOL_STATUS_SUCCESS);

    // check result
    ASSERT_EQ(test_result.areEqual(result), true);

    // free matrices
    ASSERT_EQ(cuBool_Matrix_Free(left), CUBOOL_STATUS_SUCCESS);
    ASSERT_EQ(cuBool_Matrix_Free(right), CUBOOL_STATUS_SUCCESS);
    ASSERT_EQ(cuBool_Matrix_Free(result), CUBOOL_STATUS_SUCCESS);
}

TEST(cuBool_Matrix, SymFrontA) {
    ASSERT_EQ(cuBool_Initialize(CUBOOL_HINT_NO), CUBOOL_STATUS_SUCCESS);

    DataMatrix left {
        {0, 0, 0},
        {1, 0, 0},
        {0, 0, 0},
    };

    DataMatrix right {
        {1, 0, 0, 0},
        {0, 0, 0, 0},
        {0, 0, 0, 0},
    };

    testMatrixMultiply(left, right);

    ASSERT_EQ(cuBool_Finalize(), CUBOOL_STATUS_SUCCESS);
}

TEST(cuBool_Matrix, NextFrontA) {
    ASSERT_EQ(cuBool_Initialize(CUBOOL_HINT_NO), CUBOOL_STATUS_SUCCESS);

    DataMatrix left {
        {0, 0, 0, 0},
        {1, 0, 0, 0},
        {0, 0, 0, 0},
    };

    DataMatrix right {
        {0, 1, 0, 0},
        {0, 0, 0, 0},
        {1, 0, 0, 0},
        {0, 0, 0, 0},
    };

    testMatrixMultiply(left, right);

    ASSERT_EQ(cuBool_Finalize(), CUBOOL_STATUS_SUCCESS);
}

TEST(cuBool_Matrix, SymFrontB) {
    ASSERT_EQ(cuBool_Initialize(CUBOOL_HINT_NO), CUBOOL_STATUS_SUCCESS);

    DataMatrix left {
        {1, 0, 0},
        {0, 0, 0},
        {0, 1, 0},
    };

    DataMatrix right {
        {1, 0, 0, 0},
        {0, 0, 0, 0},
        {0, 0, 0, 0},
    };

    testMatrixMultiply(left, right);

    ASSERT_EQ(cuBool_Finalize(), CUBOOL_STATUS_SUCCESS);
}

TEST(cuBool_Matrix, NextFrontB) {
    ASSERT_EQ(cuBool_Initialize(CUBOOL_HINT_NO), CUBOOL_STATUS_SUCCESS);

    DataMatrix left {
        {1, 0, 0, 0},
        {0, 0, 0, 0},
        {0, 0, 0, 0},
    };

    DataMatrix right {
        {0, 0, 0, 1},
        {0, 0, 1, 0},
        {0, 0, 0, 0},
        {1, 0, 0, 0},
    };

    testMatrixMultiply(left, right);

    ASSERT_EQ(cuBool_Finalize(), CUBOOL_STATUS_SUCCESS);
}

CUBOOL_GTEST_MAIN