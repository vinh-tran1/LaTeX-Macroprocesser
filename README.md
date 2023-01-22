# LaTeX-Macroprocesser

## TeX (or it's later evolution LaTeX) is a program used often in academia to write technical papers and documents. Users define macros in text files that also contain the contents of the document, and TeX processes macros and string expands them based on their definitions.

Implement a TeX-like macro processor in C. This macro processor will perform a transform on a set of input files (or the standard input when no files are specified) and output the result to the standard output. As the input is read, your program will replace macro strings according to the macro’s value mapping and the macro expansion rules. Any input file(s) are provided as command line arguments. The execution of your program on the command line should have this form:

proj1 [file]*
Macros always start with an unescaped backslash followed by a name string. Macros have optional arguments. Each argument is placed in curly braces immediately after the macro name. For example:

\NAME{ARGUMENT 1}{ARGUMENT 2}{ARGUMENT 3}
Here's a brief example of an input/output execution:

Input	Output
A list of values:
\def{MACRO}{VALUE = #}
\MACRO{1}
\MACRO{2}
\MACRO{3}
\MACRO{4}
\MACRO{5}
\MACRO{6}
\MACRO{7}
A list of values:\n

VALUE = 1
VALUE = 2
VALUE = 3
VALUE = 4
VALUE = 5
VALUE = 6
VALUE = 7
The \def macro defines a new macro called \MACRO, Future occurrences of \MACRO will be replaced with VALUE = # where the # is replaced with the argument to \MACRO. We will go into more detail on \def in the section below.

Some notes about the general macro grammar:

Macro names must only contain a string of letters, numbers.
No white space is allowed between a macro name and the arguments or between the arguments.
With a few exceptions, macro arguments can contain arbitrary text, including macro expressions or fragments of macro expressions. The exceptions are for built-in macros, see the section below.
Macro arguments must be brace balanced (i.e., the number of unescaped left braces is greater than or equal to the number of unescaped right braces in every prefix and equal in the entire string). For example:
\valid{this arg {{ is }} balanced}
With a few exceptions (see the built-in macros below), macro arguments can contain escape characters (backslashes). The details on how escape characters should be handled is given in a section below.
