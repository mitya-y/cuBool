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



// bad temporary algorithm
static void invert_matrix(cuBool_Matrix mask) {
  // invert mask
  cuBool_Index nvals;
  cuBool_Matrix_Nvals(mask, &nvals);

  std::vector<cuBool_Index> rows(nvals), cols(nvals);
  cuBool_Matrix_ExtractPairs(mask, rows.data(), cols.data(), &nvals);

  cuBool_Index ncols, nrows;
  cuBool_Matrix_Ncols(mask, &ncols);
  cuBool_Matrix_Nrows(mask, &nrows);
  std::vector inverted_mask(nrows, std::vector(ncols, true));

  for (int i = 0; i < nvals; i++) {
    inverted_mask[rows[i]][cols[i]] = false;
  }

  rows.clear();
  rows.reserve(ncols * nvals - nvals);
  cols.clear();
  cols.reserve(ncols * nvals - nvals);
  for (cuBool_Index i = 0; i < nrows; i++) {
    for (cuBool_Index j = 0; j < ncols; j++) {
      if (inverted_mask[i][j]) {
        rows.push_back(i);
        cols.push_back(j);
      }
    }
  }

  cuBool_Matrix_Build(mask, rows.data(), cols.data(), rows.size(), CUBOOL_HINT_NO);
}

void testInvertMatrix(const DataMatrix &data) {
    // create testing matrices for expecting values
    cuBool_Index nrows = data.size();
    cuBool_Index ncols = data[0].size();
    testing::Matrix test_matrix = testing::Matrix::generatet(nrows, ncols,
        [&data](cuBool_Index i, cuBool_Index j) { return data[i][j]; });

    cuBool_Matrix matrix;
    ASSERT_EQ(cuBool_Matrix_New(&matrix, test_matrix.nrows, test_matrix.ncols), CUBOOL_STATUS_SUCCESS);
    ASSERT_EQ(cuBool_Matrix_Build(matrix, test_matrix.rowsIndex.data(), test_matrix.colsIndex.data(), test_matrix.nvals,
        CUBOOL_HINT_VALUES_SORTED & CUBOOL_HINT_NO_DUPLICATES), CUBOOL_STATUS_SUCCESS);

    invert_matrix(matrix);

    printCuboolMatrix(matrix);
}

TEST(cuBool_Matrix, InvertMatrix) {
    ASSERT_EQ(cuBool_Initialize(CUBOOL_HINT_NO), CUBOOL_STATUS_SUCCESS);

    DataMatrix data {
        {1, 0, 0},
        {0, 0, 0},
        {0, 1, 0},
    };

    testInvertMatrix(data);

    ASSERT_EQ(cuBool_Finalize(), CUBOOL_STATUS_SUCCESS);
}

TEST(cuBool_Matrix, InvertMatrix2) {
    ASSERT_EQ(cuBool_Initialize(CUBOOL_HINT_NO), CUBOOL_STATUS_SUCCESS);

    DataMatrix data {
        {0, 0, 0, 1},
        {0, 0, 1, 0},
        {0, 0, 0, 0},
        {1, 0, 0, 0},
    };

    testInvertMatrix(data);

    ASSERT_EQ(cuBool_Finalize(), CUBOOL_STATUS_SUCCESS);
}

CUBOOL_GTEST_MAIN