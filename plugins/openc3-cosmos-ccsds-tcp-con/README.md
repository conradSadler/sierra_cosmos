# OpenC3 COSMOS Plugin

See the [OpenC3](https://openc3.com) documentation for all things OpenC3.

Update this comment with your own description.

## TCP server tests

From this plugin directory, run:

```sh
bash tests/run.sh
bash tests/run.sh --coverage
```

The tests use the bundled `doctest.h`, Clang, and a C++11 compiler. Coverage
additionally needs `llvm-profdata` and `llvm-cov` (found through `xcrun` on
macOS). The report measures only `tcp_con.c`, including `main`, and writes
an annotated HTML report to `tests/coverage/index.html`.

`tcp_con.c` is compiled as C with a test-only header substituting socket,
temperature-file, and sleep calls. No Raspberry Pi, network access, server,
or external test dependency is needed. Real temporary files exercise parsing
and command output without changing `flag.txt`. Tests cover setup failures,
packet bytes, partial sends, interrupted sends, fragmented reads, error returns,
and server-loop cleanup. Additional arguments are passed to doctest, for example
`bash tests/run.sh --test-case='*telemetry*'`.

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
