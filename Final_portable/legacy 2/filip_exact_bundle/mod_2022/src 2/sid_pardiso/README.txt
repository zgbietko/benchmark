CMake was tested on 64bit Windows and 64bit Linux

- In order to use sid_pardiso with your problem module everything you need to do is
to include it in your's CMakeLists.txt (possibly replacing other solver like sid_lapack)

- Before building inspect CMakeLists.txt and change paths, libraries if necessary.
Extensive comments were provided.

- Make sure correct mkl dynamic libraries are either in executable folder or PATH variable
contains their folder. If build is successful but execution fails first check if you selected right
dynamic mkl libraries for your architecture/build type: 64bit or 32bit.

