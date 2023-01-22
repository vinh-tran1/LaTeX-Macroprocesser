# LaTeX-Macroprocesser

## TeX (or it's later evolution LaTeX) is a program used often in academia to write technical papers and documents. Users define macros in text files that also contain the contents of the document, and TeX processes macros and string expands them based on their definitions.

## Introduction:
Implement a TeX-like macro processor in C. This macro processor will perform a transform on a set of input files (or the standard input when no files are specified) and output the result to the standard output. As the input is read, your program will replace macro strings according to the macro’s value mapping and the macro expansion rules. Any input file(s) are provided as command line arguments. The execution of your program on the command line should have this form: <br />

proj1 [file]*
Macros always start with an unescaped backslash followed by a name string. Macros have optional arguments. Each argument is placed in curly braces immediately after the macro name. For example: **\NAME{ARGUMENT 1}{ARGUMENT 2}{ARGUMENT 3}** <br /> 

### Here's a brief example of an input/output execution: <br />

#### Input	Output
**A list of values:** <br />
\def{MACRO}{VALUE = #} <br />
\MACRO{1} <br />
\MACRO{2} <br />
\MACRO{3} <br /> 
\MACRO{4} <br />
\MACRO{5} <br />
\MACRO{6} <br />
\MACRO{7} <br />

**A list of values:** <br />

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

## Built-In Macros
Implement a set of built-in macros in the macro processor you are building.  The programmer can use these special macros in the source files to define/undefine new macros, include text from other files, do comparisons, etc. These are listed below: <br />

### \def allows a programmer to define a new macro mapping:
#### \def{NAME}{VALUE}
The entire \def macro and arguments are always replaced by the empty string.  The argument NAME must be a nonempty alphanumeric string (can be arbitrarily long).  As usual, the VALUE argument must be brace balanced, but can contain arbitrary text. After processing a \def macro  and  its  arguments,  the NAME argument  is  now  mapped  to  the VALUE argument. In the future, macros with that name are valid: the \NAME{ARG} macro should be replaced by VALUE—with each occurrence of the unescaped character # replaced by the argument string (ARG). Note: custom defined macros must always have exactly one argument, if the VALUE doesn’t have any unescaped # characters, the argument is ignored.

### \undef undefines  previously defined macro. \undef is replaced by the empty string:
#### \undef{NAME}

### \if allows text to be processed conditionally (like an if-then-else block). Your implementation should consider false as the empty string  and true for any non-empty string. The \if macro should have the following form:
#### \if{COND}{THEN}{ELSE}
Like all macros, all three arguments can contain arbitrary text, but must be braced balanced.  You should not expand COND. The functionality should be this: the entire \if macro including arguments should be replaced with either the THEN or ELSE depending on the size of COND. Then, after expansion, processing resumes at the beginning of the replacement string.

### \ifdef is similar to \if; it expands to either THEN or ELSE:
#### \ifdef{NAME}{THEN}{ELSE}
The main difference is with the condition argument, NAME, which is restricted to alphanumeric characters. If NAME matches a currently defined macro name then the condition is true, otherwise it is false.

### \include macros are replaced by the contents of the file PATH. Typical brace balancing rules apply here:
#### \include{PATH}

### \expandafter has the form:
#### \expandafter{BEFORE}{AFTER}
The point of this macro is to delay expanding the before argument until the after argument has been expanded. The output of this macro expansion is simply BEFORE immediately followed by the expanded AFTER. Note that this changes the recursive evaluation rule, i.e. you should eagerly expand the all macros in the AFTER string before touching BEFORE. This means that any new macros defined in AFTER should be in scope in for the BEFORE. You may not use additional processes/threads to accomplish these actions. Here’s an example program:

## Input Output Example
#### Input
\def{B}{buffalo} <br />
\expandafter{\B{}}{\undef{B}\def{B}{bison}} <br />
#### Output
bison <br />

Why is this the case? It is because \B{} is expanded after it has be redefined in the after argument. 
 
## Comments
The comment character, %, should cause your program to ignore it and all subsequent characters up to the first non-blank, non-tab character following the next newline or the end of the current file, whichever comes first. This convention applies only when reading characters from the file(s) specified on the command line (or the standard input if none is specified) or from an included file. Comments should be removed only once from each file or from standard input. After all inputs are read and comments are removed, then you should start expanding.  Note: the comment character can be escaped, see the section below.

## Escape Characters
Besides being used as the “start” character for a macro, a \ character can also be used to escape one of the following special characters \, #, %, {, } so that it is not treated as a special character.  For these characters the effect of the \ is preserved until it is about to be output, at which point it is suppressed and the \, #, %, {, } is output instead. In effect, the \ is ignored and the following character is treated as a non-special character thereafter.  That is, in effect the pair of characters can be treated as a macro with no arguments until it is expanded and output. We then have the following cases:

1. Escape character followed by \, #, %, {, }:  for this case use the rule above (i.e., when it is time to output, only print the second character).
2. Escape character followed by an alphanumeric character: in this case we must be reading a macro, so all the macro parsing rules apply.
3. Escape character followed by non-alphanumeric and not \, #, %, {, }:  in this case these characters have no special meaning to your parser (i.e., they should both be output).
 

## Error Detection
The following kinds of errors should be detected:

1. Parsing Errors.  For example, in a \def macro, if NAME is not a nonempty alphanumeric string. Another example would be if a macro name is not immediately followed by an argument in wrapped in balanced curly braces, or more generally: if a macro has too few arguments.
2. Semantic Errors. For example, a macro name is not defined. Another example, would be an attempt to redefine a macro before undefining it, or an attempt to undefine a nonexistent macro.
3. Library Errors. You should consider how errors may be returned by any library functions you use (malloc, fopen, read, etc.) and detect them.
4. Note that correct usage of malloc, calloc, etc., should not result in any library errors due to running out of memory.
For these kinds of errors your program should write a one-line message to stderr and exit. If you detect an error, you should not output a partial evaluation of any input. 
