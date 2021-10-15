// Copyright (C) 2020 Intel Corporation

// SPDX-License-Identifier: MIT

#include <CL/cl.h>
#include <CL/sycl.hpp>
#include <CL/sycl/backend/opencl.hpp>
#include <array>
#include <iostream>
using namespace sycl;

int main() {
  constexpr size_t size = 16;
  std::array<int, size> data;

  for (int i = 0; i < size; i++) {
    data[i] = i;
  }

  {
    buffer data_buf{data};

// BEGIN CODE SNIP
    // Note: This must select a device that supports interop!
    queue Q{ cpu_selector{} };

    cl_context OCLContext = get_native<backend::opencl>(Q.get_context());
    cl_device_id OCLDeviceID = get_native<backend::opencl>(Q.get_device());

    const std::string Source = R"CLC(
            kernel void add(global int* data) {
                int index = get_global_id(0);
                data[index] = data[index] + 1;
            }
        )CLC";

    const char *SourceStr = Source.c_str();
    const size_t SourceSize = Source.size();

    cl_int Ret;
    cl_program OCLProgram =
        clCreateProgramWithSource(OCLContext, 1, &SourceStr, &SourceSize, &Ret);
    Ret = clBuildProgram(OCLProgram, 1, &OCLDeviceID, "-cl-fast-relaxed-math",
                         nullptr, nullptr);
    cl_kernel OCLKernel = clCreateKernel(OCLProgram, "add", &Ret);

    kernel SYCLKernel =
        make_kernel<backend::opencl>(OCLKernel, Q.get_context());

    std::cout << "Running on device: "
              << Q.get_device().get_info<info::device::name>() << "\n";

    Q.submit([&](handler& h) {
      accessor data_acc {data_buf, h};

      h.set_args(data_acc);
      h.parallel_for(size, SYCLKernel);
    });

    clReleaseKernel(OCLKernel);
    clReleaseProgram(OCLProgram);
    clReleaseContext(OCLContext);
    // END CODE SNIP
  }

  for (int i = 0; i < size; i++) {
    if (data[i] != i + 1) {
      std::cout << "Results did not validate at index " << i << "!\n";
      return -1;
    }
  }

  std::cout << "Success!\n";
  return 0;
}
