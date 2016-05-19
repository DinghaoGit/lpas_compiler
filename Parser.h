#include "Scanner.h"
#include "SymTable.h"
#include <sstream>

// based on LITTLE PASCAL BNF GRAMMAR VERSION 1.5

#define TYPESIZE 4

struct ExpRec
{
	char typ;
	int loc;
};

class Parser
{
private:
	Scanner  m_scanner;
	SymTable m_symTable;
	int		 m_curTokenNum;
	Token    m_curToken;
	bool     m_error;
	int      m_curOff;
	int      m_curStrLabel;

	stringstream m_codeGen;
	stringstream m_codeGenData;

public:
	Parser()
	{
		m_curTokenNum = -1;
		m_error = false;
		m_curOff = 0;
		m_curStrLabel = 1;
	}

	~Parser(){}

	// PROGTOK IDTOK '(' ')' ';' CONSTPART VARPART BEGTOK stat morestats  ENDTOK '.'
	void start()
	{
	    m_scanner.scan("input.txt");
	    m_scanner.save("output.txt");

		_findToken();

	    if (PROGTOK == m_curTokenNum)
	    {
			printf("start\n");
			_match(PROGTOK);
			_match(IDTOK);
			_match(LPTOK);
			_match(RPTOK);
			_match(SMCLNTOK);
			CONSTPART(m_curTokenNum);
			VARPART(m_curTokenNum);
			_match(BEGTOK);
			statmt(m_curTokenNum);
			morestats(m_curTokenNum);
			_match(ENDTOK);
			_match(DOTTOK);
	    }
	    else{
	    	_error("start");
	    }

	    if (m_error)
            printf("FAILED !!!\n");
	    else
            printf("SUCCESS !!!\n");
	}

    void printSymbolTable() { m_symTable.printSymbolTable(); }

	void generateMIPSCode()
	{
        ofstream fout("MIPS.txt");

		fout << "#Prolog:" << endl;
		fout << ".text" << endl;
		fout << ".globl  main" << endl;
		fout << "main:" << endl;
		fout << "move  $fp  $sp    #frame pointer will be start of active stack" << endl;
		fout << "la  $a0  ProgStart" << endl;
		fout << "li  $v0  4" << endl;
		fout << "syscall" << endl;
		fout << "#End of Prolog" << endl << endl;

		fout << m_codeGen.str() << endl;

		fout << "#Postlog:" << endl;
		fout << "la  $a0  ProgEnd" << endl;
		fout << "li  $v0  4" << endl;
		fout << "syscall" << endl;
		fout << "li  $v0  10" << endl;
		fout << "syscall" << endl;
		fout << ".data" << endl;
		fout << "ProgStart:  .asciiz  \"Program Start\\n\"" << endl;
		fout << "ProgEnd:  .asciiz  \"Program  End\\n\"" << endl;
		fout << "CR:  .asciiz  \"\\n\"" << endl;

		fout << m_codeGenData.str() << endl;

        fout.clear();
        fout.close();
	}

private:
	char _getType(string s)
	{ // look at a literal token and decide its type
	    char ch;
		if ('t' == s[0] || 'f' == s[0]) ch = 'b';
		else if (string::npos != s.find('.')) ch ='f';
		else ch = 'i';
		return ch;
	}

	string _genStringLabel()
	{
		stringstream ss;
		ss << "strLabel" << m_curStrLabel << ":  .asciiz  ";
		++m_curStrLabel;
		return ss.str();
	}

	string _getStringLabelByIdx(int idx)
	{
		stringstream ss;
		ss << "strLabel" << idx;
		return ss.str();
	}

	void _insertSymTable(const string& name, char type, bool isVar)
	{
		SymData* data = m_symTable.findInLocalScope(name);
		if (!data){
			m_symTable.insert(name, type, isVar);
			m_curOff -= TYPESIZE;
		}
		else{
			printf("**** ERROR **** %s has already existed in this scope!\n", name.c_str());
		}
	}

	void _findToken()
	{
		bool got = false;
		const Token& tk = m_scanner.findToken(got);
		if (got){
			m_curTokenNum = tk.num;
			m_curToken = tk;
		}
	}

	void _match(int tokenNum){
		if (m_curTokenNum == tokenNum){
			if (BEGTOK == tokenNum || PROGTOK == tokenNum){
				m_symTable.open();
			}
			else if(ENDTOK == tokenNum){
				m_symTable.close();
			}
			_findToken();
		}
		else{
			printf("**** ERROR **** _match Cur token : %d %s\n",
                    m_curToken.num, m_curToken.lexeme.c_str());
            m_error = true;
		}
	}

	void _error(const char* msg){
		printf("**** ERROR **** %s Cur token : %d %s\n",
        		msg, m_curToken.num, m_curToken.lexeme.c_str());
        m_error = true;
	}

	// IDTOK '=' LITTOK ';'
	void constdecl(int tokenNum){
		if (IDTOK == tokenNum){
			printf("constdecl\n");
			Token tmp;
			tmp.lexeme = m_curToken.lexeme;
			_match(IDTOK);
			_match(EQUALTOK);
			_match(LITTOK);
			_match(SMCLNTOK);
			_insertSymTable(tmp.lexeme, _getType(tmp.lexeme), false);
		}else{
			_error("constdecl");
		}
	}

	// constdecl moreconsdecls  |  <empty>
	void moreconsdecls(int tokenNum){
		if (IDTOK == tokenNum){
			printf("moreconsdecls\n");
			constdecl(m_curTokenNum);
			moreconsdecls(m_curTokenNum);
		}
	}

	// CONSTTOK constdecl moreconsdecls | <empty>
	void CONSTPART(int tokenNum){
		if (CONSTTOK == tokenNum){
			printf("CONSTPART\n");
			_match(CONSTTOK);
			constdecl(m_curTokenNum);
			moreconsdecls(m_curTokenNum);
		}
	}

	// IDTOK ':' BASTYPETOK  ';'
	void vardecl(int tokenNum){
		if (IDTOK == tokenNum){
			printf("vardecl\n");
			Token tmp;
			tmp.lexeme = m_curToken.lexeme;
			_match(IDTOK);
			_match(COLONTOK);
			_match(BASTYPETOK);
			_match(SMCLNTOK);
			_insertSymTable(tmp.lexeme, _getType(tmp.lexeme), true);
		}else{
			_error("vardecl");
		}
	}

	// vardecl morevardecls | <empty>
	void morevardecls(int tokenNum){
		if (IDTOK == tokenNum){
			printf("morevardecls\n");
			vardecl(m_curTokenNum);
			morevardecls(m_curTokenNum);
		}
	}

	// VARTOK vardecl morevardecls  |  <empty>
	void VARPART(int tokenNum){
		if (VARTOK == tokenNum){
			printf("VARPART\n");
			_match(VARTOK);
			vardecl(m_curTokenNum);
			morevardecls(m_curTokenNum);
		}
	}

	// IDTOK
	void idnonterm(int tokenNum){
		if (IDTOK == tokenNum){
			printf("idnonterm\n");
            SymData* data = m_symTable.findInLocalScope(m_curToken.lexeme);
            if (data){
                printf("%s found in local scope\n", m_curToken.lexeme.c_str());
            }
            else{
                data = m_symTable.findInAllScopes(m_curToken.lexeme);
            }
            if (data){
                printf("%s found in scope %d\n", m_curToken.lexeme.c_str(), data->scopeIdx);
            }
            else{
                printf("**** ERROR **** No %s found in any scope!\n", m_curToken.lexeme.c_str());
            }
			_match(IDTOK);
		}
		else{
			_error("idnonterm");
		}
	}

	// RELOPTOK  factor  |  <empty>
	void factorprime(int tokenNum, ExpRec& expRec){
		switch (tokenNum)
		{
		case RELOPTOK:
			printf("factorprime\n");
			_match(RELOPTOK);
			factor(m_curTokenNum, expRec);
			break;
		default:
			break;
		}
	}

	// factor factorprime
	void relfactor(int tokenNum, ExpRec& expRec){
		switch (tokenNum)
		{
		case NOTTOK:
		case IDTOK:
		case LITTOK:
		case LPTOK:
			printf("relfactor\n");
			factor(m_curTokenNum, expRec);
			factorprime(m_curTokenNum, expRec);
			break;
		default:
			_error("relfactor");
			break;
		}
	}

	// MULOPTOK  relfactor termprime  |  <empty> 
	void termprime(int tokenNum, ExpRec& expRec){
		if (MULOPTOK == tokenNum){
			printf("termprime\n");
			_match(MULOPTOK);
			relfactor(m_curTokenNum, expRec);
			termprime(m_curTokenNum, expRec);
		}
	}

	// relfactor termprime
	void term(int tokenNum, ExpRec& expRec){
		switch (tokenNum)
		{
		case NOTTOK:
		case IDTOK:
		case LITTOK:
		case LPTOK:
			printf("term\n");
			relfactor(m_curTokenNum, expRec);
			termprime(m_curTokenNum, expRec);
			break;
		default:
			_error("term");
			break;
		}
	}

	// term expprime
	void express(int tokenNum, ExpRec& expRec){
		switch (tokenNum)
		{
		case NOTTOK:
		case IDTOK:
		case LITTOK:
		case LPTOK:
			printf("express\n");
			term(m_curTokenNum, expRec);
			expprime(m_curTokenNum, expRec);
			break;
		default:
			_error("express");
			break;
		}
	}

	// NOTTOK  factor  |  idnonterm  |  LITTOK  |  '('  express  ')' 
	void factor(int tokenNum, ExpRec& expRec){
		switch (tokenNum)
		{
		case NOTTOK:
			printf("factor_not_factor\n");
			_match(NOTTOK);
			factor(m_curTokenNum, expRec);
			break;
		case IDTOK:
			{
				printf("factor_idnonterm\n");
				SymData* data = m_symTable.findInAllScopes(m_curToken.lexeme);
				idnonterm(m_curTokenNum);
				if (data){
					expRec.typ = data->type;
					expRec.loc = data->offset;
				}
			}
			break;
		case LITTOK:
			printf("factor_LITTOK\n");
			{
				expRec.loc = m_curOff;
				expRec.typ = _getType(m_curToken.lexeme);
				m_codeGen << "li  $t0  " << m_curToken.lexeme << endl;
				m_codeGen << "sw  $t0  " << expRec.loc << "($fp)" << endl;
				m_curOff -= TYPESIZE;
			}
			_match(LITTOK);
			break;
		case LPTOK:
			printf("factor_express\n");
			_match(LPTOK);
			express(m_curTokenNum, expRec);
			_match(RPTOK);
			break;
		default:
			_error("factor");
			break;
		}
	}

	// ADDOPTOK  term expprime  |  <empty>  
	void expprime(int tokenNum, ExpRec& expRec){
		if (ADDOPTOK == tokenNum){
			{
				ExpRec lop = expRec; // copy the incoming parameter into a local var
         		char ch = m_curToken.lexeme[0]; // remember the addop
         		string st; // which operation will we generate?
         		switch(ch)
         		{
				case '+': st = "add"; break;
				case '-': st = "mul"; break;
				case 'o': st = "bor"; break;
				}

				printf("expprime\n");
				_match(ADDOPTOK);
         		term(m_curTokenNum, expRec); // get the right operand

				m_codeGen << "lw  $t0  " << lop.loc << "($fp)" << endl;
				m_codeGen << "lw  $t1  " << expRec.loc << "($fp)" << endl;
				m_codeGen << st	<< "  $t2  $t0  $t1" << endl;
				m_codeGen << "sw  $t2  " << m_curOff << "($fp)" << endl;
				m_curOff -= TYPESIZE;
				expRec.loc = m_curOff;
			}
			expprime(m_curTokenNum, expRec);
		}
	}

	// idnonterm  ASTOK express
	void assignstat(int tokenNum){
		if (IDTOK == tokenNum){
			printf("assignstat\n");
			SymData* data = m_symTable.findInAllScopes(m_curToken.lexeme);
			if (!data){
				printf("ERROR %s not found in all scopes\n", m_curToken.lexeme.c_str());
				return;
			}
			idnonterm(m_curTokenNum);
			_match(ASTOK);
			ExpRec expRec;
			express(m_curTokenNum, expRec);
			if (data->type != expRec.typ){
				printf("WARNING types not match\n");
			}
			m_codeGen << "lw  $t0  " << expRec.loc << "($fp)" << endl;
			m_codeGen << "sw  $t0  " << data->offset << "($fp)" << endl;
		}
		else{
			_error("assignstat");
		}
	}

	// IFTOK express THENTOK  stat 
	void ifstat(int tokenNum){
		switch (tokenNum)
		{
		case IFTOK:
			printf("ifstat\n");
			_match(IFTOK);
			ExpRec expRec;
			express(m_curTokenNum, expRec);
			_match(THENTOK);
			statmt(m_curTokenNum);
			break;
		default:
			_error("ifstat");
			break;
		}
	}

	// READTOK '(' idnonterm ')'
	void readstat(int tokenNum){
		switch (tokenNum)
		{
		case READTOK:
			printf("readstat\n");
			_match(READTOK);
			_match(LPTOK);
			idnonterm(m_curTokenNum);
			_match(RPTOK);
			break;
		default:
			_error("readstat");
			break;
		}
	}

	// STRLITTOK  |  express
	void writeexp(int tokenNum, ExpRec& expRec){
		switch (tokenNum)
		{
		case STRLITTOK:
			printf("writeexp_string\n");
			expRec.typ = 's';
			expRec.loc = m_curStrLabel;
			m_codeGenData << _genStringLabel() << m_curToken.lexeme << endl;
			_match(STRLITTOK);
			break;
		case IDTOK:
			printf("writeexp_express\n");
			express(m_curTokenNum, expRec);
			break;
		}
	}

	// WRITETOK '('  writeexp ')' 
	void writestat(int tokenNum, ExpRec& expRec){
		if (WRITETOK == tokenNum)
		{
			printf("writestat\n");
			bool isWriteln = (m_curToken.lexeme == "writeln");
			_match(WRITETOK);
			_match(LPTOK);
			writeexp(m_curTokenNum, expRec);
			if ('s' == expRec.typ)
			{
				m_codeGen << "la  $t0  " << _getStringLabelByIdx(expRec.loc) << endl;
				m_codeGen << "li  $v0  4" << endl;
				m_codeGen << "syscall" << endl;
			}
			else if ('i' == expRec.typ)
            {
				m_codeGen << "la  $t0  " << expRec.loc << "($fp)" << endl;
				m_codeGen << "li  $v0  1" << endl;
				m_codeGen << "syscall" << endl;
            }
			else if ('f' == expRec.typ)
            {
				m_codeGen << "la  $t0  " << expRec.loc << "($fp)" << endl;
				m_codeGen << "li  $v0  2" << endl;
				m_codeGen << "syscall" << endl;
            }
            if (isWriteln)
            {
                m_codeGen << "la  $t0  CR" << endl;
                m_codeGen << "li  $v0  4" << endl;
                m_codeGen << "syscall" << endl;
            }
			_match(RPTOK);
		}
		else
		{
			_error("writestat");
		}
	}

	// BEGINTOK stats ENDTOK 
	void blockst(int tokenNum){
		switch (tokenNum)
		{
		case VARTOK:
		case BEGTOK:
			printf("blockst\n");
			VARPART(m_curTokenNum);
			_match(BEGTOK);
			statmt(m_curTokenNum);
			morestats(m_curTokenNum);
			_match(ENDTOK);
			break;
		default:
			_error("blockst");
			break;
		}
	}

	// WHILETOK express DOTOK stat 
	void whilest(int tokenNum){
		if (WHILETOK == tokenNum){
			printf("whilest\n");
			_match(WHILETOK);
			ExpRec expRec;
			express(m_curTokenNum, expRec);
			_match(DOTOK);
			statmt(m_curTokenNum);
		}
		else{
			_error("whilest");
		}
	}

	// assignstat  |  ifstat  |  readstat  |  writestat |  blockst  | whilest  
	void statmt(int tokenNum){
		switch (tokenNum)
		{
		case IDTOK:
			printf("statmt_assignstat\n");
			assignstat(m_curTokenNum);
			break;
		case IFTOK:
			printf("statmt_ifstat\n");
			ifstat(m_curTokenNum);
			break;
		case READTOK:
			printf("statmt_readstat\n");
			readstat(m_curTokenNum);
			break;
		case WRITETOK:
			printf("statmt_writestat\n");
            ExpRec expRec;
			writestat(m_curTokenNum, expRec);
			break;
		case VARTOK:
		case BEGTOK:
			printf("statmt_blockst\n");
			blockst(m_curTokenNum);
			break;
		case WHILETOK:
			printf("statmt_whilest\n");
			whilest(m_curTokenNum);
			break;
		default:
			_error("statmt");
			break;
		}
	}

	// ';' statmt  morestats  |  <empty>
	void morestats(int tokenNum){
		if (SMCLNTOK == tokenNum){
			printf("morestats\n");
			_match(SMCLNTOK);
			statmt(m_curTokenNum);
			morestats(m_curTokenNum);
		}
	}
};

