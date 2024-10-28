#include <iostream>

#include <thrust/set_operations.h>
#include <thrust/device_vector.h>

#include "test.h"

static void print_vector(const thrust::device_vector<uint32_t> &v) {
  for (auto a : v) {
    std::cout << a << ' ';
  }
  std::cout << std::endl;
}

void test() {
  thrust::device_vector<uint32_t> v {2, 7};
  thrust::device_vector<uint32_t> u {1, 2, 3, 4, 5, 6, 7, 8, 9};

  print_vector(v);
  print_vector(u);

  thrust::device_vector<uint32_t> w(2);

  auto end = thrust::set_intersection(v.begin(), v.end(),
                                      u.begin(), u.end(),
                                      w.begin());

  auto size = thrust::distance(w.begin(), end);

  std::cout << "size = " << size << std::endl;
  print_vector(w);
}