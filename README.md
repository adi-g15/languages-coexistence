# Crypto... Testbed

<div align=center>
  <a href="https://www.codefactor.io/repository/github/adi-g15/crypto-testbed"><img src="https://www.codefactor.io/repository/github/adi-g15/crypto-testbed/badge" alt="CodeFactor" /></a>
  <a href="https://www.codacy.com/gh/adi-g15/crypto-testbed/dashboard?utm_source=github.com&amp;utm_medium=referral&amp;utm_content=adi-g15/crypto-testbed&amp;utm_campaign=Badge_Grade"><img src="https://app.codacy.com/project/badge/Grade/d85c430d83bf477c9ca14223c76eaaaf"/></a>
  <a href="https://github.com/adi-g15/crypto-testbed/actions/workflows/cmake.yml"><imt src="https://github.com/adi-g15/crypto-testbed/actions/workflows/cmake.yml/badge.svg"/></a>
</div>

Coexistence of C++, Rust, JavaScript and Python.
And multiple ways of calling functions from different languages:

* C++ <-> Rust : FFI (using cbindgen)
* C++ <-> C++  : ZeroMQ (networking; client-server)
* C++ -> Python : FFI (using pybind11)
* C++ <-> JavaScript : gRPC (networking; server-server (the C++ server contacting gRPC server so then a client actually))

> CMake builds Cargo (Not exactly a language, but this was new for me, using a CMake script to build rust code triggered by CMake)

Run `client --help` for help.

![Design Plan](design.png)

## Testbed

This is a testbed application (originally wasn't), so it's INTENTIONALLY much more complex than it should be... though as always I did start very simple at beginning.

For me it's a place to try some **co-existence** things:

* C++ code calls methods in Rust; Used build.rs for the first time in rust to create the bindings
* CMake builds rust code too, first time
* Rust code and C++ both using OpenSSL (Initially was doing that part in Rust, then wanted some fun... C++)... was a first time working with OpenSSL in C++
* Setup networking with libzmq, first time doing so in a C++ app
* Used argparse library, first time in C++, though have used different CLI parsers earlier

Some things I have done before, but those that add a cherry on top of the cake
* Use protocol buffers for serialisation/deserialisation
* Using CPM.cmake... Iske baare me kya hi bole, bahut gadar hai, bahut help karta hai

## Building

Since it uses a variety of tech... you may expect some dependencies, though for most developers you must have installed these sometime

Firstly install openssl, protobuf, zeromq (`libssl-dev`, `protobuf-compiler`, `libzmq3-dev` in apt)

> NOTE: On debian systems atleast, you must install `pkg-config` too, it's required by CMake to search OpenSSL

> Considering you have `python3` (with python3-distutils, python3-dev), `cargo` (rust), and `nodejs` (javascript)... 

```sh
cmake -B build
cmake --build build
```

Then to run:
* Server: `./build/server`
* Client: `./build/client --help`

> 2. Basic study of cryptographic concepts and practising them with either javascript/python/rust libraries or Linux command CLI(sha256sum, base64, openssl),
>
>    Encoding/Decoding
>
>    Hashing
>
>    Digital Signatures
>
>    Encryption/Decryption
>

Extras Planned:
> * Serialisation/Deserialisation (using protobuf)
>
> * Digital Certificates

