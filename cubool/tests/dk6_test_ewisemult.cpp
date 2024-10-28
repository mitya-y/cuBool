#include <testing/testing.hpp>
#include <algorithm>

using DataMatrix = std::vector<std::vector<int>>;

static void printTestingMatrix(const testing::Matrix &matrix, std::string name = "") {
    if (name != "") {
        std::cout << name << std::endl;
    }

    for (int i = 0; i < matrix.nvals; i++) {
        printf("(%d, %d)\n", matrix.rowsIndex[i], matrix.colsIndex[i]);
    }
}

static void printCuboolMatrix(cuBool_Matrix matrix, std::string name = "") {
    if (name != "") {
        std::cout << name << std::endl;
    }

    cuBool_Index nvals;
    cuBool_Matrix_Nvals(matrix, &nvals);
    std::vector<cuBool_Index> rows(nvals), cols(nvals);
    cuBool_Matrix_ExtractPairs(matrix, rows.data(), cols.data(), &nvals);

    for (int i = 0; i < nvals; i++) {
        printf("(%d, %d)\n", rows[i], cols[i]);
    }
}

static void testMatrixMultiply(const DataMatrix &left_data, const DataMatrix &right_data) {
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
    ncols = left_data[0].size();
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
    testing::MatrixEWiseMultFunctor functor;
    test_result = std::move(functor(test_left, test_right));

    // get actual result
    ASSERT_EQ(cuBool_Matrix_EWiseMult(result, left, right, CUBOOL_HINT_NO), CUBOOL_STATUS_SUCCESS);

    printCuboolMatrix(result, "result");
    printTestingMatrix(test_result, "result test");

    // check result
    ASSERT_EQ(test_result.areEqual(result), true);

    // free matrices
    ASSERT_EQ(cuBool_Matrix_Free(left), CUBOOL_STATUS_SUCCESS);
    ASSERT_EQ(cuBool_Matrix_Free(right), CUBOOL_STATUS_SUCCESS);
    ASSERT_EQ(cuBool_Matrix_Free(result), CUBOOL_STATUS_SUCCESS);
}

#if 0
TEST(cuBool_Matrix, TestEwiseMult) {
    ASSERT_EQ(cuBool_Initialize(CUBOOL_HINT_NO), CUBOOL_STATUS_SUCCESS);

    DataMatrix left {
        {1, 0, 0},
        {0, 0, 0},
        {0, 1, 0},
    };

    // DataMatrix right {
    //     {0, 1, 1},
    //     {1, 0, 1},
    //     {0, 1, 1},
    // };
    DataMatrix right {
        {1, 1, 1},
        {1, 1, 1},
        {1, 1, 1},
    };

    testMatrixMultiply(left, right);

    ASSERT_EQ(cuBool_Finalize(), CUBOOL_STATUS_SUCCESS);
}
#endif

TEST(cuBool_Matrix, Test) {
    test();
}

CUBOOL_GTEST_MAIN
