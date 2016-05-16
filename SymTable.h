#include <unordered_map>
#include <vector>
#include <stack>
#include <string>

using namespace std;

/* My Symbol Table Structure:
   hash_table< symbol_name, array<symbol_data> >
   example helping explain this:
    key("ac") -- array {{scope:0}}
    key("bb") -- array {{scope:0},{scope:2}}
    key("qw") -- array {{scope:1}}
    ......
*/

struct SymData
{
    string    name;
    int       type;
    int       offset;
    int       scopeIdx;
    int       scopeDepth;
};

class SymTable
{
public:
	SymTable() : m_scopeCount(0){}
	~SymTable(){}

	void open()
	{
        m_scopeStack.push(m_scopeCount++);
		m_scopeDepth[m_scopeCount] = m_scopeStack.size();
	}

    void close()
    {
		m_scopeStack.pop();
    }

    void insert(const string& name, int type = 0)
    {
    	int curScope = m_scopeStack.top();
        auto& stArr = m_symbolTable[name];
        auto& ssArr = m_scopeSymols[curScope];
		SymData sym;
        sym.name = name;
		sym.type = type;
        sym.offset = -4 * (int)ssArr.size();
		sym.scopeIdx = curScope;
		sym.scopeDepth = m_scopeStack.size();
		stArr.push_back(sym);
        ssArr.push_back(&stArr.back());
	}

    SymData* findInLocalScope(const string& name)
    {
    	int curScope = m_scopeStack.top();
    	auto it = m_symbolTable.find(name);
    	if (m_symbolTable.end() != it){
            auto& arr = it->second;
			for (SymData& sym : arr){
				if (sym.scopeIdx == curScope){
					return &sym;
				}
			}
    	}
    	return nullptr;
    }

    SymData* findInAllScopes(const string& name)
    {
    	int curScope = m_scopeStack.top();
		int curDepth = m_scopeDepth[curScope];
		auto it = m_symbolTable.find(name);
    	if (m_symbolTable.end() != it){
            auto& arr = it->second;
			for (SymData& sym : arr){
				if (sym.scopeDepth <= curDepth){
					return &sym;
				}
			}
    	}
		return nullptr;
    }

    void printSymbolTable()
    {
        printf("SymbolTable Dump\n");
    	for (auto it(m_scopeSymols.begin()),itEnd(m_scopeSymols.end());
    		it != itEnd; ++it)
    	{
            auto& arr = it->second;
			for (SymData* sym : arr){
				printf("SCOPE:%d SYMBOL:%s OFFSET:%d\n", sym->scopeIdx, sym->name.c_str(), sym->offset);
			}
    	}
    }

private:
	int         m_scopeCount; // store the total scope number

	stack<int>  m_scopeStack; // using for handling current and upper scopes

	unordered_map<int, int>   // hash table using for storing scope depth
				m_scopeDepth;

	unordered_map<string, vector<SymData> >
				m_symbolTable; // hash table using for storing all symbol datas

    unordered_map<int, vector<SymData*> >
                m_scopeSymols; // hash table using for storing all symbol data poiters of one scope
};
