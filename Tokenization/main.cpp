#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <map>
using namespace std;


/*{ Sample program
  in TINY language
  compute factorial
}

read x; {input an integer}
if 0<x then {compute only if x>=1}
  fact:=1;
  repeat
    fact := fact * x;
    x:=x-1
  until x=0;
  write fact {output factorial}
end
*/

// sequence of statements separated by ;
// no procedures - no declarations
// all variables are integers
// variables are declared simply by assigning values to them :=
// variable names can include only alphabetic charachters(a:z or A:Z) and underscores
// if-statement: if (boolean) then [else] end
// repeat-statement: repeat until (boolean)
// boolean only in if and repeat conditions < = and two mathematical expressions
// math expressions integers only, + - * / ^
// I/O read write
// Comments {}

////////////////////////////////////////////////////////////////////////////////////
// Strings /////////////////////////////////////////////////////////////////////////

bool Equals(const char* a, const char* b)
{
    return strcmp(a, b)==0;
}

bool StartsWith(const char* a, const char* b)
{
    int nb=strlen(b);
    return strncmp(a, b, nb)==0;
}

void Copy(char* a, const char* b, int n=0)
{
    if(n>0) {strncpy(a, b, n); a[n]=0;}
    else strcpy(a, b);
}

void AllocateAndCopy(char** a, const char* b)
{
    if(b==0) {*a=0; return;}
    int n=strlen(b);
    *a=new char[n+1];
    strcpy(*a, b);
}

////////////////////////////////////////////////////////////////////////////////////
// Input and Output ////////////////////////////////////////////////////////////////

#define MAX_LINE_LENGTH 10000

struct InFile
{
    FILE* file;
    int cur_line_num;

    char line_buf[MAX_LINE_LENGTH];
    int cur_ind, cur_line_size;

    InFile(const char* str) {file=0; if(str) file=fopen(str, "r"); cur_line_size=0; cur_ind=0; cur_line_num=0;}
    ~InFile(){if(file) fclose(file);}

    void SkipSpaces()
    {
        while(cur_ind<cur_line_size)
        {
            char ch=line_buf[cur_ind];
            if(ch!=' ' && ch!='\t' && ch!='\r' && ch!='\n') break;
            cur_ind++;
        }
    }

    bool SkipUpto(const char* str)
    {
        while(true)
        {
            SkipSpaces();
            while(cur_ind>=cur_line_size) {if(!GetNewLine()) return false; SkipSpaces();}

            if(StartsWith(&line_buf[cur_ind], str))
            {
                cur_ind+=strlen(str);
                return true;
            }
            cur_ind++;
        }
        return false;
    }

    bool GetNewLine()
    {
        cur_ind=0; line_buf[0]=0;
        if(!fgets(line_buf, MAX_LINE_LENGTH, file)) return false;
        cur_line_size=strlen(line_buf);
        if(cur_line_size==0) return false; // End of file
        cur_line_num++;
        return true;
    }

    char* GetNextTokenStr()
    {
        SkipSpaces();
        while(cur_ind>=cur_line_size) {if(!GetNewLine()) return 0; SkipSpaces();}
        return &line_buf[cur_ind];
    }

    void Advance(int num)
    {
        cur_ind+=num;
    }
};

struct OutFile
{
    FILE* file;
    OutFile(const char* str) {file=0; if(str) file=fopen(str, "w");}
    ~OutFile(){if(file) fclose(file);}

    void Out(const char* s)
    {
        fprintf(file, "%s\n", s); fflush(file);
    }
};

////////////////////////////////////////////////////////////////////////////////////
// Compiler Parameters /////////////////////////////////////////////////////////////

struct CompilerInfo
{
    InFile in_file;
    OutFile out_file;
    OutFile debug_file;

    CompilerInfo(const char* in_str, const char* out_str, const char* debug_str)
            : in_file(in_str), out_file(out_str), debug_file(debug_str)
    {
    }
};

////////////////////////////////////////////////////////////////////////////////////
// Scanner /////////////////////////////////////////////////////////////////////////

#define MAX_TOKEN_LEN 40

enum TokenType{
    IF, THEN, ELSE, END, REPEAT, UNTIL, READ, WRITE,
    ASSIGN, EQUAL, LESS_THAN,
    PLUS, MINUS, TIMES, DIVIDE, POWER,
    SEMI_COLON,
    LEFT_PAREN, RIGHT_PAREN,
    LEFT_BRACE, RIGHT_BRACE,
    ID, NUM,
    ENDFILE, ERROR
};

map<TokenType,string> mp={
        {IF,"IF"}, {THEN,"THEN"}, {ELSE,"ELSE"}, {END,"END"}, {REPEAT,"REPEAT"}, {UNTIL,"UNTIL"}, {READ,"READ"}, {WRITE,"WRITE"},
        {ASSIGN,"ASSIGN"}, {EQUAL,"EQUAL"}, {LESS_THAN,"LESS_THAN"},
        {PLUS,"PLUS"}, {MINUS,"MINUS"}, {TIMES,"TIMES"}, {DIVIDE,"DIVIDE"}, {POWER,"POWER"},
        {SEMI_COLON,"SEMI_COLON"},
        {LEFT_PAREN,"LEFT_PAREN"}, {RIGHT_PAREN,"RIGHT_PAREN"},
        {LEFT_BRACE,"LEFT_BRACE"}, {RIGHT_BRACE,"RIGHT_BRACE"},
        {ID,"ID"}, {NUM,"NUM"},
        {ENDFILE,"ENDFILE"}, {ERROR,"ERROR"}
};

// Used for debugging only /////////////////////////////////////////////////////////
const char* TokenTypeStr[]=
        {
                "If", "Then", "Else", "End", "Repeat", "Until", "Read", "Write",
                "Assign", "Equal", "LessThan",
                "Plus", "Minus", "Times", "Divide", "Power",
                "SemiColon",
                "LeftParen", "RightParen",
                "LeftBrace", "RightBrace",
                "ID", "Num",
                "EndFile", "Error"
        };

struct Token
{
    TokenType type;
    string str;

    Token(){str[0]=0; type=ERROR;}
    Token(TokenType _type, const string& _str) {type=_type; str=_str;}
};

const Token reserved_words[]=
        {
                Token(IF, "if"),
                Token(THEN, "then"),
                Token(ELSE, "else"),
                Token(END, "end"),
                Token(REPEAT, "repeat"),
                Token(UNTIL, "until"),
                Token(READ, "read"),
                Token(WRITE, "write")
        };
const int num_reserved_words=sizeof(reserved_words)/sizeof(reserved_words[0]);

// if there is tokens like < <=, sort them such that sub-tokens come last: <= <
// the closing comment should come immediately after opening comment
const Token symbolic_tokens[]=
        {
                Token(ASSIGN, ":="),
                Token(EQUAL, "="),
                Token(LESS_THAN, "<"),
                Token(PLUS, "+"),
                Token(MINUS, "-"),
                Token(TIMES, "*"),
                Token(DIVIDE, "/"),
                Token(POWER, "^"),
                Token(SEMI_COLON, ";"),
                Token(LEFT_PAREN, "("),
                Token(RIGHT_PAREN, ")"),
                Token(LEFT_BRACE, "{"),
                Token(RIGHT_BRACE, "}")
        };
const int num_symbolic_tokens=sizeof(symbolic_tokens)/sizeof(symbolic_tokens[0]);

inline bool IsDigit(char ch){return (ch>='0' && ch<='9');}
inline bool IsLetter(char ch){return ((ch>='a' && ch<='z') || (ch>='A' && ch<='Z'));}
inline bool IsLetterOrUnderscore(char ch){return (IsLetter(ch) || ch=='_');}
///////////////////////////////////////////////////////////////////////////////////////
bool isReservedWord(const string& str) {
    for (const Token& word : reserved_words) {
        if (str == word.str) {
            return true;
        }
    }
    return false;
}

bool isSymbolicToken(const string& str) {
    for (const Token& word : symbolic_tokens) {
        if (str == word.str||str==":") {
            return true;
        }
    }
    return false;
}

vector<pair<Token,int>> tokens;

// Inside the tokenize function
void tokenize(const string& inputFileName) {
    ifstream inputFile(inputFileName);
    string line;
    int lineNumber = 1;
    bool insideCommentBlock = false;

    while (getline(inputFile, line)) {
        string token;
        int i = 0;

        while (i < line.length()) {
            if (line[i] == ' ' || line[i] == '\t') {
                i++;  // Skip whitespace
                continue;
            }

            if (!insideCommentBlock && line[i] == '{') {
                // Start of comment block
                insideCommentBlock = true;
                tokens.push_back({Token(LEFT_BRACE, "{"), lineNumber});
                i++;
                continue;
            }

            if (insideCommentBlock && line[i] == '}') {
                // End of comment block
                insideCommentBlock = false;
                tokens.push_back({Token(RIGHT_BRACE, "}"), lineNumber});
                i++;
                continue;
            }

            if (insideCommentBlock) {
                // Ignore characters inside comment block
                i++;
                continue;
            }

            if (IsLetter(line[i])) {
                token = line[i++];
                while (i < line.length() && IsLetterOrUnderscore(line[i])) {
                    token += line[i++];
                }
                if (isReservedWord(token)) {
                    TokenType type;
                    for(const Token&word:reserved_words){
                        if(word.str==token)
                            type=word.type;
                    }
                    tokens.push_back({Token(type,token),lineNumber});
                } else {
                    tokens.push_back({Token(ID,token),lineNumber});
                }
            } else if (IsDigit(line[i])) {
                token = line[i++];
                while (i < line.length() && IsDigit(line[i])) {
                    token += line[i++];
                }
                tokens.push_back({Token(NUM,token),lineNumber});
            } else {
                token = line[i];
                if (isSymbolicToken(token)) {
                    string potentialTwoCharToken = token;
                    if (i < line.length()) {
                        potentialTwoCharToken += line[i + 1];
                    }
                    if (isSymbolicToken(potentialTwoCharToken)) {
                        i += 2;  // Advance by 2 to skip both characters
                        tokens.push_back({Token(ASSIGN, potentialTwoCharToken), lineNumber});
                    } else {
                        TokenType type;
                        for (const Token& symbol : symbolic_tokens) {
                            if (symbol.str == token)
                                type = symbol.type;
                        }
                        tokens.push_back({Token(type, token), lineNumber});
                        i++;
                    }
                }
                else {
                    tokens.push_back({Token(ERROR, token), lineNumber});
                    i++;
                }
            }
        }
        lineNumber++;
    }
    tokens.push_back({Token(ENDFILE,""),lineNumber});
}



void writeTokensToFile(const string& outputFileName) {
    ofstream outputFile(outputFileName);
    int tokenCount = 0;

    for (const auto& token : tokens) {
        outputFile << "[" << token.second << "] " << token.first.str << " (" << mp[token.first.type] << ")\n";
        tokenCount++;
    }

    cout << "Tokenization complete. Total tokens found: " << tokenCount << endl;
}

int main(){
    string inputFileName = "input.txt";
    string outputFileName = "output.txt";

    tokenize(inputFileName);
    writeTokensToFile(outputFileName);

    return 0;
}
