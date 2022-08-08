# fundamentals-of-system-programming
This is my practical work on subject: File I/O and dynamic libraries. 
ITMO University forth semester of study on SIT program.
---
# Technical Specification
Develop on C language for OS Linux:
- a program that allows you to perform a recursive search for files, starting from the specified directory, using dynamic (shared) plug-in libraries;
- a dynamic library that implements the search for files containing a given single-precision real number in binary (little-endian or big-endian) form, i.e. in the form of four consecutive octets with corresponding values. A real number can be written in conventional or scientific formats. 
**Option:** `--float-bin <value>` 
**Example:** `--float-bin 2.71`

The program should be a console utility, which is configured by passing arguments in the startup line and/or using environment variables.:
`lab1kamN3247 [[ options ] directory]`

The program must perform a recursive search for files that meet the criteria that are set by options on the command line. The available search criteria (and, accordingly, the available options) are determined by the presence in a given directory of dynamic libraries that extend the functionality of the program (hereinafter — plugins).

When running without a directory name to search for, the program displays reference information on options and plugins available at the time of launch. Options supported by the program:
- `-P directory` – directory with plugins (by default, plugins are searched in the same directory where the executable file of the program is located).
- `-A` –  Combining options with "AND" operator (works by default).
- `-O` –  Combining options with "OR" operator.
- `-N` –  Inverting the search term (after combining plugin options with -A or -O).
- `-v` –  Outputs the program version and information about the program.
- `-h` –  Outputs help option.

Plugins are dynamic libraries in ELF format with an arbitrary name and extension .so and the plugin_get_info and plugin_process_file interface functions. Checking whether the file meets the specified criteria. A detailed description of the plugins API is contained in the `kamN3247.h` file.

When a file is found that meets the specified search criteria, the full path to this file is output to the standard output stream. When defining the `LAB1DEBUG` environment variable, information about what was found and where in the file should be output to the standard error stream (to make it easier to understand why the file meets the search criteria), and any additional debugging information can also be output.

The project must contain a Makefile and can be built using make, which supports at least two goals: all and clean
