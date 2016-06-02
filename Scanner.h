#include <vector>
#include <map>
#include <string>
#include <fstream>

using namespace std;

#define PROGTOK    13  // program
#define IDTOK      1   // Identifier
#define LPTOK      5   // (
#define RPTOK      6   // )
#define SMCLNTOK   8   // ;
#define BEGTOK     14  // begin
#define ENDTOK     15  // end
#define DOTTOK     9   // .
#define CONSTTOK   16  // const
#define EQUALTOK   12  // =
#define LITTOK     2   // Number
#define COLONTOK   7   // :
#define BASTYPETOK 22  // integer real boolean
#define VARTOK     17  // var
#define ASTOK      4   // :=
#define WHILETOK   18  // while
#define DOTOK      19  // do
#define IFTOK      20  // if
#define THENTOK    21  // then
#define READTOK    25  // read readln
#define WRITETOK   24  // write writeln
#define STRLITTOK  3   // String
#define NOTTOK     23  // not
#define RELOPTOK   12  // < <= >= >
#define ADDOPTOK   10  // + - or
#define MULOPTOK   11  // * / div and mod

struct Token{
	int    num;
	string lexeme;
	int    line;
};

class Scanner
{
public:
	Scanner()
	{
		m_curLine = 0;
		m_findTokenIdx = 0;
		_initTokenNums();
	}

	~Scanner(){}

	void scan(const char* fileName)
	{
		ifstream fin(fileName, ios::in);
		while(fin.getline(m_lineBuffer, sizeof(m_lineBuffer)))
		{
			_analyzeCurLine();
		}
		fin.clear();
		fin.close();
	}

    void save(const char* fileName)
	{
	    ofstream fout(fileName);
	    for (const Token& tk : m_tokens)
	    {
	    	fout << tk.num << "\t" << tk.lexeme << endl;
	    }
		fout.clear();
		fout.close();
	}

	const Token& findToken(bool& got)
	{
		if (m_findTokenIdx < (int)m_tokens.size()){
			got = true;
			return m_tokens[m_findTokenIdx++];
		}
		got = false;
		return m_tmpToken;
	}

private:
	void _nextChar()
	{
		m_ch = m_lineBuffer[m_curBufIndex++];
	}

	void _initTokenNums()
	{
		m_keywordTokenNums["program"] = PROGTOK;
		m_keywordTokenNums["begin"] = BEGTOK;
		m_keywordTokenNums["end"] = ENDTOK;
		m_keywordTokenNums["const"] = CONSTTOK;
		m_keywordTokenNums["var"] = VARTOK;
		m_keywordTokenNums["while"] = WHILETOK;
		m_keywordTokenNums["do"] = DOTOK;
		m_keywordTokenNums["if"] = IFTOK;
		m_keywordTokenNums["then"] = THENTOK;
		m_keywordTokenNums["integer"] = BASTYPETOK;
		m_keywordTokenNums["real"] = BASTYPETOK;
		m_keywordTokenNums["boolean"] = BASTYPETOK;
		m_keywordTokenNums["mod"] = MULOPTOK;
		m_keywordTokenNums["div"] = MULOPTOK;
		m_keywordTokenNums["and"] = MULOPTOK;
		m_keywordTokenNums["or"] = ADDOPTOK;
		m_keywordTokenNums["not"] = NOTTOK;
		m_keywordTokenNums["write"] = WRITETOK;
		m_keywordTokenNums["writeln"] = WRITETOK;
		m_keywordTokenNums["read"] = READTOK;
		m_keywordTokenNums["readln"] = READTOK;
		m_keywordTokenNums["true"] = LITTOK;
		m_keywordTokenNums["false"] = LITTOK;

		m_stateTokenNums[S2] = IDTOK;
		m_stateTokenNums[S3] = LITTOK;
		m_stateTokenNums[S4] = LITTOK;
		m_stateTokenNums[S5] = LITTOK;
		m_stateTokenNums[S25] = STRLITTOK;
		m_stateTokenNums[S6] = LPTOK;
		m_stateTokenNums[S7] = RPTOK;
		m_stateTokenNums[S8] = COLONTOK;
		m_stateTokenNums[S9] = ASTOK;
		m_stateTokenNums[S10] = SMCLNTOK;
		m_stateTokenNums[S11] = 35; // ,
		m_stateTokenNums[S12] = ADDOPTOK; // +
		m_stateTokenNums[S13] = ADDOPTOK; // -
		m_stateTokenNums[S14] = MULOPTOK; // *
		m_stateTokenNums[S15] = MULOPTOK; // /
		m_stateTokenNums[S16] = RELOPTOK;
		m_stateTokenNums[S17] = RELOPTOK;
		m_stateTokenNums[S18] = RELOPTOK;
		m_stateTokenNums[S19] = RELOPTOK;
		m_stateTokenNums[S26] = EQUALTOK;
		m_stateTokenNums[S23] = DOTTOK;
	}

    void _pushToken()
    {
        if (S1 != m_curState &&
        	S20 != m_curState &&
        	S21 != m_curState &&
        	S24 != m_curState &&
        	!m_tmpToken.lexeme.empty())
        {
        	if (S2 == m_curState){
        		auto iter = m_keywordTokenNums.find(m_tmpToken.lexeme);
        		if (m_keywordTokenNums.end() != iter){
        			m_tmpToken.num = iter->second;
        		}
        		else{
        			m_tmpToken.num = m_stateTokenNums[S2];
        		}
        		m_tmpToken.line = m_curLine;
				m_tokens.push_back(m_tmpToken);
        	}
        	else
        	{
        		auto iter = m_stateTokenNums.find(m_curState);
				if (m_stateTokenNums.end() != iter){
        			m_tmpToken.num = m_stateTokenNums[m_curState];
        			m_tmpToken.line = m_curLine;
					m_tokens.push_back(m_tmpToken);
        		}
        	}
            m_tmpToken.lexeme.clear();
        }
        else
        {
        	_nextChar();
        }
        m_curState = S1;
	}

	void _analyzeCurLine()
	{
		m_curBufIndex = 0;
		m_curState = S1;
		bool stateDone = false;
		++m_curLine;

		do
		{
			if(stateDone){
				_pushToken();
				stateDone = false;
			}
			else{
				_nextChar();
			}

			// break while EOF
			if('\0' == m_ch){
                _pushToken();
                break;
            }

			// start DFA
			switch(m_curState)
			{
				case S1:{
					if(isalpha(m_ch))
						m_curState = S2;
					else if(isdigit(m_ch))
						m_curState = S3;
					else if('(' == m_ch)
						m_curState = S6;
					else if(')' == m_ch)
						m_curState = S7;
					else if(':' == m_ch)
						m_curState = S8;
					else if(';' == m_ch)
						m_curState = S10;
					else if(',' == m_ch)
						m_curState = S11;
					else if('+' == m_ch)
						m_curState = S12;
					else if('-' == m_ch)
						m_curState = S13;
					else if('*' == m_ch)
						m_curState = S14;
					else if('/' == m_ch)
						m_curState = S15;
					else if('<' == m_ch)
						m_curState = S16;
					else if('>' == m_ch)
						m_curState = S18;
					else if('.' == m_ch)
						m_curState = S23;
					else if('"' == m_ch)
						m_curState = S24;
					else if('=' == m_ch)
						m_curState = S26;
					else
						stateDone = true;
				}break;
				case S2:{
					if(isalpha(m_ch) || isdigit(m_ch))
						m_curState = S2;
					else
						stateDone = true;
				}break;
				case S3:{
					if(isdigit(m_ch))
						m_curState = S3;
					else if('.' == m_ch)
						m_curState = S4;
					else
						stateDone = true;
				}break;
				case S4:{
					if(isdigit(m_ch))
						m_curState = S5;
					else
						stateDone = true;
				}break;
				case S5:{
					if(isdigit(m_ch))
						m_curState = S5;
					else
						stateDone = true;
				}break;
				case S6:{
					if('*' == m_ch)
						m_curState = S20;
					else
						stateDone = true;
				}break;
				case S7:{
					stateDone = true;
				}break;
				case S8:{
					if('=' == m_ch)
						m_curState = S9;
					else
						stateDone = true;
				}break;
				case S9:
				case S10:
				case S11:
				case S12:
				case S13:
				case S14:
				case S15:{
					stateDone = true;
				}break;
				case S16:{
					if('=' == m_ch)
						m_curState = S17;
					else
						stateDone = true;
				}break;
				case S17:{
					stateDone = true;
				}break;
				case S18:{
					if('=' == m_ch)
						m_curState = S19;
					else
						stateDone = true;
				}break;
				case S19:{
					stateDone = true;
				}break;
				case S20:{
					if('*' != m_ch)
						m_curState = S20;
					else if('*' == m_ch)
						m_curState = S21;
					else
						stateDone = true;
				}break;
				case S21:{
					if('*' == m_ch)
						m_curState = S21;
					else if(')' == m_ch)
						m_curState = S22;
					else if('*' != m_ch)
						m_curState = S20;
					else
						stateDone = true;
				}break;
				case S22:{
					stateDone = true;
				}break;
				case S23:{
					stateDone = true;
				}break;
				case S24:{
					if('"' == m_ch)
						m_curState = S25;
					else if('"' != m_ch)
						m_curState = S24;
					else
						stateDone = true;
				}break;
				case S25:
				case S26:{
					stateDone = true;
				}break;
			}
			if(!stateDone){
				m_tmpToken.lexeme.push_back(m_ch);
			}
		} while(true);
	}

private:
	enum state{ S1,S2,S3,S4,S5,S6,S7,S8,S9,S10,
				S11,S12,S13,S14,S15,S16,S17,S18,S19,
			    S20,S21,S22,S23,S24,S25,S26 };

	state m_curState;

	int m_curBufIndex;
	char m_lineBuffer[4096];

	int m_curLine;

	map<state,int>  m_stateTokenNums;
	map<string,int> m_keywordTokenNums;

	vector<Token> m_tokens;
	Token m_tmpToken;
	int m_findTokenIdx;

	char m_ch;
};
