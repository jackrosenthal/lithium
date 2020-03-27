Lithium C Library and Unit Test Framework
=========================================

Lithium is a C library designed to help you write high quality systems
programs in C. Right now, it is in the early stages and is mostly a
unit testing framework, but this may change in the future as Lithium
grows more features.

Dependencies:

* A POSIX operating system and ``libc``.

To compile the code as a shared object file, type:

    $ make build/release/libithium.so

To compile the code as a static library, type:

    $ make build/release/libithium.a

Distribution packages are pending which will install the library and
header files.

To run the unit tests (written using **Lithium Unit** and **Lithium
Mock**), type:

    $ make run-tests

Lithium Unit
------------

*Lithium Unit* is the core of Lithium's unit testing features. Using
Lithium Unit, tests can be written anywhere, and are automatically
collected by the test runner. This way, even static helper functions
can even be tested. To get started, ``#include <lithium/unit.h>``
where you want to write your test, and use the ``DEFTEST`` macro to
define your test. An example is below:

    DEFTEST("myproject.utils.math.fibonacci", {})
    {
            EXPECT(fib(0) == 0);
            EXPECT(fib(1) == 1);
            EXPECT(fib(11) == 89);
    }

The second parameter to ``DEFTEST`` is an options struct, for example,
to disable tests or make them informational. See the header file for
all available fields.

The ``EXPECT`` macro will test that the condition is true. If it were
false, it would print an error message and mark the test as failed,
but continue running. This differs from the ``ASSERT`` macro, in that
failed assertions will abort the test immediately.

``EXPECT`` will return ``true`` if the condition was true: this way
you can conditionally choose to run additional code based on the
result.

There are 4 other macros to know:

* ``EXPECT_NULL`` - expect that a pointer is null.
* ``ASSERT_NULL`` - assert that a pointer is null, aborting the test
  if the pointer is non-null.
* ``EXPECT_NOT_NULL`` - expect that a pointer is non-null.
* ``ASSERT_NOT_NULL`` - assert that a pointer is non-null, aborting
  the test if the pointer is null.

To run your test, compile your code with ``-DLITHIUM_TEST_BUILD`` and
``-lithium``. Call ``return li_unit_run_tests_main(argv);`` from your
main function. The test runner will run all tests in parallel
automatically.

Lithium Mock
------------

Lithium takes a different approach to mocking from other test
frameworks. Instead of providing specific features to mock a function,
its return values, or structures, Lithium provides a single
highly-powerful macro to mock any expression in your code.

This relies on providing a "mocked expression variable" which is
visible to both the code being mocked and the test code providing
mocked values. This variable gets compiled away to nothing and using
the macros incurs no additional overhead unless ``LITHIUM_TEST_BUILD``
is defined.

First, declare the variable as extern in the header file visible to
both the code being mocked and the tests providing mocked values:

    #include <lithium/mock.h>

    #include "myproject/utils/file.h"

    /* We are mocking the "read_file" function. You can also use a data
       type here instead (e.g. "char *"). */
    extern LI_MOCKABLE_STORAGE_T(read_file) read_file_mock;

You'll need to provide a definition of the variable too, outside of
the header file:

LI_MOCKABLE_STORAGE_T(read_file) read_file_mock = { 0 };

Then, in place of the expression which is mockable, use the
``LI_MOCKABLE(storage_var, default_value)`` macro. For example:

    if (LI_MOCKABLE(read_file_mock, read_file)(path, buffer, buf_sz) < 0) {
            ...
    }

Finally, use ``LI_SETUP_MOCK(storage_var, mocked_value)`` in your test
code:

    static ssize_t mocked_read_file(const char *path, void *buf, size_t buf_sz)
    {
            /* implementation of your mocked function ... */
    }

    DEFTEST("myproject.some.test", {})
    {
            LI_SETUP_MOCK(read_file_mock, mocked_read_file);
            /* ... */
    }
