#include "Scanner.h"
#include "SymTable.h"

// based on LITTLE PASCAL BNF GRAMMAR VERSION 1.5

class Parser
{
private:
	Scanner  m_scanner;
	SymTable m_symTable;
	int		 m_curTokenNum;
	Token    m_curToken;
	bool     m_error;

public:
	Parser(){ m_curTokenNum = -1; m_error = false; }
	~Parser(){}

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
			_match(IDTOK);
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

private:
	void _insertSymTable(const string& name)
	{
		SymData* data = m_symTable.findInLocalScope(name);
		if (!data){
			m_symTable.insert(name);
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

	void constdecl(int tokenNum){
		if (IDTOK == tokenNum){
			printf("constdecl\n");
			Token tmp;
			tmp.lexeme = m_curToken.lexeme;
			_match(IDTOK);
			_match(EQUALTOK);
			_match(LITTOK);
			_match(SMCLNTOK);
			_insertSymTable(tmp.lexeme);
		}else{
			_error("constdecl");
		}
	}

	void moreconsdecls(int tokenNum){
		if (IDTOK == tokenNum){
			printf("moreconsdecls\n");
			constdecl(m_curTokenNum);
			moreconsdecls(m_curTokenNum);
		}
	}

	void CONSTPART(int tokenNum){
		if (CONSTTOK == tokenNum){
			printf("CONSTPART\n");
			_match(CONSTTOK);
			constdecl(m_curTokenNum);
			moreconsdecls(m_curTokenNum);
		}else{
			_error("CONSTPART");
		}
	}

	void vardecl(int tokenNum){
		if (IDTOK == tokenNum){
			printf("vardecl\n");
			Token tmp;
			tmp.lexeme = m_curToken.lexeme;
			_match(IDTOK);
			_match(COLONTOK);
			_match(BASTYPETOK);
			_match(SMCLNTOK);
			_insertSymTable(tmp.lexeme);
		}else{
			_error("vardecl");
		}
	}

	void morevardecls(int tokenNum){
		if (IDTOK == tokenNum){
			printf("morevardecls\n");
			vardecl(m_curTokenNum);
			morevardecls(m_curTokenNum);
		}
	}

	void VARPART(int tokenNum){
		if (VARTOK == tokenNum){
			printf("VARPART\n");
			_match(VARTOK);
			vardecl(m_curTokenNum);
			morevardecls(m_curTokenNum);
		}
	}

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

	void factorprime(int tokenNum){
		switch (tokenNum)
		{
		case RELOPTOK:
			printf("factorprime\n");
			_match(RELOPTOK);
			factor(m_curTokenNum);
			break;
		default:
			break;
		}
	}

	void relfactor(int tokenNum){
		switch (tokenNum)
		{
		case NOTTOK:
		case IDTOK:
		case LITTOK:
		case LPTOK:
			printf("relfactor\n");
			factor(m_curTokenNum);
			factorprime(m_curTokenNum);
			break;
		default:
			_error("relfactor");
			break;
		}
	}

	void termprime(int tokenNum){
		if (MULOPTOK == tokenNum){
			printf("termprime\n");
			_match(MULOPTOK);
			relfactor(m_curTokenNum);
			termprime(m_curTokenNum);
		}
	}

	void term(int tokenNum){
		switch (tokenNum)
		{
		case NOTTOK:
		case IDTOK:
		case LITTOK:
		case LPTOK:
			printf("term\n");
			relfactor(m_curTokenNum);
			termprime(m_curTokenNum);
			break;
		default:
			_error("term");
			break;
		}
	}

	void express(int tokenNum){
		switch (tokenNum)
		{
		case NOTTOK:
		case IDTOK:
		case LITTOK:
		case LPTOK:
			printf("express\n");
			term(m_curTokenNum);
			expprime(m_curTokenNum);
			break;
		default:
			_error("express");
			break;
		}
	}

	void factor(int tokenNum){
		switch (tokenNum)
		{
		case NOTTOK:
			printf("factor_not_factor\n");
			_match(NOTTOK);
			factor(m_curTokenNum);
			break;
		case IDTOK:
			printf("factor_idnonterm\n");
			idnonterm(m_curTokenNum);
			break;
		case LITTOK:
			printf("factor_LITTOK\n");
			_match(LITTOK);
			break;
		case LPTOK:
			printf("factor_express\n");
			_match(LPTOK);
			express(m_curTokenNum);
			_match(RPTOK);
			break;
		default:
			_error("factor");
			break;
		}
	}

	void expprime(int tokenNum){
		if (ADDOPTOK == tokenNum){
			printf("expprime\n");
			_match(ADDOPTOK);
			term(m_curTokenNum);
			expprime(m_curTokenNum);
		}
	}

	void assignstat(int tokenNum){
		if (IDTOK == tokenNum){
			printf("assignstat\n");
			idnonterm(m_curTokenNum);
			_match(ASTOK);
			express(m_curTokenNum);
		}
		else{
			_error("assignstat");
		}
	}

	void ifstat(int tokenNum){
		switch (tokenNum)
		{
		case IFTOK:
			printf("ifstat\n");
			_match(IFTOK);
			express(m_curTokenNum);
			_match(THENTOK);
			statmt(m_curTokenNum);
			break;
		default:
			_error("ifstat");
			break;
		}
	}

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

	void writeexp(int tokenNum){
		switch (tokenNum)
		{
		case STRLITTOK:
			printf("writeexp_string\n");
			_match(STRLITTOK);
			break;
		case IDTOK:
			printf("writeexp_express\n");
			express(m_curTokenNum);
			break;
		}
	}

	void writestat(int tokenNum){
		switch (tokenNum)
		{
		case WRITETOK:
			printf("writestat\n");
			_match(WRITETOK);
			_match(LPTOK);
			writeexp(m_curTokenNum);
			_match(RPTOK);
			break;
		default:
			_error("writestat");
			break;
		}
	}

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

	void whilest(int tokenNum){
		if (WHILETOK == tokenNum){
			printf("whilest\n");
			_match(WHILETOK);
			express(m_curTokenNum);
			_match(DOTOK);
			statmt(m_curTokenNum);
		}
		else{
			_error("whilest");
		}
	}

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
			writestat(m_curTokenNum);
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

	void morestats(int tokenNum){
		if (SMCLNTOK == tokenNum){
			printf("morestats\n");
			_match(SMCLNTOK);
			statmt(m_curTokenNum);
			morestats(m_curTokenNum);
		}
	}
};

