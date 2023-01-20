#include "proj1.h"

typedef struct stringBuff
{
    size_t capacity; //size of data
    size_t size; //current number of elements
    char *str; //string buffer array
} stringBuff;

typedef enum {
    state_plaintext,
    state_escape,
    state_macroname,
    state_def,
    state_value, 
    state_undef,
    state_if,
    state_ifdef,
    state_include,
    state_expandafter,
    state_usermacro
} State;

typedef enum {
    state_plain,
    state_comment,
    state_deleteWhiteSpace
} commentStates;

typedef struct Table {
    stringBuff *key;
    stringBuff *value;
} table;

//-----------------------------GLOBAL VARIABLES-----------------------------//
table **macroArr; int arrIndex = 0; int arrCap = 6; 

//-----------------------------FUNCTIONS-----------------------------//
void stringReverse(stringBuff *file)
{
    char temp; int length = file->size;

    for (int i = 0; i < length/2; i++)
    {
        temp = file->str[i];
        file->str[i] = file->str[length - i - 1];
        file->str[length - i - 1] = temp;
    }
}

//----------------------string lib----------------------//
stringBuff *stringBuffCreate()
{
    stringBuff *buff = (stringBuff *)malloc(sizeof(stringBuff));

    buff->capacity = 32;
    buff->size = 0;
    buff->str = (char *)calloc(buff->capacity, sizeof(char));

    return buff;
}

void stringBuffDestroy(stringBuff *buff)
{
    free(buff->str);
    free(buff);
}

void reallocBuff(stringBuff *buff)
{
    buff->capacity *= 2;
    buff->str = (char *)realloc(buff->str, buff->capacity * sizeof(char));
}

void strPutchar(char c, stringBuff *buff)
{
    if (buff->size > buff->capacity - 2)
        reallocBuff(buff);
       
    buff->str[buff->size] = c;
    if (c != '\0')
    {
        buff->size++;
        buff->str[buff->size] = '\0'; //so that I don't have to keep doing this, this will be overwritten every time
    } 
}

char strPop(stringBuff *buff)
{
    if( buff->size == 0)
        return '\0';

    char c = buff->str[buff->size - 1]; //last index
    buff->str[buff->size - 1] = '\0'; //set to null
    buff->size--;

    return c;
}

//concat string (str) to stringBuffer (buff)
stringBuff *strBuffConcat(char *str, stringBuff *buff)
{
    for (int i = 0; i < strlen(str); i++)
        strPutchar(str[i], buff);
    
    return buff; 
}

//concat stringBuffer (buff2) to stringBuffer (buff1)
stringBuff *buffBuffConcat(stringBuff *buff1, stringBuff *buff2)
{
    char c;
    while((c = strPop(buff2)) != '\0')
    {
        strPutchar(c, buff1);
    }
    return buff1; 
}

//handles # in usermacro expansion
void substPound(stringBuff *inputBuff, stringBuff *buff, char *str)
{
    int len = buff->size; int strlength = strlen(str);
    stringBuff *tmp = stringBuffCreate();

    int escapeFlag = 0; 

    for (int i = 0 ; i < len ; i++)
    {
        if (buff->str[i] == '\\' && escapeFlag == 0)
        {
            escapeFlag = 1;
            strPutchar(buff->str[i], tmp);
        }
           
        else
        {
            if (escapeFlag == 1)
            {
                if (buff->str[i] == '\\' || buff->str[i] == '#' || buff->str[i] == '%' || buff->str[i] == '{' || buff->str[i] == '}')
                    strPutchar(buff->str[i], tmp);
                else 
                     strPutchar(buff->str[i], tmp);
                escapeFlag = 0;
            }
                
            else
            {
                if (buff->str[i] == '#')
                    for (int i = 0; i < strlength; i++)
                        strPutchar(str[i], tmp);

                else
                    strPutchar(buff->str[i], tmp);
            } 
        }
    }
 
    buffBuffConcat(inputBuff, tmp);
    stringBuffDestroy(tmp);
}

void buffReset(stringBuff *buff)
{
    buff->size = 0;
    buff->str[0] = '\0';
}

//-----------------------------TABLE FUNCTIONS-------------------//

table *tableCreate()
{
    table *t = (table *)malloc(sizeof(table));
    t->key = stringBuffCreate();
    t->value = stringBuffCreate();

    return t; 
}

void tableUndef(table *arr)
{
    stringBuffDestroy(arr->key);
    stringBuffDestroy(arr->value);

    free(arr);
}

void tableDestroy(table **arr, int size)
{
    for (int i = 0; i < size; i++)
        tableUndef(arr[i]);
        
    free(arr);     
}

void loadKey(table *arr, stringBuff *argBuffer)
{
    int len = argBuffer->size;
    for (int i = 0; i < len; i++)
        strPutchar(argBuffer->str[i], arr->key);
}

void loadValue(table *arr, stringBuff *argBuffer)
{
    int len = argBuffer->size;
    for (int i = 0; i < len; i++)
        strPutchar(argBuffer->str[i], arr->value);
}

//-------------------------------------------------------------------//

stringBuff *removeComments(FILE *f)
{
    stringBuff *file = stringBuffCreate();
    commentStates state = state_plain;
    int c; int escapeFlag = 0;

    while ((c = getc(f)) != EOF)
    {
        switch(state)
        {
            case state_plain: 
                if (c == '%' && escapeFlag == 0)
                    state = state_comment;
                else
                {
                    if (c == '\\') //checks to see if there is an escape char
                        escapeFlag = 1;
                    else  
                        escapeFlag = 0;
                    strPutchar((char) c, file); 
                        
                }  
                break;

            case state_comment: 
                if (c == '\n')
                    state = state_deleteWhiteSpace;
                break;
            
            case state_deleteWhiteSpace:
                if (c == '%')
                    state = state_comment;
                    
                else if(!isblank(c) || c == '\n') //accounts for another new line
                {
                    strPutchar((char) c, file);
                    state = state_plain;
                }   
                break;
        }
    }

    //strings in c end in null character
    strPutchar('\0', file);
    stringReverse(file);

    return file;
}

stringBuff *expand(stringBuff *inputBuffer)
{
    int c; int bracketCount = -1; int bracketCountIsZero = 0;
    int exists = 0; int escapeFlag = 0; int checkOnce = 0;
    stringBuff *fileBuff; FILE *file;
    State state = state_plaintext;

    stringBuff *outputBuffer = stringBuffCreate();
    stringBuff *macroNameBuffer = stringBuffCreate();
    stringBuff *argBuffer = stringBuffCreate();
    stringBuff *arg2Buffer = stringBuffCreate();
    stringBuff *arg3Buffer = stringBuffCreate();

    while((c = strPop(inputBuffer)) != '\0') // or should I do inputBuffer->size >= 0 -- added this size check
    {
        switch(state)
        {
            case state_plaintext:
                if (c == '\\')
                {
                    buffReset(macroNameBuffer);
                    state = state_escape;
                }

                else 
                    strPutchar(c, outputBuffer);
                    
                break;

            case state_escape:
                if (c == '\\' || c == '#' || c == '%' || c == '{' || c == '}')
                {
                    strPutchar(c, outputBuffer);
                    state = state_plaintext;
                }

                else if (isalnum(c))
                {
                    strPutchar(c, macroNameBuffer);
                    state = state_macroname;
                } 

                else
                {
                    strPutchar('\\', outputBuffer);
                    strPutchar(c, outputBuffer);
                    state = state_plaintext;
                }

                break;
                
            case state_macroname: //has to be alphanumeric and no spaces
                if (isalnum(c))
                    strPutchar(c, macroNameBuffer);

                else if (c == '{')
                {
                    bracketCount++;

                    //check for built in macros
                    if (strcmp(macroNameBuffer->str, "def") == 0) 
                        state = state_def;   
                    else if (strcmp(macroNameBuffer->str, "undef") == 0)
                        state = state_undef;
                    else if (strcmp(macroNameBuffer->str, "if") == 0)
                        state = state_if;
                    else if (strcmp(macroNameBuffer->str, "ifdef") == 0)
                        state = state_ifdef;
                    else if (strcmp(macroNameBuffer->str, "include") == 0)
                        state = state_include;
                    else if (strcmp(macroNameBuffer->str, "expandafter") == 0)
                        state = state_expandafter;
                    else //checks for user defined macros
                    {
                        for (int j = 0; j < arrIndex; j++)
                        {
                            if (strcmp(macroNameBuffer->str, macroArr[j]->key->str) == 0)
                            {
                                strBuffConcat(macroArr[j]->value->str, argBuffer);
                                state = state_usermacro;
                                break;
                            } 
                        }

                        if (state == state_macroname)
                            DIE("%s", "ERROR: Macro does not exist"); 
                    }
                }

                else if(isblank(c))
                    DIE("%s", "ERROR: Cannot have space between macroname and argument"); 

                break;

            case state_def: //alphanumeric + no spaces in b/w
                if (c == '}' && bracketCount == 0)
                {
                    if (argBuffer->size == 0)
                        DIE("%s", "ERROR: Def argument cannot be empty");
                    
                    //checks for already defined macronames
                    for (int i = 0; i < arrIndex; i++)
                        if (strcmp(argBuffer->str, macroArr[i]->key->str) == 0)
                             DIE("%s", "ERROR: Macro already defined");
                        
                    //dynamic realloc of marcoArr struct (table **)
                    if (arrIndex > arrCap - 2)
                    {
                        arrCap *= 2;
                        macroArr = (table **)realloc(macroArr, arrCap * sizeof(table *));
                    }

                    //create new struct per new index
                    macroArr[arrIndex] = tableCreate(); 
                    loadKey(macroArr[arrIndex], argBuffer);

                    buffReset(argBuffer);
                    bracketCount--;
                }

                //hit the end of {NAME}, go to value state
                else if (c == '{' && bracketCount == -1)
                {
                    bracketCount++;    
                    state = state_value;  
                }

                else if (isalnum(c))
                    strPutchar(c, argBuffer);
                  
                else
                    DIE("%s", "ERROR: Name argument must be alphanumeric/can't have spaces between args");
                    
                break;

            case state_value: //not really any restrictions, just store everything in array
                if (c == '}' && bracketCount == 0)
                {
                    loadValue(macroArr[arrIndex], argBuffer);
                    arrIndex++;

                    buffReset(argBuffer);
                    bracketCount--;
                    
                    state = state_plaintext;
                }

                else
                {
                    if (c == '{')
                        bracketCount++;
                    else if (c == '}')
                        bracketCount--;

                    strPutchar(c, argBuffer);
                }

                break;

            case state_usermacro: //expands user macro but putting it back onto input
                if (c == '}' && bracketCount == 0 && escapeFlag == 0)
                {
                    bracketCount--;
                    substPound(inputBuffer, argBuffer, arg2Buffer->str);
     
                    buffReset(argBuffer);
                    buffReset(arg2Buffer);
                    escapeFlag = 0; 

                    state = state_plaintext;
                }

                else if (c == '\\' && escapeFlag == 0)
                    escapeFlag = 1;

                else
                {
                    if (escapeFlag == 1)
                    {
                        if (c == '\\' || c == '#' || c == '%' || c == '{' || c == '}')
                            strPutchar(c, arg2Buffer);
                        escapeFlag = 0;
                    }
                        
                    else
                    {
                        if (c == '{')
                            bracketCount++;
                        else if (c == '}')
                            bracketCount--;

                        strPutchar(c, arg2Buffer);
                    } 
                }
              
                break;
            
            case state_undef: 
                if (c == '}' && bracketCount == 0)
                {
                    bracketCount--;
                     //deletes the index of specified key
                    for (int i = 0; i < arrIndex; i++)
                    {
                        if (strcmp(argBuffer->str, macroArr[i]->key->str) ==0)
                        {
                            tableUndef(macroArr[i]);
                            arrIndex--;
                            macroArr[i] = macroArr[arrIndex];
                            exists = 1;
                            break;     
                        }
                    }
                    
                    if (exists == 0)
                        DIE("%s", "ERROR: cannot undef a macro that hasn't been defined");

                    buffReset(argBuffer);
                    exists = 0;
                    state = state_plaintext;
                }

                else
                    strPutchar(c, argBuffer);
                    
                break;

            case state_if: //no spaces in b/w arguments
                if (c == '}' && bracketCount == 0)
                {
                    checkOnce = 0;
                    bracketCount--;
                    bracketCountIsZero++;
                }
                    
                else if (bracketCountIsZero == 0)
                {
                    if (c == '{')
                        bracketCount++;
                    else if (c == '}')
                        bracketCount--;
                    
                    strPutchar(c, argBuffer);
                }
                else if (bracketCountIsZero == 1)
                {
                    if (isblank(c) && checkOnce == 0)
                        DIE("%s", "ERROR: Can't have spaces inbetween args for if macro");
                    else if (checkOnce == 0)
                        checkOnce = 1;

                    if (c == '{' && bracketCount == -1)
                        bracketCount++;
                    else
                    {
                        if (c == '{')
                            bracketCount++;
                        else if (c == '}')
                            bracketCount--;
                        strPutchar(c, arg2Buffer);
                    }
                        
                }
                else if (bracketCountIsZero == 2)
                {
                    if (isblank(c) && checkOnce == 0)
                        DIE("%s", "ERROR: Can't have spaces inbetween args for if macro");
                    else if (checkOnce == 0)
                        checkOnce = 1;

                    if (c == '{' && bracketCount == -1)
                        bracketCount++;
                    else
                    {
                        if (c == '{')
                            bracketCount++;
                        else if (c == '}')
                            bracketCount--;
                        strPutchar(c, arg3Buffer);
                    }
                        
                }
                else if (bracketCountIsZero == 3)
                {
                    strPutchar(c, inputBuffer); //puts the current character back on input so we don't miss it
                    if (argBuffer->size == 0) //if {cond} is empty
                        buffBuffConcat(inputBuffer, arg3Buffer);
                    else
                        buffBuffConcat(inputBuffer, arg2Buffer);

                    buffReset(argBuffer);
                    buffReset(arg2Buffer);
                    buffReset(arg3Buffer);
                    bracketCountIsZero = 0;
                    
                    state = state_plaintext;
                }

                break;
            
            case state_ifdef: //no spaces in b/w args
                if (c == '}' && bracketCount == 0)
                {
                    checkOnce = 0;
                    bracketCount--;
                    bracketCountIsZero++;
                }

                else if (bracketCountIsZero == 0)
                {
                    if (c == '{')
                        bracketCount++;
                    else if (c == '}')
                        bracketCount--;

                    strPutchar(c, argBuffer);
                }
                else if (bracketCountIsZero == 1)
                {
                    if (isblank(c) && checkOnce == 0)
                        DIE("%s", "ERROR: Can't have spaces inbetween args for ifdef macro");
                    else if (checkOnce == 0)
                        checkOnce = 1;

                    if (c == '{' && bracketCount == -1)
                        bracketCount++;
                    else
                    {
                        if (c == '{')
                            bracketCount++;
                        else if (c == '}')
                            bracketCount--;
                        strPutchar(c, arg2Buffer);
                    }
                }
                else if (bracketCountIsZero == 2)
                {
                    if (isblank(c) && checkOnce == 0)
                        DIE("%s", "ERROR: Can't have spaces inbetween args for if macro");
                    else if (checkOnce == 0)
                        checkOnce = 1;

                    if (c == '{' && bracketCount == -1)
                        bracketCount++;
                    else
                    {
                        if (c == '{')
                            bracketCount++;
                        else if (c == '}')
                            bracketCount--;
                        strPutchar(c, arg3Buffer);
                    }
                }
                else if (bracketCountIsZero  == 3)
                {
                    int macro = 0;
                    strPutchar(c, inputBuffer); //puts the current character back on input so we don't miss it

                    for (int j = 0; j < arrIndex; j++) //checks to see if macro is defined
                    {
                        if (strcmp(argBuffer->str, macroArr[j]->key->str) == 0)
                        {
                            buffBuffConcat(inputBuffer, arg2Buffer);
                            macro = 1;
                            break;
                        } 
                    }

                    if (macro == 0)
                        buffBuffConcat(inputBuffer, arg3Buffer);

                    buffReset(argBuffer);
                    buffReset(arg2Buffer);
                    buffReset(arg3Buffer);
                    bracketCountIsZero = 0;

                    state = state_plaintext; 
                }

                break;

            case state_include: //open file, remove comments, append to input buffer
                if (c == '}' && bracketCount == 0)
                {
                    bracketCount--;

                    file = fopen(argBuffer->str, "r");
                    if (file == NULL)
                        DIE("%s", "ERROR: File does not exist");
                        
                    fileBuff = removeComments(file);

                    while((c = strPop(fileBuff)) != '\0')
                        strPutchar(c, arg2Buffer);

                    stringBuffDestroy(fileBuff);
                    buffBuffConcat(inputBuffer, arg2Buffer);
                    buffReset(argBuffer);
                    buffReset(arg2Buffer);
                    fclose(file);

                    state = state_plaintext;
                }

                else
                {
                    if (c == '{')
                        bracketCount++;
                    else if (c == '}')
                        bracketCount--;

                    strPutchar(c, argBuffer);
                }

                break;

            case state_expandafter:
                if (c == '}' && bracketCount == 0)
                {
                    checkOnce = 0;
                    bracketCount--;
                    bracketCountIsZero++;
                }

                else if (bracketCountIsZero == 0)
                {
                    if (c == '{')
                        bracketCount++;
                    else if (c == '}')
                        bracketCount--;
                    
                    strPutchar(c, argBuffer);
                }
                else if (bracketCountIsZero == 1)
                {
                    if (isblank(c) && checkOnce == 0)
                        DIE("%s", "ERROR: Can't have spaces inbetween args for expandafter macro");
                    else if (checkOnce == 0)
                        checkOnce = 1;

                    if (c == '{' && bracketCount == -1)
                        bracketCount++;
                    else
                    {
                        if (c == '{')
                            bracketCount++;
                        else if (c == '}')
                            bracketCount--;
                        strPutchar(c, arg2Buffer);
                    }
                }
                else if (bracketCountIsZero == 2)
                {
                    strPutchar(c, inputBuffer);

                    stringBuff *tempInput = stringBuffCreate(); //need a copy of arg2 because it will get freed in the recursive call
                    tempInput = buffBuffConcat(tempInput, arg2Buffer); //buffBuffConcat reverses the tempInput for me already

                    stringBuff *tempArg = expand(tempInput); //now it's in normal order
                    stringReverse(tempArg); //make it backwards again to input it back into input buffer

                    buffBuffConcat(argBuffer, tempArg); //expanded after is now appended to before
                    buffBuffConcat(inputBuffer, argBuffer);

                    stringBuffDestroy(tempArg);
                    buffReset(argBuffer);
                    buffReset(arg2Buffer);
                    bracketCountIsZero = 0;

                    state = state_plaintext; 
                }

                break;
        }
    }
   
    if (state != state_plaintext && state != state_escape)
        DIE("%s", "ERROR: Unclosed argument");

    stringBuffDestroy(inputBuffer);
    stringBuffDestroy(macroNameBuffer);
    stringBuffDestroy(argBuffer);
    stringBuffDestroy(arg2Buffer);
    stringBuffDestroy(arg3Buffer);
    

    return outputBuffer;
}

//------------------MAIN----------------------//
int main(int argc, char **argv)
{
    FILE *f;
    stringBuff *output; 
    macroArr = (table **)calloc(arrCap, sizeof(table *));

    if (argc == 1)
    {
        output = expand(removeComments(stdin));
        printf("%s", output->str); //no arguments in argv - only stdin
    }
       
    else //if there are arguments i.e. files
    {
         for (int i = 1; i < argc; i++)
        {
            f = fopen(argv[i], "r");

            if (f == NULL) //if not file, error
            {
                fprintf(stderr, "Error %s\n", argv[i]);
                return 1;
            }

            else
            {
                output = expand(removeComments(f));
                printf("%s", output->str);
            }
                   
            fclose(f);
        }
    }

    stringBuffDestroy(output);
    tableDestroy(macroArr, arrIndex);

    return 0;
}
