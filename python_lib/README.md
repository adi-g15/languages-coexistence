# Python Lib and Flask server

* lib.py - General functions, used through:
		* flask server
		* FFI (interfacing from C++ with pybind)

* main.py - Flask server, this is a basic server that listens for HTTP requests... and all internal working using the lib.py functions

## pipenv

> Here it's suggested to use pipenv for projects: https://packaging.python.org/tutorials/managing-dependencies/

* To install deps -> `pipenv install <dep>`
* To activate this project's virtualenv -> `pipenv shell`.
* Alternatively, run a command inside the virtualenv with pipenv run

### Dropped `complexities`

* I was thinking of using Rust FFI, https://bheisler.github.io/post/calling-rust-in-python/ as a reference.
But it would just be for calculating hash... so I had to drop because it made it "look" more complex (ie. more code, which isn't the goal)
* For the above... I had created a `rust-dyn-lib` crate (which 'would' generate dynamic lib, for using with Python ffi.dlopen), that used the `rust-lib` crate itself (which generates staticlib, for using with C++), reference was https://www.reddit.com/r/rust/comments/4lgb2o/newbie_question_multiple_library_crates_in_a/, and https://github.com/callahad/python-rust-ffi/blob/master/burn/burn.py

