# awk-mruby-ext
A lightweight extension to integrate mruby scripting within GNU awk, providing enhanced scripting capabilities and flexibility.

## Usage

First, download the `mruby.so`.

```
$ wget https://github.com/buty4649/awk-mruby-ext/releases/download/v1.1.0/mruby.so

# optional: Verify the checksum
$ wget https://github.com/buty4649/awk-mruby-ext/releases/download/v1.1.0/checksums.txt
$ sha256sum -c checksums.txt
mruby.so: OK
```

Next, run the `awk` command. Specify the `-l` option to load `mruby.so`.
```
$ awk -l./mruby.so 'BEGIN{print mruby_eval("MRUBY_VERSION")}'
3.3.0
```

`mruby.so` adds the `mruby_eval` and `mruby_set_val` functions.

### `mruby_eval` Function

This function evaluates and executes the Ruby code passed as an argument. After execution, it returns the last evaluated value, converted to the appropriate awk type. However, Array and Hash are converted to strings using the `to_s` method, because awk cannot return Array. If you need to retrieve Array or Hash, refer to the `MRUBY_EVAL_RESULT` variable. Below is an example using `MRUBY_EVAL_RESULT`:

```sh
$ awk -l./mruby.so 'BEGIN{print mruby_eval("{foo:1}"); print MRUBY_EVAL_RESULT["foo"]}'
{:foo=>1}
1
```

### `mruby_set_val` Function

This function sets a value to a variable in the mruby VM. The variable set is always a global variable. The value set to the variable is always a string, and Array is not allowed. For example, the following example sets a variable named `$foo`:

```sh
$ awk -l./mruby.so 'BEGIN{mruby_set_val("$foo","bar");print mruby_eval("$foo")}'
bar
```

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
