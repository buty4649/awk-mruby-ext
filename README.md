# awk-mruby-ext
A lightweight extension to integrate mruby scripting within GNU awk, providing enhanced scripting capabilities and flexibility.

## Usage

First, download the `mruby.so`.

```
$ wget https://github.com/buty4649/awk-mruby-ext/releases/download/v1.0.0/mruby.so

# optional: Verify the checksum
$ wget https://github.com/buty4649/awk-mruby-ext/releases/download/v1.0.0/checksums.txt
$ sha256sum -c checksums.txt
mruby.so: OK
```

Next, run the `awk` command. Specify the `-l` option to load `mruby.so`.
```
$ awk -l./mruby.so 'BEGIN{print mruby_eval("MRUBY_VERSION")}'
3.3.0
```

`mruby.so` adds the `mruby_eval` function. This function evaluates the Ruby code passed as an argument in the mruby VM and returns the result as a string. Even if the evaluation result is Numeric or Hash, it returns it as a string. This is a current limitation of `mruby.so`.

## Environment

The following environment has been confirmed to work:

* Ubuntu 24.04 / GNU Awk 5.2.1, API 3.2

## Build Instructions

The following tools are required for building:

* gcc
* make
* CRuby / rake

After installing the tools, run the following commands:

```
$ git clone https://github.com/buty4649/awk-mruby-ext/
$ cd awk-mruby-ext
$ make
```
