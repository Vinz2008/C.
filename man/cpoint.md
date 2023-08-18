% HELLO(1) Version 1.0 | Hello World User Manuals
%
% December 17, 2021

# NAME

**hello** - prints Hello, World!

# SYNOPSIS

| **hello** \[**-o**|**\--out** _file_] \[_dedication_]
| **hello** \[**-h**|**\--help**|**-v**|**\--version**]

# DESCRIPTION

Prints "Hello, _dedication_!" to the terminal. If no dedication is
given, uses the default dedication. The default dedication is chosen by
the following sequence:

 1. Using the environment variable *DEFAULT_HELLO_DEDICATION*
 2. Using the per-user configuration file, *~/.hellorc*
 3. Using the system-wide configuration file, */etc/hello.conf*
 4. Finally, using "world".

# OPTIONS

-h, \--help

:   Prints brief usage information.

-o *FILE*, \--output *FILE*

:   Outputs the greeting to the given filename.

-v, \--version

:   Prints the current version number.

# ENVIRONMENT

**DEFAULT_HELLO_DEDICATION**

:   The default dedication if none is given. Has the highest precedence
    if a dedication is not supplied on the command line.

# FILES

*~/.hellorc*

:   Per-user default dedication file.

*/etc/hello.conf*

:   Global default dedication file.

# BUGS

See GitHub Issues: <https://github.com/[owner]/[repo]/issues>

# AUTHORS

Foobar Goodprogrammer <foo@example.org>

# SEE ALSO

**hi(1)**, **hello(3)**, **hello.conf(4)**