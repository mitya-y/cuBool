#include <testing/testing.hpp>
#include <algorithm>

using DataMatrix = std::vector<std::vector<int>>;

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

static void apply_not_mask(cuBool_Matrix matrix, cuBool_Matrix mask) {
  cuBool_Matrix inverted_mask;
  cuBool_Matrix_Duplicate(mask, &inverted_mask);
  invert_matrix(inverted_mask);

  // printCuboolMatrix(inverted_mask, "inverted mask");

  cuBool_Matrix tmp_frontier;
  cuBool_Matrix_Duplicate(matrix, &tmp_frontier);

  cuBool_Matrix_EWiseMult(matrix, tmp_frontier, inverted_mask, CUBOOL_HINT_NO);

  printCuboolMatrix(tmp_frontier, "tmp_frontier");
  printCuboolMatrix(inverted_mask, "inverted_mask");
  printCuboolMatrix(matrix, "matrix");

  cuBool_Matrix_Free(inverted_mask);
  cuBool_Matrix_Free(tmp_frontier);
}

void testApplyNotMask(const DataMatrix &matrix_data, const DataMatrix &mask_data) {
    cuBool_Index nrows, ncols;
    nrows = matrix_data.size();
    ncols = matrix_data[0].size();
    testing::Matrix test_matrix = testing::Matrix::generatet(nrows, ncols,
        [&matrix_data](cuBool_Index i, cuBool_Index j) { return matrix_data[i][j]; });
    nrows = mask_data.size();
    ncols = mask_data[0].size();
    testing::Matrix test_mask = testing::Matrix::generatet(nrows, ncols,
        [&mask_data](cuBool_Index i, cuBool_Index j) { return mask_data[i][j]; });

    cuBool_Matrix matrix, mask;
    ASSERT_EQ(cuBool_Matrix_New(&matrix, test_matrix.nrows, test_matrix.ncols), CUBOOL_STATUS_SUCCESS);
    ASSERT_EQ(cuBool_Matrix_New(&mask, test_mask.nrows, test_mask.ncols), CUBOOL_STATUS_SUCCESS);

    ASSERT_EQ(cuBool_Matrix_Build(matrix, test_matrix.rowsIndex.data(), test_matrix.colsIndex.data(), test_matrix.nvals,
        CUBOOL_HINT_VALUES_SORTED & CUBOOL_HINT_NO_DUPLICATES), CUBOOL_STATUS_SUCCESS);
    ASSERT_EQ(cuBool_Matrix_Build(mask, test_mask.rowsIndex.data(), test_mask.colsIndex.data(), test_mask.nvals,
        CUBOOL_HINT_VALUES_SORTED & CUBOOL_HINT_NO_DUPLICATES), CUBOOL_STATUS_SUCCESS);

    apply_not_mask(matrix, mask);

    // printCuboolMatrix(matrix, "matrix");

    auto mask_data_inverted = mask_data;
    for (auto &row : mask_data_inverted) {
        for (int &value : row) {
            value = !value;
        }
    }

    // validate value of algorithm
    cuBool_Index nvals;
    cuBool_Matrix_Nvals(matrix, &nvals);
    std::vector<cuBool_Index> rows(nvals), cols(nvals);
    cuBool_Matrix_ExtractPairs(mask, rows.data(), cols.data(), &nvals);

    for (int i = 0; i < nvals; i++) {
        int &value = mask_data_inverted[rows[i]][cols[i]];
        ASSERT_NE(value, 0);
        value = 0;
    }

    ASSERT_TRUE(std::all_of(mask_data_inverted.begin(), mask_data_inverted.end(),
        [](const auto &row) {
            return std::all_of(row.begin(), row.end(),
                [](int value) { return value == 0; });
        }));
}

TEST(cuBool_Matrix, ApplyMatrix) {
    ASSERT_EQ(cuBool_Initialize(CUBOOL_HINT_NO), CUBOOL_STATUS_SUCCESS);

    DataMatrix matrix {
        {1, 0, 0},
        {0, 0, 0},
        {0, 1, 0},
    };

    DataMatrix mask {
        {0, 1, 1},
        {1, 0, 1},
        {0, 1, 1},
    };
    // iverted is
    // 1 0 0
    // 0 1 0
    // 1 0 0
    // matrix & ~mask must have (0, 0)

    testApplyNotMask(matrix, mask);

    ASSERT_EQ(cuBool_Finalize(), CUBOOL_STATUS_SUCCESS);
}

TEST(cuBool_Matrix, ApplyMatrixRandom) {
    return;
    ASSERT_EQ(cuBool_Initialize(CUBOOL_HINT_NO), CUBOOL_STATUS_SUCCESS);

    for (int i = 0; i < 10; i++) {
        int n = rand() % 10;
        int m = rand() % 10;

        DataMatrix matrix(n, std::vector(m, 0));
        DataMatrix mask(n, std::vector(m, 0));

        for (int i = 0; i < n; i++) {
            for (int j = 0; j < m; j++) {
                matrix[i][j] = rand() & 1;
                mask[i][j] = rand() & 1;
            }
        }

        testApplyNotMask(matrix, mask);
    }

    ASSERT_EQ(cuBool_Finalize(), CUBOOL_STATUS_SUCCESS);
}


CUBOOL_GTEST_MAIN