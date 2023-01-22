# LaTeX-Macroprocesser

## TeX (or it's later evolution LaTeX) is a program used often in academia to write technical papers and documents. Users define macros in text files that also contain the contents of the document, and TeX processes macros and string expands them based on their definitions.

Implement a TeX-like macro processor in C. This macro processor will perform a transform on a set of input files (or the standard input when no files are specified) and output the result to the standard output. As the input is read, your program will replace macro strings according to the macro’s value mapping and the macro expansion rules. Any input file(s) are provided as command line arguments. The execution of your program on the command line should have this form: <br />

proj1 [file]*
Macros always start with an unescaped backslash followed by a name string. Macros have optional arguments. Each argument is placed in curly braces immediately after the macro name. For example: <br />

\NAME{ARGUMENT 1}{ARGUMENT 2}{ARGUMENT 3}
Here's a brief example of an input/output execution: <br />

### Input	Output
A list of values: <br />
\def{MACRO}{VALUE = #} <br />
\MACRO{1} <br />
\MACRO{2} <br />
\MACRO{3} <br /> 
\MACRO{4} <br />
\MACRO{5} <br />
\MACRO{6} <br />
\MACRO{7} <br />
A list of values: <br />

VALUE = 1 <br />
VALUE = 2 <br />
VALUE = 3 <br />
VALUE = 4 <br />
VALUE = 5 <br />
VALUE = 6 <br />
VALUE = 7 <br />
The \def macro defines a new macro called \MACRO, Future occurrences of \MACRO will be replaced with VALUE = # where the # is replaced with the argument to \MACRO. We will go into more detail on \def in the section below. <br />

#### Some notes about the general macro grammar: <br />

1. Macro names must only contain a string of letters, numbers.
2. No white space is allowed between a macro name and the arguments or between the arguments.
3. With a few exceptions, macro arguments can contain arbitrary text, including macro expressions or fragments of macro expressions. The exceptions are for built-in macros, see the section below.
4. Macro arguments must be brace balanced (i.e., the number of unescaped left braces is greater than or equal to the number of unescaped right braces in every prefix and equal in the entire string). For example:
\valid{this arg {{ is }} balanced}
5. With a few exceptions (see the built-in macros below), macro arguments can contain escape characters (backslashes). The details on how escape characters should be handled is given in a section below.
