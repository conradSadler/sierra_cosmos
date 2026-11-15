# OpenC3 COSMOS Plugin

See the [OpenC3](https://openc3.com) documentation for all things OpenC3.

Update this comment with your own description.

<<<<<<< Updated upstream
=======
## TCP server tests (GoogleTest prototype)

The test setup has three files:

- `tests/tcp_con_test.cpp`: I/O state, small replacement functions, one fixture,
  and the test cases, all in one place like the earlier doctest prototype.
- `tests/test_io.h`: substitutes socket/file calls only in the test build.
- `tests/CMakeLists.txt`: builds the actual `tcp_con.c` as C, links GoogleTest,
  and registers the tests with CTest.

No Raspberry Pi or network sockets are needed. Real temporary files exercise
file parsing and output; the fixture cleans them up even after test failures.
The production source is unchanged. The older standalone `tcp_send_test.c`
experiment is not part of this build.

### Build and run

Requires CMake 3.24+, Make or Ninja, and C11/C++17 compilers. GoogleTest 1.17.0
is downloaded at a pinned commit on the first build and cached in `tests/.build`.
From this plugin directory:

```sh
cmake -S tests -B tests/.build
cmake --build tests/.build --parallel
ctest --test-dir tests/.build --output-on-failure
```

Run only telemetry tests:

```sh
tests/.build/tcp_con_test --gtest_filter='TcpConnectionTest.Telemetry*'
```

For an offline build, add
`-DFETCHCONTENT_SOURCE_DIR_GOOGLETEST=/absolute/path/to/googletest`
to the configure command, pointing to a GoogleTest 1.17.0 source checkout.

### Coverage

Use Clang and matching LLVM tools. A separate build directory avoids changing
compilers in an existing CMake cache:

```sh
cmake -S tests -B tests/.build/coverage \
  -DCMAKE_C_COMPILER=clang -DCMAKE_CXX_COMPILER=clang++ -DENABLE_COVERAGE=ON
cmake --build tests/.build/coverage --parallel
LLVM_PROFILE_FILE=tests/.build/coverage/tests.profraw tests/.build/coverage/tcp_con_test
llvm-profdata merge -sparse tests/.build/coverage/tests.profraw -o tests/.build/coverage/tests.profdata
llvm-cov report tests/.build/coverage/tcp_con_test -instr-profile=tests/.build/coverage/tests.profdata "$PWD/tcp_con.c"
```

On macOS, prefix the last two commands with `xcrun` if LLVM tools are not on
`PATH`. The report measures only `tcp_con.c`, including `main`. Run the full
suite when checking the 90% coverage target.

### Comparing frameworks

Both prototypes use the same basic design: a shared `io` object, test-only
I/O replacements, and a fixture that resets state. GoogleTest uses
`TEST_F`, `EXPECT_EQ`, and `ASSERT_EQ` in place of doctest's
`TEST_CASE_FIXTURE`, `CHECK`, and `REQUIRE`. Setup failures use a plain loop;
other scenarios use individually named tests. There is no GoogleMock,
parameterized fixture, or shell runner.

GoogleTest needs a compiled dependency; doctest used a single bundled header.
The earlier prototype remains in Git history for comparison.

>>>>>>> Stashed changes
## Getting Started

1. Edit the .gemspec file fields: name, summary, description, authors, email, and homepage
1. Update the LICENSE.md file with your company name

## Building non-tool / widget plugins

1. <Path to COSMOS installation>/openc3.sh cli rake build VERSION=X.Y.Z (or openc3.bat for Windows)
   - VERSION is required
   - gem file will be built locally

## Building tool / widget plugins using a local Ruby/Node/pnpm/Rake Environment

1. pnpm install --frozen-lockfile --ignore-scripts
1. rake build VERSION=1.0.0

## Building tool / widget plugins using Docker and the openc3-node container

If you don’t have a local node environment, you can use our openc3-node container to build custom tools and custom widgets

Mac / Linux:

```
docker run -it -v `pwd`:/openc3/local:z -w /openc3/local docker.io/openc3inc/openc3-node sh
```

Windows:

```
docker run -it -v %cd%:/openc3/local -w /openc3/local docker.io/openc3inc/openc3-node sh
```

1. pnpm install --frozen-lockfile --ignore-scripts
1. rake build VERSION=1.0.0

## Installing into OpenC3 COSMOS

1. Go to the OpenC3 Admin Tool, Plugins Tab
1. Click the install button and choose your plugin.gem file
1. Fill out plugin parameters
1. Click Install

## Contributing

We encourage you to contribute to OpenC3!

Contributing is easy.

1. Fork the project
2. Create a feature branch
3. Make your changes
4. Submit a pull request

Before any contributions can be incorporated we do require all contributors to agree to a Contributor License Agreement

This protects both you and us and you retain full rights to any code you write.

## License

This OpenC3 plugin is released under the MIT License. See [LICENSE.md](LICENSE.md)
