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

#ifndef CUBOOL_SPREDUCE_CUH
#define CUBOOL_SPREDUCE_CUH

#include <cuda/details/sp_vector.hpp>
#include <cuda/kernels/bin_search.cuh>
#include <thrust/device_vector.h>
#include <nsparse/matrix.h>

#include <thrust/sort.h>

namespace cubool {
    namespace kernels {

        template <typename IndexType, typename AllocType>
        class SpReduceFunctor {
        public:
            template<typename T>
            using ContainerType = thrust::device_vector<T, typename AllocType::template rebind<T>::other>;
            using MatrixType = nsparse::matrix<bool, IndexType, AllocType>;

            /**
             * evaluates r = sum(col for each col in a).
             *
             * @param a Csr matrix
             * @param b Csr matrix
             *
             * @return reduce(a)
             */
            MatrixType operator()(const MatrixType& a) {
                auto nrows = a.m_rows;

                ContainerType<IndexType> rowIndex(nrows + 1);
                ContainerType<IndexType> tmpRowIndex(nrows + 1);

                // For each row if not empty then row will have 1 value
                thrust::for_each(thrust::counting_iterator<IndexType>(0), thrust::counting_iterator<IndexType>(nrows),
                    [aRowIndex = a.m_row_index.data(), tmpRowIndex = tmpRowIndex.data()]
                    __device__ (IndexType i) {
                        auto nnz = aRowIndex[i + 1] - aRowIndex[i];
                        tmpRowIndex[i] = nnz > 0? 1: 0;
                    }
                );

                // Use prefix sum to evaluate row index offsets
                thrust::exclusive_scan(tmpRowIndex.begin(), tmpRowIndex.end(), rowIndex.begin(), (IndexType) 0, thrust::plus<IndexType>());

                // Total number of nnz in the result
                size_t resultNvals = rowIndex.back();

                // Fill the col index with 0 for each value, since the result is M x 1 dim vector
                ContainerType<IndexType> colIndex(resultNvals);
                thrust::fill(colIndex.begin(), colIndex.end(), (IndexType) 0);

                assert(colIndex.size() == resultNvals);
                assert(rowIndex.size() == nrows + 1);
                assert(rowIndex.back() == resultNvals);

                // Returned matrix has M x 1 dim (so ncols always 1)
                return MatrixType(std::move(colIndex), std::move(rowIndex), nrows, 1, resultNvals);
            }
        };

        template <typename IndexType, typename AllocType>
        class SpVectorMatrixReduceFunctor {
        public:
            template<typename T>
            using ContainerType = thrust::device_vector<T, typename AllocType::template rebind<T>::other>;
            using MatrixType = nsparse::matrix<bool, IndexType, AllocType>;
            using VectorType = details::SpVector<IndexType, AllocType>;

            VectorType operator()(const MatrixType& m) {
                auto nrows = m.m_rows;

                ContainerType<index> nnz(1);
                thrust::fill(nnz.begin(), nnz.end(), 0);

                // For each row if not empty then row will have 1 value
                thrust::for_each(thrust::counting_iterator<IndexType>(0), thrust::counting_iterator<IndexType>(nrows),
                     [rowIndex = m.m_row_index.data(), nnz = nnz.data()]
                         __device__ (IndexType i) {
                        auto rowSize = rowIndex[i + 1] - rowIndex[i];

                        if (rowSize > 0) {
                            atomicAdd(nnz.get(), (IndexType) 1);
                        }
                     }
                );

                IndexType resultSize = nnz.back();
                thrust::fill(nnz.begin(), nnz.end(), 0);

                ContainerType<index> result(resultSize);

                // For each row if not empty then row will have 1 value
                thrust::for_each(thrust::counting_iterator<IndexType>(0), thrust::counting_iterator<IndexType>(nrows),
                     [rowIndex = m.m_row_index.data(), resultBuffer = result.data(), nnz = nnz.data()]
                         __device__ (IndexType i) {
                         auto rowSize = rowIndex[i + 1] - rowIndex[i];

                         if (rowSize > 0) {
                             auto writePlace = atomicAdd(nnz.get(), (IndexType) 1);
                             resultBuffer[writePlace] = i;
                         }
                     }
                );

                // Sort values within vector
                thrust::sort(result.begin(), result.end());

                return VectorType(std::move(result), nrows, resultSize);
            }

        };

        template <typename IndexType, typename AllocType>
        class SpVectorMatrixTransposedReduceFunctor {
        public:
            template<typename T>
            using ContainerType = thrust::device_vector<T, typename AllocType::template rebind<T>::other>;
            using MatrixType = nsparse::matrix<bool, IndexType, AllocType>;
            using VectorType = details::SpVector<IndexType, AllocType>;

            VectorType operator()(const MatrixType& m) {
                auto ncols = m.m_cols;
                auto nvals = m.m_vals;

                ContainerType<index> mask(ncols);
                thrust::fill(mask.begin(), mask.end(), (IndexType) 0);

                // For each value check, if column correspond to the result
                thrust::for_each(thrust::counting_iterator<IndexType>(0), thrust::counting_iterator<IndexType>(nvals),
                     [colIndex = m.m_col_index.data(), mask = mask.data()]
                         __device__ (IndexType i) {
                         auto j = colIndex[i];

                         mask[j] = 0x1u;
                     }
                );

                // Count nnz of the vector
                auto resultSize = thrust::reduce(mask.begin(), mask.end(), (IndexType) 0);

                ContainerType<index> result(resultSize);
                ContainerType<index> order(1);
                thrust::fill(order.begin(), order.end(), (IndexType) 0);

                // For each value of the mask write in the result buffer if it not zero
                thrust::for_each(thrust::counting_iterator<IndexType>(0), thrust::counting_iterator<IndexType>(ncols),
                     [mask = mask.data(), result = result.data(), order = order.data()]
                         __device__ (IndexType i) {
                        if (mask[i] > 0) {
                            auto offset = atomicAdd(order.get(), (IndexType) 1);
                            auto colId = i;

                            result[offset] = colId;
                        }
                     }
                );

                // Sort values within vector
                thrust::sort(result.begin(), result.end());

                return VectorType(std::move(result), ncols, resultSize);
            }

        };

    }
}

#endif //CUBOOL_SPREDUCE_CUH
