% CPOINT-RUN(1) Version 1.0 | Cpoint User Manuals
%
% August 18, 2023

# NAME

**cpoint-run** - run a cpoint file and ignore shebangs

# SYNOPSIS

| **cpoint-run** \[_file_]

# DESCRIPTION

It will run the cpoint file provided in the args and ignore the shebangs at the start of the file. It is meant to be used in cpoint scripts with "#!/usr/bin/env cpoint-run" at the start.

# OPTIONS

-h,

:   Prints brief usage information.

# BUGS

See GitHub Issues: <https://github.com/Vinz2008/C./issues>

# AUTHORS

Vincent Bidard de la Noë <vincentbidarddelanoe@gmail.com>

# SEE ALSO

**cpoint(1)**, **cpoint-build(1)**