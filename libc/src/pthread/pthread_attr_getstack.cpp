//===-- Implementation of the pthread_attr_getstack -----------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#include "pthread_attr_getstack.h"
#include "pthread_attr_getstacksize.h"

#include "src/__support/common.h"
#include "src/__support/macros/config.h"
#include "src/__support/macros/optimization.h"

#include <pthread.h>

namespace LIBC_NAMESPACE_DECL {

LLVM_LIBC_FUNCTION(int, pthread_attr_getstack,
                   (const pthread_attr_t *__restrict attr,
                    void **__restrict stack, size_t *__restrict stacksize)) {
  // As of writing this `pthread_attr_getstacksize` can never fail.
  int result = LIBC_NAMESPACE::pthread_attr_getstacksize(attr, stacksize);
  if (LIBC_UNLIKELY(result != 0))
    return result;
  *stack = attr->__stack;
  return 0;
}

} // namespace LIBC_NAMESPACE_DECL
