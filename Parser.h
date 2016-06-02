#include "Scanner.h"
#include "SymTable.h"
#include <sstream>
#include <string.h>
#include <stdarg.h>
#include <functional>

// based on LITTLE PASCAL BNF GRAMMAR

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

	// 1  program :  PROGTOK IDTOK '(' ')' ';' CONSTPART VARPART BEGTOK stat morestats ENDTOK '.'
	void start()
	{
	    m_scanner.scan("input.txt");
	    m_scanner.save("output.txt");

		_findToken();

	    if (PROGTOK == m_curTokenNum)
	    {
			_info("1 program \n");
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
	    	_error("1 program ");
	    }

	    if (m_error)
            _info("Compiling FAILED !!!\n");
	    else
            _info("Compiling SUCCESS !!!\n");
	}

    void printSymbolTable() { m_symTable.printSymbolTable(); }

	void generateMIPSCode()
	{
        ofstream fout("MIPS.asm");

		fout << "#Prolog:" << endl;
		fout << ".text" << endl;
		fout << ".globl  main" << endl;
		fout << "main:" << endl;
		fout << "move  $fp  $sp  #frame pointer will be start of active stack" << endl;
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

	string _genJumpLabel()
	{
		static int curIdx = 0;
		stringstream ss;
		ss << "jumpLabel" << curIdx;
		++curIdx;
		return ss.str();
	}

	string _getStringLabelByIdx(int idx)
	{
		stringstream ss;
		ss << "strLabel" << idx;
		return ss.str();
	}

	// used for generating MIPS code
	void _genCode( ExpRec* expRec,
					const char* p1,
					const char* p2 = nullptr,
					const char* p3 = nullptr,
					const char* p4 = nullptr)
	{
		// Floating Point Arithmetic on MIPS
		if (expRec && 'f' == expRec->typ){
			if (0 == strcmp(p1, "lw")) p1 = "l.s";
			else if (0 == strcmp(p1, "li")) p1 = "li.s";
			else if (0 == strcmp(p1, "sw")) p1 = "s.s";
			auto mkf = [] (const char*& rp){
			    if(!rp) return;
				if (0 == strcmp(rp, "$t0")) rp = "$f0";
				else if (0 == strcmp(rp, "$t1")) rp = "$f1";
				else if (0 == strcmp(rp, "$t2")) rp = "$f2";
				else if (0 == strcmp(rp, "$t3")) rp = "$f3";
			};
			mkf(p2);
			mkf(p3);
			mkf(p4);
		}
		if (p1) m_codeGen << p1 << "  ";
		if (p2) m_codeGen << p2 << "  ";
		if (p3) m_codeGen << p3 << "  ";
		if (p4) m_codeGen << p4;
		m_codeGen << endl;
	}

	// make a offset + $fp string
	const char* _mkfp(int offset){
		static char tmp[256];
		sprintf(tmp, "%d($fp)", offset);
		return tmp;
	}

	void _insertSymTable(const string& name, char type, bool isVar)
	{
		SymData* data = m_symTable.findInLocalScope(name);
		if (!data){
			m_symTable.insert(name, type, isVar);
			m_curOff -= TYPESIZE;
		}
		else{
			_error2("%s has already existed in this scope!", name.c_str());
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
			_error("_match");
		}
	}

	void _error(const char* msg){
		printf("**** ERROR **** %s (line: %d token: %d %s)\n",
        		msg, m_curToken.line, m_curToken.num, m_curToken.lexeme.c_str());
		m_error = true;
	}

	void _error2(const char* format, ...){
		va_list argPtr;
		va_start(argPtr, format);
		char tmp[512];
		vsprintf(tmp, format, argPtr);
		va_end(argPtr);
		printf("**** ERROR **** %s (line: %d token: %d %s)\n",
        		tmp, m_curToken.line, m_curToken.num, m_curToken.lexeme.c_str());
        m_error = true;
	}

	void _warning(const char* format, ...){
		va_list argPtr;
		va_start(argPtr, format);
		char tmp[512];
		vsprintf(tmp, format, argPtr);
		va_end(argPtr);
		printf("**** Warning **** %s (line: %d token: %d %s)\n",
        		tmp, m_curToken.line, m_curToken.num, m_curToken.lexeme.c_str());
	}

	void _info(const char* format, ...){
		va_list argPtr;
		va_start(argPtr, format);
		char tmp[512];
		vsprintf(tmp, format, argPtr);
		va_end(argPtr);
		printf(tmp);
	}

private:
	// 7  constdecl : IDTOK '=' LITTOK ';'
	void constdecl(int tokenNum){
		if (IDTOK == tokenNum){
			_info("7 constdecl\n");
			Token tmp;
			tmp.lexeme = m_curToken.lexeme;
			_match(IDTOK);
			_match(EQUALTOK);
			_match(LITTOK);
			_match(SMCLNTOK);
			_insertSymTable(tmp.lexeme, _getType(tmp.lexeme), false);
		}else{
			_error("7 constdecl");
		}
	}

	// 4  moreconstdecls :  constdecl moreconsdecls | <empty>
	void moreconsdecls(int tokenNum){
		if (IDTOK == tokenNum){
			_info("4 moreconsdecls\n");
			constdecl(m_curTokenNum);
			moreconsdecls(m_curTokenNum);
		}
	}

	// 2  CONSTPART :  CONSTTOK  constdecl moreconsdecls |  <empty>
	void CONSTPART(int tokenNum){
		if (CONSTTOK == tokenNum){
			_info("2 CONSTPART\n");
			_match(CONSTTOK);
			constdecl(m_curTokenNum);
			moreconsdecls(m_curTokenNum);
		}
	}

	// 6  vardecl : IDTOK ':' BASTYPETOK  ';'
	void vardecl(int tokenNum){
		if (IDTOK == tokenNum){
			_info("6 vardecl\n");
            string symName = m_curToken.lexeme;
            char symType = 'i';
			_match(IDTOK);
			_match(COLONTOK);
            if("real" == m_curToken.lexeme) symType = 'f';
			_match(BASTYPETOK);
			_match(SMCLNTOK);
			_insertSymTable(symName, symType, true);
		}else{
			_error("6 vardecl");
		}
	}

	// 5  morevardecls	 : vardecl morevardecls  | <empty>
	void morevardecls(int tokenNum){
		if (IDTOK == tokenNum){
			_info("5 morevardecls\n");
			vardecl(m_curTokenNum);
			morevardecls(m_curTokenNum);
		}
	}

	// 3  VARPAET :  VARTOK vardecl morevardecls  |  <empty>
	void VARPART(int tokenNum){
		if (VARTOK == tokenNum){
			_info("3 VARPART\n");
			_match(VARTOK);
			vardecl(m_curTokenNum);
			morevardecls(m_curTokenNum);
		}
	}

	// 24 idnonterm :  IDTOK
	void idnonterm(int tokenNum){
		if (IDTOK == tokenNum){
			_info("24 idnonterm\n");
            SymData* data = m_symTable.findInLocalScope(m_curToken.lexeme);
            if (data){
                _info("%s found in local scope\n", m_curToken.lexeme.c_str());
            }
            else{
                data = m_symTable.findInAllScopes(m_curToken.lexeme);
            }
            if (data){
                _info("%s found in scope %d\n", m_curToken.lexeme.c_str(), data->scopeIdx);
            }
            else{
                _error2("No %s found in any scope!", m_curToken.lexeme.c_str());
            }
			_match(IDTOK);
		}
		else{
			_error("24 idnonterm");
		}
	}

	// 22 factorprime :  RELOPTOK  factor |  <empty>
	void factorprime(int tokenNum, ExpRec& expRec){
		if (RELOPTOK == tokenNum)
		{
			ExpRec lop = expRec;
			bool smallerThan = ("<" == m_curToken.lexeme ? true : false);

			_info("22 factorprime\n");
			_match(RELOPTOK);
			factor(m_curTokenNum, expRec);

			_genCode(&expRec, "lw", "$t0", _mkfp(lop.loc));
			_genCode(&expRec, "lw", "$t1", _mkfp(expRec.loc));
			if (smallerThan){
				_genCode(&expRec, "slt", "$t2", "$t0", "$t1");
			}else{
				_genCode(&expRec, "slt", "$t2", "$t1", "$t0");
			}
			_genCode(&expRec, "sw", "$t2", _mkfp(m_curOff));
			m_curOff -= TYPESIZE;
			expRec.loc = m_curOff;
			expRec.typ = 'b';
		}
	}

	// 21 relfactor :  factor factorprime
	void relfactor(int tokenNum, ExpRec& expRec){
		switch (tokenNum)
		{
		case NOTTOK:
		case IDTOK:
		case LITTOK:
		case LPTOK:
			_info("21 relfactor\n");
			factor(m_curTokenNum, expRec);
			factorprime(m_curTokenNum, expRec);
			break;
		default:
			_error("21 relfactor");
			break;
		}
	}

	// 20 termprime :  MULOPTOK  relfactor termprime  |  <empty>
	void termprime(int tokenNum, ExpRec& expRec){
		if (MULOPTOK == tokenNum){
			{
				ExpRec lop = expRec;
				char ch = m_curToken.lexeme[0];
				string st;
				switch(ch)
				{
				case 'm': st = "rem"; break; // mod
				case '/':
				case 'd': st = "div"; break;
				case 'a': st = "and"; break;
				case '*': st = "mult"; break;
				}

				_info("20 termprime\n");
				_match(MULOPTOK);
				relfactor(m_curTokenNum, expRec);

				_genCode(&expRec, "lw", "$t0", _mkfp(lop.loc));
				_genCode(&expRec, "lw", "$t1", _mkfp(expRec.loc));
				_genCode(&expRec, st.c_str(), "$t2", "$t0", "$t1");
				_genCode(&expRec, "sw", "$t2", _mkfp(m_curOff));
				m_curOff -= TYPESIZE;
				expRec.loc = m_curOff;
			}
			termprime(m_curTokenNum, expRec);
		}
	}

	// 19 term :  relfactor termprime
	void term(int tokenNum, ExpRec& expRec){
		switch (tokenNum)
		{
		case NOTTOK:
		case IDTOK:
		case LITTOK:
		case LPTOK:
			_info("19 term\n");
			relfactor(m_curTokenNum, expRec);
			termprime(m_curTokenNum, expRec);
			break;
		default:
			_error("19 term");
			break;
		}
	}

	// 17 express :  term expprime
	void express(int tokenNum, ExpRec& expRec){
		switch (tokenNum)
		{
		case NOTTOK:
		case IDTOK:
		case LITTOK:
		case LPTOK:
			_info("17 express\n");
			term(m_curTokenNum, expRec);
			expprime(m_curTokenNum, expRec);
			break;
		default:
			_error("17 express");
			break;
		}
	}

	// 23 factor :  NOTTOK factor | idnonterm |  LITTOK |  '('  express  ')'
	void factor(int tokenNum, ExpRec& expRec){
		switch (tokenNum)
		{
		case NOTTOK:
			_info("23 factor : NOTTOK factor\n");
			_match(NOTTOK);
			factor(m_curTokenNum, expRec);
			break;
		case IDTOK:
			{
				_info("23 factor : idnonterm\n");
				SymData* data = m_symTable.findInAllScopes(m_curToken.lexeme);
				idnonterm(m_curTokenNum);
				if (data){
					expRec.typ = data->type;
					expRec.loc = data->offset;
				}
			}
			break;
		case LITTOK:
			_info("23 factor : LITTOK\n");
			{
				expRec.loc = m_curOff;
				expRec.typ = _getType(m_curToken.lexeme);
				_genCode(&expRec, "li", "$t0", m_curToken.lexeme.c_str());
				_genCode(&expRec, "sw", "$t0", _mkfp(expRec.loc));
				m_curOff -= TYPESIZE;
			}
			_match(LITTOK);
			break;
		case LPTOK:
			_info("23 factor : (express)\n");
			_match(LPTOK);
			express(m_curTokenNum, expRec);
			_match(RPTOK);
			break;
		default:
			_error("23 factor");
			break;
		}
	}

	// 18 expprime :  ADDOPTOK  term expprime |  <empty>
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
				case 'o': st = "or"; break;
				}

				_info("18 expprime\n");
				_match(ADDOPTOK);
         		term(m_curTokenNum, expRec); // get the right operand

				_genCode(&expRec, "lw", "$t0", _mkfp(lop.loc));
				_genCode(&expRec, "lw", "$t1", _mkfp(expRec.loc));
				_genCode(&expRec, st.c_str(), "$t2", "$t0", "$t1");
				_genCode(&expRec, "sw", "$t2", _mkfp(m_curOff));
				m_curOff -= TYPESIZE;
				expRec.loc = m_curOff;
			}
			expprime(m_curTokenNum, expRec);
		}
	}

	// 10 assignstat :  idnonterm  ASTOK express
	void assignstat(int tokenNum){
		if (IDTOK == tokenNum){
			_info("10 assignstat\n");
			SymData* data = m_symTable.findInAllScopes(m_curToken.lexeme);
			if (!data){
				_error2("%s not found in all scopes", m_curToken.lexeme.c_str());
				return;
			}
			idnonterm(m_curTokenNum);
			_match(ASTOK);
			ExpRec expRec;
			express(m_curTokenNum, expRec);
			if (data->type != expRec.typ){
				_warning("types not match");
			}
			_genCode(&expRec, "lw", "$t0", _mkfp(expRec.loc));
			_genCode(&expRec, "sw", "$t0", _mkfp(data->offset));
		}
		else{
			_error("10 assignstat");
		}
	}

	// 11 ifstat : IFTOK express THENTOK  stat
	void ifstat(int tokenNum){
		if (IFTOK == tokenNum){
			_info("11 ifstat\n");
			_match(IFTOK);
			ExpRec expRec;
			express(m_curTokenNum, expRec);

			if ('b' != expRec.typ){
				_warning("types not match in ifstat");
			}
			_genCode(&expRec, "lw", "$t0", _mkfp(m_curOff));
			string label = _genJumpLabel();
			_genCode(&expRec, "beq", "$t0", "$0", label.c_str());

			_match(THENTOK);
			statmt(m_curTokenNum);

			label.append(":");
			_genCode(&expRec, label.c_str());
		}
		else{
			_error("11 ifstat");
		}
	}

	// 12 readstat :  READTOK '(' idnonterm ')'
	void readstat(int tokenNum){
		switch (tokenNum)
		{
		case READTOK:
			_info("12 readstat\n");
			_match(READTOK);
			_match(LPTOK);
			idnonterm(m_curTokenNum);
			_match(RPTOK);
			break;
		default:
			_error("12 readstat");
			break;
		}
	}

	// 16 writeexp :  STRLITTOK  |  express
	void writeexp(int tokenNum, ExpRec& expRec){
		switch (tokenNum)
		{
		case STRLITTOK:
			_info("16 writeexp : STRLITTOK\n");
			expRec.typ = 's';
			expRec.loc = m_curStrLabel;
			m_codeGenData << _genStringLabel() << m_curToken.lexeme << endl;
			_match(STRLITTOK);
			break;
		case IDTOK:
			_info("16 writeexp : express\n");
			express(m_curTokenNum, expRec);
			break;
		}
	}

	// 13 writestat :  WRITETOK '('  writeexp ')'
	void writestat(int tokenNum, ExpRec& expRec){
		if (WRITETOK == tokenNum)
		{
			_info("13 writestat\n");
			bool isWriteln = (m_curToken.lexeme == "writeln");
			_match(WRITETOK);
			_match(LPTOK);
			writeexp(m_curTokenNum, expRec);
			if ('s' == expRec.typ)
			{
				_genCode(nullptr, "la", "$t0", _getStringLabelByIdx(expRec.loc).c_str());
				_genCode(nullptr, "li", "$v0", "4");
				_genCode(nullptr, "syscall");
			}
			else if ('i' == expRec.typ)
            {
				_genCode(nullptr, "la", "$t0", _mkfp(expRec.loc));
				_genCode(nullptr, "li", "$v0", "1");
				_genCode(nullptr, "syscall");
            }
			else if ('f' == expRec.typ)
            {
				_genCode(nullptr, "la", "$t0", _mkfp(expRec.loc));
				_genCode(nullptr, "li", "$v0", "2");
				_genCode(nullptr, "syscall");
            }
            if (isWriteln)
            {
				_genCode(nullptr, "la", "$t0", "CR");
				_genCode(nullptr, "li", "$v0", "4");
				_genCode(nullptr, "syscall");
            }
			_match(RPTOK);
		}
		else
		{
			_error("13 writestat");
		}
	}

	// 15 blockst :  BEGINTOK   stats   ENDTOK
	void blockst(int tokenNum){
		switch (tokenNum)
		{
		case VARTOK:
		case BEGTOK:
			_info("15 blockst\n");
			VARPART(m_curTokenNum);
			_match(BEGTOK);
			statmt(m_curTokenNum);
			morestats(m_curTokenNum);
			_match(ENDTOK);
			break;
		default:
			_error("15 blockst");
			break;
		}
	}

	// 14 whilest :  WHILETOK express DOTOK stat
	void whilest(int tokenNum){
		if (WHILETOK == tokenNum){
			_info("14 whilest\n");
			_match(WHILETOK);

			// Generate a new label, remember it, AND lay it down
			string labelStart = _genJumpLabel();
			m_codeGen << labelStart << ":" << endl;

			// Evaluate the expression, leaving the value on the stack. Check Boolean.
			ExpRec expRec;
			express(m_curTokenNum, expRec);

			if ('b' != expRec.typ){
				_warning("types not match in whilest");
			}
			_genCode(&expRec, "lw", "$t0", _mkfp(m_curOff));
			string labelEnd = _genJumpLabel();
			// CodeGen a jump to this label if t0 == false.
			_genCode(&expRec, "beq", "$t0", "$0", labelEnd.c_str());

			_match(DOTOK);
			statmt(m_curTokenNum);

			_genCode(&expRec, "j", labelStart.c_str());
			labelEnd.append(":");
			_genCode(&expRec, labelEnd.c_str());
		}
		else{
			_error("14 whilest");
		}
	}

	// 9  statmt :  assignstat  |  ifstat |  readstat |  writestat |  blockst | whilest
	void statmt(int tokenNum){
		switch (tokenNum)
		{
		case IDTOK:
			_info("9 statmt : assignstat\n");
			assignstat(m_curTokenNum);
			break;
		case IFTOK:
			_info("9 statmt : ifstat\n");
			ifstat(m_curTokenNum);
			break;
		case READTOK:
			_info("9 statmt : readstat\n");
			readstat(m_curTokenNum);
			break;
		case WRITETOK:
			_info("9 statmt : writestat\n");
            ExpRec expRec;
			writestat(m_curTokenNum, expRec);
			break;
		case VARTOK:
		case BEGTOK:
			_info("9 statmt : blockst\n");
			blockst(m_curTokenNum);
			break;
		case WHILETOK:
			_info("9 statmt : whilest\n");
			whilest(m_curTokenNum);
			break;
		default:
			_error("9 statmt");
			break;
		}
	}

	// 8  morestats : ';' statmt   morestats | <empty>
	void morestats(int tokenNum){
		if (SMCLNTOK == tokenNum){
			_info("8 morestats\n");
			_match(SMCLNTOK);
			statmt(m_curTokenNum);
			morestats(m_curTokenNum);
		}
	}
};

