# GSysMon

A GTK system monitor, works by reading `/proc`.  


## Usage

_This project is at its very early stage of development._  


## Test

For module at `src/[module].c`, run `make test_[module]` at project root.  

```console
$ make
💬 Building utils.o ...
💬 Building test for src/system.c ...
💬 Running test_system ...
Testing sysmon_get_hostname():
   SUCCESS : smdsbz-Z2-Series-1050-SW
Testing sysmon_get_uptime():
   SUCCESS : uptime: 8613.600, idle: 65882.240
```
