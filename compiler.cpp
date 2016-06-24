#include <vector>
#include <fstream>
#include <algorithm>
#include <cstdio>
#include <map>
#include <set>
#include <iostream>
#include <string>
#include <cstring>
#include <stack>
#include <iomanip>
using namespace std;
#define LEN_CHAR_ARRAY 100
#define RECUR_LEVEL 5
#define GRAMMAR_FILE "grammar.txt"
#define INPUT_FILE "main.c"
#define ASM_FILE "main.asm"
#define SYMBOL_FILE "symbol_table.txt"

struct Var {
	int scope, size, isPara;
	string symbol, type, alias;
	bool isFunc, isArray;
	Var(int sco, string sym, string typ, bool isf, bool isa, string ali, int siz, int ipa) {
		scope = sco;
		symbol = sym;
		type = typ;
		isFunc = isf;
		isArray = isa;
		alias = ali;
		size = siz;
		isPara = ipa;
	}
	bool operator<(const struct Var &t) const {
		return scope < t.scope;
	}
};

typedef struct Var Var;
typedef vector<vector<string> > Productions;
typedef vector<string> Production;
map<string, Productions> grammar;
map<string, set<string> > firstSet;
map<string, set<string> > followSet;
map<pair<string, string>, set<Production> > LLtable;
set<string> symbolSet;
vector<string> tree;
vector<Var> symbolTable;
string resultType, funcCall;
fstream asmout;
int paraSum;

void getGrammar();
bool hasEpsilonProduct(string symbol);
bool isEpsilonProduct(Production p);
bool isTerminal(string symbol);
bool canReduceToEpsilon(Production p, int level);
bool canReduceToEpsilon(string symbol, int level);
bool isKeyword(string token);
int alias2index(string);
set<string> &getFirstSet(string symbol);
set<string> &getFirstSet(Production p);
void generateFollowSet();
void generateLLtable();
void generateSet();
void generateTree();
int generateCode(int step);
void generateSymbolTable();
void printAL(string, string);

int main(int argc, char **argv) {

	getGrammar();

	generateSet();

	generateLLtable();

	generateTree();

	generateSymbolTable();

	asmout.open(ASM_FILE, fstream::out);
	generateCode(0);
	asmout.close();

	return 0;
}

int generateCode(int step) {
	static stack<string> whileStack;
	static int labelCount = 0;
	int cost = 0;
	if (tree[step].compare("S") == 0) {
		asmout << "\t.data" << endl;
		asmout << "NEWLINE:\t.asciiz\t\"\\n\"" << endl;
		asmout << "ADDR:\t.word\t0" << endl;
		for (int i = 0; i < symbolTable.size(); ++i) {
			if (symbolTable[i].scope == -1) {
				asmout << symbolTable[i].alias << ":\t";
				asmout << "." << symbolTable[i].type << "\t";
				asmout << symbolTable[i].symbol << endl;
			}
		}
		asmout << "\t.text" << endl;
		asmout << "set_true:" << endl;
		asmout << "\tli\t$v0, 1" << endl;
		asmout << "\tlw\t$t8, ADDR" << endl;
		asmout << "\taddi\t$t8, 8" << endl;
		asmout << "\tjr\t$t8" << endl;
		asmout << "get_addr:" << endl;
		asmout << "\tsw\t$ra, ADDR" << endl;
		asmout << "\tjr\t$ra" << endl;
		asmout << "\t.globl\tmain" << endl;
		return cost + generateCode(step + 1) + 1;
	} else if (tree[step].compare("Program") == 0) {
		return cost + generateCode(step + 1) + 1;
	} else if (tree[step].compare("DeclList") == 0) {
		if (tree[step + 1].compare("epsilon") == 0) {
			return 2;
		} else {
			cost += generateCode(step + 1);
			return cost + generateCode(step + cost + 1) + 1;
		}
	} else if (tree[step].compare("DeclList'") == 0) {
		if (tree[step + 6].compare("VarDecl'") == 0) {
			asmout << "\t.data" << endl;
			asmout << tree[step + 4] << ":\t.space\t";
			asmout << symbolTable[alias2index(tree[step + 4])].size << endl;
		}
		cost += generateCode(step + 1);
		cost += 2;
		return cost + generateCode(step + cost + 1) + 1;
	} else if (tree[step].compare("Decl") == 0) {
		return generateCode(step + 1) + 1;
	} else if (tree[step].compare("VarDecl") == 0) {
		cost += generateCode(step + 1);
		cost += 2;
		return cost + generateCode(step + cost + 1) + 1;
	} else if (tree[step].compare("VarDecl'") == 0) {
		if (tree[step + 1][0] == ';') {
			return 2;
		} else {
			return 6;
		}
	} else if (tree[step].compare("FunDecl") == 0) {
		asmout << "\t.text" << endl;
		asmout << symbolTable[alias2index(tree[step - 2])].symbol << ":" << endl;
		asmout << "\taddi\t$sp, $sp, ";
		asmout << (-(symbolTable[alias2index(tree[step - 2])].size + 4)) << endl;
		asmout << "\tsw\t$s0, 0($sp)" << endl;
		asmout << "\taddi\t$s0, $sp, 4" << endl;
		cost += 1;
		cost += generateCode(step + cost + 1);
		cost += 1;
		cost += generateCode(step + cost + 1);
		asmout << "\tlw\t$s0, 0($sp)" << endl;
		asmout << "\taddi\t$sp, $sp, ";
		asmout << (symbolTable[alias2index(tree[step - 2])].size + 4)<< endl;
		asmout << "\tjr\t$ra" << endl;
		return cost + 1;
	} else if (tree[step].compare("VarDeclList") == 0) {
		if (tree[step + 1].compare("epsilon") == 0) {
			return 2;
		} else {
			cost += generateCode(step + 1);
			return cost + generateCode(step + cost + 1) + 1;
		}
	} else if (tree[step].compare("ParamDeclList") == 0) {
		if (tree[step + 1].compare("epsilon") == 0) {
			return 2;
		} else {
			return cost + generateCode(step + 1) + 1;
		}
	} else if (tree[step].compare("ParamDeclListTail") == 0) {
		cost += generateCode(step + 1);
		return cost + generateCode(step + cost + 1) + 1;
	} else if (tree[step].compare("ParamDeclListTail'") == 0) {
		if (tree[step + 1][0] == ',') {
			cost += 1;
			return cost + generateCode(step + cost + 1) + 1;
		} else {
			return 2;
		}
	} else if (tree[step].compare("ParamDecl") == 0) {
		cost += 4;
		return cost + generateCode(step + cost + 1) + 1;
	} else if (tree[step].compare("ParamDecl'") == 0) {
		if (tree[step + 1][0] == '[') {
			return 3;
		} else {
			return 2;
		}
	} else if (tree[step].compare("Block") == 0) {
		cost += 1;
		cost += generateCode(step + cost + 1);
		cost += generateCode(step + cost + 1);
		cost += 1;
		return cost + 1;
	} else if (tree[step].compare("Type") == 0) {
		return 2;
	} else if (tree[step].compare("StmtList") == 0) {
		cost += generateCode(step + 1);
		return cost + generateCode(step + cost + 1) + 1;
	} else if (tree[step].compare("StmtList'") == 0) {
		if (tree[step + 1].compare("epsilon") == 0) {
			return 2;
		} else {
			return cost + generateCode(step + 1) + 1;
		}
	} else if (tree[step].compare("Stmt") == 0) {
		if (tree[step + 1].compare(";") == 0) {
			return 2;
		} else if (tree[step + 1].compare("Expr") == 0) {
			cost += generateCode(step + 1);
			cost += 1;
			return cost + 1;
		} else if (tree[step + 1].compare("return") == 0) {
			cost += 1;
			cost += generateCode(step + cost + 1);
			cost += 1;
			return cost + 1;
		} else if (tree[step + 1].compare("break") == 0) {
			asmout << "\tj\t" << whileStack.top() << endl;
		} else if (tree[step + 1].compare("if") == 0) {
			char buffer[20];
			string iflabel, elselabel, exitlabel;

			sprintf(buffer, "LAB%03x", labelCount++);
			iflabel.assign(buffer);
			sprintf(buffer, "LAB%03x", labelCount++);
			elselabel.assign(buffer);
			sprintf(buffer, "LAB%03x", labelCount++);
			exitlabel.assign(buffer);

			cost += 2;
			cost += generateCode(step + cost + 1);
			asmout << "\tbeqz\t$v0, " << elselabel << endl;
			asmout << iflabel << ":" << endl;
			cost += 1;
			cost += generateCode(step + cost + 1);
			asmout << "\tj\t" << exitlabel << endl;
			asmout << elselabel << ":" << endl;
			cost += 1;
			cost += generateCode(step + cost + 1);
			asmout << exitlabel << ":" << endl;
			return cost + 1;
		} else if (tree[step + 1].compare("while") == 0) {
			char buffer[20];
			string whilelabel, exitlabel;

			sprintf(buffer, "LAB%03x", labelCount++);
			whilelabel.assign(buffer);
			sprintf(buffer, "LAB%03x", labelCount++);
			exitlabel.assign(buffer);

			whileStack.push(exitlabel);

			asmout << whilelabel << ":" << endl;
			cost += 2;
			cost += generateCode(step + cost + 1);
			asmout << "\tbeqz\t$v0, " << exitlabel << endl;
			cost += 1;
			cost += generateCode(step + cost + 1);
			asmout << "\tj\t" << whilelabel << endl;
			asmout << exitlabel << ":" << endl;

			whileStack.pop();
			return cost + 1;
		} else if (tree[step + 1].compare("Block") == 0) {
			return generateCode(step + 1) + 1;
		} else if (tree[step + 1].compare("print") == 0) {
			int index = alias2index(tree[step + 3]);
			if (symbolTable[index].type.compare("int") == 0) {
				asmout << "\tli\t$v0, 1" << endl;
				if (symbolTable[index].scope == 0) {
					asmout << "\tlw\t$a0, " << symbolTable[index].alias << endl;
				} else if (symbolTable[index].scope > 0) {
					asmout << "\tlw\t$a0, " << symbolTable[index].size << "($s0)" << endl;
				}
			} else if (symbolTable[index].type.compare("float") == 0) {
				asmout << "\tli\t$v0, 2" << endl;
				if (symbolTable[index].scope == 0) {
					asmout << "\tl.s\t$f12, " << symbolTable[index].alias << endl;
				} else if (symbolTable[index].scope > 0) {
					asmout << "\tl.s\t$f12, " << symbolTable[index].size << "($s0)" << endl;
				}
			} else {
				asmout << "\tli\t$v0, 3" << endl;
				if (symbolTable[index].scope == 0) {
					asmout << "\tl.d\t$f12, " << symbolTable[index].alias << endl;
				} else if (symbolTable[index].scope > 0) {
					asmout << "\tl.d\t$f12, " << symbolTable[index].size << "($s0)" << endl;
				}
			}
			asmout << "\tsyscall" << endl;
			asmout << "\tli\t$v0, 4" << endl;
			asmout << "\tla\t$a0, NEWLINE" << endl;
			asmout << "\tsyscall" << endl;
			return 5;
		}
	} else if (tree[step].compare("Expr") == 0) {
		if (tree[step + 1].compare("UnaryOp") == 0) {
			cost += generateCode(step + 1);
			cost += generateCode(step + cost + 1);
			if (tree[step + 2][0] == '-') {
				if (resultType.compare("int") == 0) {
					asmout << "\tneg\t$v0, $v0" << endl;
					resultType = "int";
				} else if (resultType.compare("float") == 0) {
					asmout << "\tneg.s\t$f0, $f0" << endl;
					resultType = "float";
				} else {
					asmout << "\tneg.d\t$f4, $f4" << endl;
					resultType = "double";
				}
			} else {
				asmout << "\tseq\t$v0, $v0, $zero" << endl;
				resultType = "int";
			}
			return cost + 1;
		} else if (tree[step + 1].compare("num") == 0) {
			int tmp = alias2index(tree[step + 2]);
			if (tree[step + 4].compare("epsilon") == 0) {
				if (symbolTable[tmp].type.compare("word") == 0) {
					asmout << "\tlw\t$v0, " << tree[step + 2] << endl;
					resultType = "int";
				} else {
					asmout << "\tl.d\t$f4, " << tree[step + 2] << endl;
					resultType = "double";
				}
				cost += 4;
			} else {
				cost += 5;
				cost += generateCode(step + cost + 1);
				if (symbolTable[tmp].type.compare("word") == 0) {
					asmout << "\tlw\t$t0, " << tree[step + 2] << endl;
				} else {
					asmout << "\tl.d\t$f6, " << tree[step + 2] << endl;
				}
				printAL(tree[step + 5], resultType);
			}
			return cost + 1;
		} else if (tree[step + 1].compare("(") == 0) {
			int typ;
			asmout << "\taddi\t$sp, $sp, -8" << endl;
			cost += 1;
			cost += generateCode(step + cost + 1);
			if (resultType.compare("int") == 0) {
				asmout << "\tsw\t$v0, 0($sp)" << endl;
				typ = 1;
			} else if (resultType.compare("float") == 0) {
				asmout << "\ts.s\t$f0, 0($sp)" << endl;
				typ = 2;
			} else {
				asmout << "\ts.d\t$f4, 0($sp)" << endl;
				typ = 3;
			}
			cost += 1;
			if (tree[step + cost + 2].compare("epsilon") == 0) {
				cost += 2;
				if (typ == 1) {
					asmout << "\tlw\t$v0, 0($sp)" << endl;
					resultType = "int";
				} else if (typ == 2) {
					asmout << "\tl.s\t$f0, 0($sp)" << endl;
					resultType = "float";
				} else {
					asmout << "\tl.d\t$f4, 0($sp)" << endl;
					resultType = "double";
				}
			} else {
				int tmp;
				tmp = generateCode(step + cost + 4);
				if (typ == 1) {
					asmout << "\tlw\t$t0, 0($sp)" << endl;
				} else if (typ == 2) {
					asmout << "\tl.s\t$f2, 0($sp)" << endl;
				} else {
					asmout << "\tl.d\t$f6, 0($sp)" << endl;
				}
				printAL(tree[step + cost + 3], resultType);
				cost += 3 + tmp;
			}
			asmout << "\taddi\t$sp, $sp, 8" << endl;
			return cost + 1;
		} else if (tree[step + 1].compare("id") == 0) {
			int in = alias2index(tree[step + 2]);
			string ty = symbolTable[in].type;
			if (tree[step + 4][0] == 'E') {
				if (tree[step + 5].compare("epsilon") == 0) {
					resultType = ty;
					if (symbolTable[in].isArray) {
						resultType = "array";
					} else if (ty.compare("int") == 0) {
						asmout << "\tlw\t$v0, ";
					} else if (ty.compare("float") == 0) {
						asmout << "\tl.s\t$f0, ";
					} else {
						asmout << "\tl.d\t$f4, ";
					}
					if (symbolTable[in].scope == 0) {
						asmout << tree[step + 2] << endl;
					} else if (resultType.compare("array") == 0) {
						asmout << "\taddi\t$v0, $s0, " << symbolTable[in].size << endl;
					} else {
						asmout << symbolTable[in].size << "($s0)" << endl;
					}
					cost += 5;
				} else {
					cost += 6;
					cost += generateCode(step + cost + 1);
					if (ty.compare("int") == 0) {
						asmout << "\tlw\t$t0, ";
					} else if (ty.compare("float") == 0) {
						asmout << "\tl.s\t$f2, ";
					} else {
						asmout << "\tl.d\t$f6, ";
					}
					if (symbolTable[in].scope == 0) {
						asmout << tree[step + 2] << endl;
					} else {
						asmout << symbolTable[in].size << "($s0)" << endl;
					}
					printAL(tree[step + 6], resultType);
				}
				return cost + 1;
			} else if (tree[step + 4][0] == '(') {
				int typ;
				funcCall = symbolTable[alias2index(tree[step + 2])].symbol;
				asmout << "\taddi\t$sp, $sp, -12" << endl;
				asmout << "\tsw\t$ra, 8($sp)" << endl;
				cost += 4;
				cost += generateCode(step + cost + 1);
				if (symbolTable[in].type.compare("int") == 0) {
					asmout << "\tsw\t$v0, 0($sp)" << endl;
					typ = 1;
				} else if (symbolTable[in].type.compare("float") == 0) {
					asmout << "\ts.s\t$f0, 0($sp)" << endl;
					typ = 2;
				} else {
					asmout << "\ts.d\t$f4, 0($sp)" << endl;
					typ = 3;
				}
				cost += 1;
				if (tree[step + cost + 2].compare("epsilon") == 0) {
					if (typ == 1) {
						asmout << "\tlw\t$v0, 0($sp)" << endl;
						resultType = "int";
					} else if (typ == 2) {
						asmout << "\tl.s\t$f0, 0($sp)" << endl;
						resultType = "float";
					} else {
						asmout << "\tl.d\t$f4, 0($sp)" << endl;
						resultType = "double";
					}
					cost += 2;
				} else {
					int tmp;
					tmp = generateCode(step + cost + 4);
					if (typ == 1) {
						asmout << "\tlw\t$t0, 0($sp)" << endl;
					} else if (typ == 2) {
						asmout << "\tl.s\t$f2, 0($sp)" << endl;
					} else {
						asmout << "\tl.d\t$f6, 0($sp)" << endl;
					}
					printAL(tree[step + cost + 3], resultType);
					cost += 3 + tmp;
				}
				asmout << "\tlw\t$ra, 8($sp)" << endl;
				asmout << "\taddi\t$sp, $sp, 12" << endl;
				return cost + 1;
			} else if (tree[step + 4][0] == '[') {
				cost += 4;
				cost += generateCode(step + cost + 1);
				if (symbolTable[in].type.compare("double") == 0) {
					asmout << "\taddi\t$t0, $zero, 8" << endl;
				} else {
					asmout << "\taddi\t$t0, $zero, 4" << endl;
				}
				printAL("*", "int");
				if (symbolTable[in].scope == 0) {
					asmout << "\tla\t$t0, " << tree[step + 2] << endl;
				} else {
					asmout << "\taddi\t$t0, $s0, " << symbolTable[in].size << endl;
					if (symbolTable[in].isPara) {
						asmout << "\tlw\t$t0, 0($t0)" << endl;
					}
				}
				printAL("+", "int");
				cost += 2;
				if (tree[step + cost + 1][0] == '=') {
					cost += 1;
					asmout << "\taddi\t$sp, $sp, -4" << endl;
					asmout << "\tsw\t$v0, 0($sp)" << endl;
					cost += generateCode(step + cost + 1);
					asmout << "\tlw\t$t0, 0($sp)" << endl;
					asmout << "\taddi\t$sp, $sp, 4" << endl;
					if (resultType.compare("int") == 0) {
						asmout << "\tsw\t$v0, 0($t0)" << endl;
					} else if(resultType.compare("float") == 0) {
						asmout << "\ts.s\t$f0, 0($t0)" << endl;
					} else {
						asmout << "\ts.d\t$f4, 0($t0)" << endl;
					}
				} else if (tree[step + cost + 2].compare("epsilon") == 0) {
					if (symbolTable[in].type.compare("int") == 0) {
						asmout << "\tlw\t$v0, 0($v0)" << endl;
						resultType = "int";
					} else if(symbolTable[in].type.compare("float") == 0) {
						asmout << "\tl.s\t$f0, 0($v0)" << endl;
						resultType = "float";
					} else {
						asmout << "\tl.d\t$f4, 0($v0)" << endl;
						resultType = "double";
					}
					cost += 2;
				} else {
					int tmp;
					tmp = cost + 3;
					asmout << "\taddi\t$sp, $sp, -4" << endl;
					asmout << "\tsw\t$v0, 0($sp)" << endl;
					tmp += generateCode(step + tmp + 1);
					asmout << "\tlw\t$t0, 0($sp)" << endl;
					asmout << "\taddi\t$sp, $sp, 4" << endl;
					if (symbolTable[in].type.compare("double") == 0) {
						asmout << "\tl.d\t$f6, 0($t0)" << endl;
					} else if (symbolTable[in].type.compare("float") == 0) {
						asmout << "\tl.s\t$f2, 0($t0)" << endl;
					} else {
						asmout << "\tlw\t$t0, 0($t0)" << endl;
					}
					printAL(tree[step + cost + 3], resultType);
					cost = tmp;
				}
				return cost + 1;
			} else if (tree[step + 4][0] == '=') {
				cost += 4;
				cost += generateCode(step + cost + 1);
				if (ty.compare("int") == 0) {
					if (symbolTable[in].scope == 0) {
						asmout << "\tsw\t$v0, " << tree[step + 2] << endl;
					} else {
						asmout << "\tsw\t$v0, " << symbolTable[in].size;
						asmout << "($s0)" << endl;
					}
				} else if (ty.compare("float") == 0) {
					if (symbolTable[in].scope == 0) {
						asmout << "\ts.s\t$f0, " << tree[step + 2] << endl;
					} else {
						asmout << "\ts.s\t$f0, " << symbolTable[in].size;
						asmout << "($s0)" << endl;
					}
				} else {
					if (symbolTable[in].scope == 0) {
						asmout << "\ts.d\t$f4, " << tree[step + 2] << endl;
					} else {
						asmout << "\ts.d\t$f4, " << symbolTable[in].size;
						asmout << "($s0)" << endl;
					}
				}
				return cost + 1;
			}
		}
	} else if (tree[step].compare("ExprList") == 0) {
		if (tree[step + 1].compare("epsilon") == 0) {
			return 2;
		} else {
			asmout << "\taddi\t$t0, $sp, " << (-(symbolTable[alias2index(tree[step - 3])].size)) << endl;
			paraSum = 0;
			return generateCode(step + 1) + 1;
		}
	} else if (tree[step].compare("ExprListTail") == 0) {
		int u;
		cost = generateCode(step + 1);
		if (resultType.compare("double") == 0) {
			asmout << "\ts.d\t$f4, " << paraSum << "($t0)" << endl;
			paraSum += 8;
			u = 1;
		} else if (resultType.compare("float") == 0) {
			asmout << "\ts.s\t$f0, " << paraSum << "($t0)" << endl;
			paraSum += 4;
			u = 2;
		} else {
			asmout << "\tsw\t$v0, " << paraSum << "($t0)" << endl;
			paraSum += 4;
			u = 3;
		}
		cost += generateCode(step + cost + 1);
		return cost + 1;
	} else if (tree[step].compare("ExprListTail'") == 0) {
		if (tree[step + 1].compare("epsilon") == 0) {
			asmout << "\tjal\t" << funcCall << endl;
			return 2;
		} else {
			cost += 1;
			cost += generateCode(step + cost + 2);
			return cost + 1;
		}
	}
	return cost;
}

void printAL(string s, string t) {
	if (s.compare("+") == 0) {
		if (t.compare("int") == 0) {
			asmout << "\tadd\t$v0, $t0, $v0" << endl;
		} else if (t.compare("float") == 0) {
			asmout << "\tadd.s\t$f0, $f2, $f0" << endl;
		} else {
			asmout << "\tadd.d\t$f4, $f6, $f4" << endl;
		}
	} else if (s.compare("-") == 0) {
		if (t.compare("int") == 0) {
			asmout << "\tsub\t$v0, $t0, $v0" << endl;
		} else if (t.compare("float") == 0) {
			asmout << "\tsub.s\t$f0, $f2, $f0" << endl;
		} else {
			asmout << "\tsub.d\t$f4, $f6, $f4" << endl;
		}
	} else if (s.compare("*") == 0) {
		if (t.compare("int") == 0) {
			asmout << "\tmul\t$v0, $t0, $v0" << endl;
		} else if (t.compare("float") == 0) {
			asmout << "\tmul.s\t$f0, $f2, $f0" << endl;
		} else {
			asmout << "\tmul.d\t$f4, $f6, $f4" << endl;
		}
	} else if (s.compare("/") == 0) {
		if (t.compare("int") == 0) {
			asmout << "\tdiv\t$v0, $t0, $v0" << endl;
		} else if (t.compare("float") == 0) {
			asmout << "\tdiv.s\t$f0, $f2, $f0" << endl;
		} else {
			asmout << "\tdiv.d\t$f4, $f6, $f4" << endl;
		}
	} else if (s.compare("==") == 0) {
		if (t.compare("int") == 0) {
			asmout << "\tseq\t$v0, $t0, $v0" << endl;
		} else if (t.compare("float") == 0) {
			asmout << "\tc.eq.s\t$f2, $f0" << endl;
		} else {
			asmout << "\tc.eq.d\t$f6, $f4" << endl;
		}
		if (t.compare("int") != 0) {
			asmout << "\tmove\t$v0, $zero" << endl;
			asmout << "\tmove\t$t8, $ra" << endl;
			asmout << "\tjal\tget_addr" << endl;
			asmout << "\tmove\t$ra, $t8" << endl;
			asmout << "\tbc1t\tset_true" << endl;
		}
	} else if (s.compare("!=") == 0) {
		if (t.compare("int") == 0) {
			asmout << "\tsne\t$v0, $t0, $v0" << endl;
		} else if (t.compare("float") == 0) {
			asmout << "\tc.eq.s\t$f2, $f0" << endl;
		} else {
			asmout << "\tc.eq.d\t$f6, $f4" << endl;
		}
		if (t.compare("int") != 0) {
			asmout << "\tmove\t$v0, $zero" << endl;
			asmout << "\tmove\t$t8, $ra" << endl;
			asmout << "\tjal\tget_addr" << endl;
			asmout << "\tmove\t$ra, $t8" << endl;
			asmout << "\tbc1f\tset_true" << endl;
		}
	} else if (s.compare("<") == 0) {
		if (t.compare("int") == 0) {
			asmout << "\tslt\t$v0, $t0, $v0" << endl;
		} else if (t.compare("float") == 0) {
			asmout << "\tc.lt.s\t$f2, $f0" << endl;
		} else {
			asmout << "\tc.lt.d\t$f6, $f4" << endl;
		}
		if (t.compare("int") != 0) {
			asmout << "\tmove\t$v0, $zero" << endl;
			asmout << "\tmove\t$t8, $ra" << endl;
			asmout << "\tjal\tget_addr" << endl;
			asmout << "\tmove\t$ra, $t8" << endl;
			asmout << "\tbc1t\tset_true" << endl;
		}
	} else if (s.compare("<=") == 0) {
		if (t.compare("int") == 0) {
			asmout << "\tsle\t$v0, $t0, $v0" << endl;
		} else if (t.compare("float") == 0) {
			asmout << "\tc.le.s\t$f2, $f0" << endl;
		} else {
			asmout << "\tc.le.d\t$f6, $f4" << endl;
		}
		if (t.compare("int") != 0) {
			asmout << "\tmove\t$v0, $zero" << endl;
			asmout << "\tmove\t$t8, $ra" << endl;
			asmout << "\tjal\tget_addr" << endl;
			asmout << "\tmove\t$ra, $t8" << endl;
			asmout << "\tbc1t\tset_true" << endl;
		}
	} else if (s.compare(">") == 0) {
		if (t.compare("int") == 0) {
			asmout << "\tsgt\t$v0, $t0, $v0" << endl;
		} else if (t.compare("float") == 0) {
			asmout << "\tc.le.s\t$f2, $f0" << endl;
		} else {
			asmout << "\tc.le.d\t$f6, $f4" << endl;
		}
		if (t.compare("int") != 0) {
			asmout << "\tmove\t$v0, $zero" << endl;
			asmout << "\tmove\t$t8, $ra" << endl;
			asmout << "\tjal\tget_addr" << endl;
			asmout << "\tmove\t$ra, $t8" << endl;
			asmout << "\tbc1f\tset_true" << endl;
		}
	} else if (s.compare(">=") == 0) {
		if (t.compare("int") == 0) {
			asmout << "\tsge\t$v0, $t0, $v0" << endl;
		} else if (t.compare("float") == 0) {
			asmout << "\tc.lt.s\t$f2, $f0" << endl;
		} else {
			asmout << "\tc.lt.d\t$f6, $f4" << endl;
		}
		if (t.compare("int") != 0) {
			asmout << "\tmove\t$v0, $zero" << endl;
			asmout << "\tmove\t$t8, $ra" << endl;
			asmout << "\tjal\tget_addr" << endl;
			asmout << "\tmove\t$ra, $t8" << endl;
			asmout << "\tbc1f\tset_true" << endl;
		}
	} else if (s.compare("&&") == 0) {
		// Temp
		asmout << "\tand\t$v0, $t0, $v0" << endl;
	} else if (s.compare("||") == 0) {
		asmout << "\tor\t$v0, $t0, $v0" << endl;
	}
	resultType = t;
}

int alias2index(string ali) {
	for (int i = 0; i < symbolTable.size(); ++i) {
		if (symbolTable[i].alias.compare(ali) == 0) {
			return i;
		}
	}
}

void generateSymbolTable() {
	stack<int> scopeStack;
	int scopeCount = 0, serial = 0, size, start, scope;
	bool isFunc, isArray, isPara;
	scopeStack.push(0);
	char buffer[20];
	string funcName;
	map<string, string> constMap;

//	cout << "scope\t" << "symbol\t" << "type\t" << "array\t" << "function" << endl;
	for (int i = 0; i < tree.size(); ++i) {
		string token = tree[i];
		isFunc = isArray = isPara = false;
		if (token[0] == '{') {
			++scopeCount;
			scopeStack.push(scopeCount);
		} else if (token[0] == '}') {
			scopeStack.pop();
			if (tree[i + 1].compare("DeclList") == 0) {
				for (int j = 0; j < symbolTable.size(); ++j) {
					if (symbolTable[j].symbol.compare(funcName) == 0) {
						symbolTable[j].size = start;
					}
				}
			}
		} else if (token.compare("Type") == 0) {
			size = 1;
			if (tree[i + 4].compare("Decl") == 0) {
				if (tree[i + 5].compare("FunDecl") == 0) {
					isFunc = true;
					funcName = tree[i + 3];
					start = 0;
				} else if (tree[i + 6][0] == '['){
					isArray = true;
					size = atoi(tree[i + 8].c_str());
				}
			} else if (tree[i + 4].compare("VarDecl'") == 0) {
				if (tree[i + 5][0] == '['){
					isArray = true;
					size = atoi(tree[i + 7].c_str());
				}
			} else {
				isPara = true;
				if (tree[i + 5][0] == '[') {
					isArray = true;
				}
			}
			sprintf(buffer, "VAR%03x", serial++);
			string alias (buffer);
			scope = (isPara ? scopeCount + 1 : scopeStack.top());
			size = (tree[i + 1].compare("double") == 0 ? size * 8 : size * 4);
			if (isPara && isArray) {
				size = 4;
			}
			if (scope == 0 && !isFunc) {
				start = size;
			}
			symbolTable.push_back(Var(
						scope,
						tree[i + 3],
						tree[i + 1],
						isFunc,
						isArray,
						alias,
						start,
						isPara)
			);
			if (!isFunc) {
				start += size;
			}
			tree[i + 3] = alias;
			i += 3;
		} else if (token.compare("num") == 0) {
			if (constMap.find(tree[i + 1]) != constMap.end()) {
				tree[i + 1] = constMap[tree[i + 1]];
				continue;
			}
			sprintf(buffer, "VAR%03x", serial++);
			string alias (buffer);
			symbolTable.push_back(Var(
						-1,
						tree[i + 1],
						(tree[i + 1].find(".") == -1 ? "word" : "double"),
						false,
						false,
						alias,
						0,
						isPara)
			);
			constMap[tree[i + 1]] = alias;
			tree[i + 1] = alias;
		} else if (token.compare("id") == 0) {
			for (int j = 0; j < symbolTable.size(); ++j) {
				// temp
				if (symbolTable[j].symbol.compare(tree[i + 1]) == 0 &&
						(symbolTable[j].scope == scopeStack.top() ||
						 symbolTable[j].scope < scopeStack.top())) {
					tree[i + 1] = symbolTable[j].alias;
				}
			}
			i += 1;
		}
	}
/*	sort(symbolTable.begin(), symbolTable.end());
	for (int i = 0; i < symbolTable.size(); ++i) {
		if (symbolTable[i].scope < 0) {
			continue;
		}
		cout << symbolTable[i].scope << "\t";
		cout << symbolTable[i].symbol << "\t";
		cout << symbolTable[i].type << "\t";
		cout << symbolTable[i].size << "\t";
		cout << (symbolTable[i].isArray ? "true" : "false") << "\t";
		cout << (symbolTable[i].isFunc ? "true" : "false") << endl;
	}*/
}

void generateTree() {
	int level, c;
	stack<string> symbolStack;
	stack<int> levelStack;
	stack<int> scopeStack;
	string token, tmp;
	void *ptr;
	fstream fs;

	fs.open(INPUT_FILE, fstream::in);

	symbolStack.push("S");
	levelStack.push(1);

	while (!symbolStack.empty()) {
		ptr = (fs >> token);
		if (ptr == NULL) {
			token = "$";
		}
		if (isdigit(token[0])) {
			tmp = token;
			token = "num";
		} else if (!isKeyword(token)) {
			tmp = token;
			token = "id";
		}

		while (!isTerminal(symbolStack.top())) {
			pair<string, string> p(symbolStack.top(), token);
			level = levelStack.top();
			tree.push_back(symbolStack.top());
			symbolStack.pop();
			levelStack.pop();
			++level;
			for (int k = (LLtable[p].begin()->size()) - 1; k >= 0; --k) {
				symbolStack.push((*LLtable[p].begin())[k]);
				levelStack.push(level);
			}
			while (symbolStack.top().compare("epsilon") == 0) {
				tree.push_back(symbolStack.top());
				symbolStack.pop();
				levelStack.pop();
			}
		}
		tree.push_back(token);
		if (token.compare("id") == 0 || token.compare("num") == 0) {
			tree.push_back(tmp);
		}
		symbolStack.pop();
		levelStack.pop();
	}
	fs.close();
}

bool isKeyword(string token) {
	for (set<string>::iterator it = symbolSet.begin(); it != symbolSet.end(); ++it) {
		if (token.compare(*it) == 0) {
			return true;
		}
	}
	return false;
}

void generateLLtable() {
	map<string, Productions>::iterator it;
	Productions::iterator itP;
	set<string>::iterator itS;
	for (it = grammar.begin(); it != grammar.end(); ++it) {
		for (itP = grammar[it->first].begin(); itP != grammar[it->first].end(); ++itP) {
			set<string> s = getFirstSet(*itP);
			for (itS = s.begin(); itS != s.end(); ++itS) {
				if (itS->compare("epsilon") != 0) {
					pair<string, string> p(it->first, *itS);
					LLtable[p].insert(*itP);
				}
			}
			if (s.find("epsilon") != s.end()) {
				for (itS = followSet[it->first].begin(); itS != followSet[it->first].end(); ++itS) {
					pair<string, string> p(it->first, *itS);
					LLtable[p].insert(*itP);
				}
			}
		}
	}
}

void generateSet() {

	for (map<string, Productions>::iterator it = grammar.begin(); it != grammar.end(); ++it) {
		set<string> tmp = getFirstSet(it->first);
	}

	generateFollowSet();

}

void generateFollowSet() {
	bool done = false, building;
	int n;
	map<string, Productions>::iterator itA, itB;
	set<string> s;
	set<string>::iterator itS;
	Productions::iterator itP;
	Production::iterator v, w;
	Production p;
	followSet["S"].insert("$");
	while (!done) {
		building = false;
		for (itA = grammar.begin(); itA != grammar.end(); ++itA) {
			for (itB = grammar.begin(); itB != grammar.end(); ++itB) {
				for (itP = grammar[itA->first].begin(); itP != grammar[itA->first].end(); ++itP) {
					w = find(itP->begin(), itP->end(), itB->first);
					n = followSet[itB->first].size();
					if (w + 1 == itP->end()) {
						for (itS = followSet[itA->first].begin(); itS != followSet[itA->first].end(); ++itS) {
							followSet[itB->first].insert(*itS);
						}
						if (followSet[itB->first].size() > n) {
							building = true;
						}
					} else if ( w != itP->end()) {
						p.clear();
						for (v = w + 1; v != itP->end(); ++v) {
							p.push_back(*v);
						}
						s = getFirstSet(p);
						for (itS = s.begin(); itS != s.end(); ++itS) {
							followSet[itB->first].insert(*itS);
						}
						followSet[itB->first].erase("epsilon");
						if (canReduceToEpsilon(p, 0)) {
							for (itS = followSet[itA->first].begin(); itS != followSet[itA->first].end(); ++itS) {
								followSet[itB->first].insert(*itS);
							}
						}
						if (followSet[itB->first].size() > n) {
							building = true;
						}
					}
				}
			}
		}
		if (building == false) {
			done = true;
		}
	}
}

bool canReduceToEpsilon(Production p, int level) {
	bool allEpsilon = true;
	for (int i = 0; i < p.size(); ++i) {
		allEpsilon &= canReduceToEpsilon(p[i], level + 1);
	}
	return allEpsilon;
}

bool canReduceToEpsilon(string symbol, int level) {
	bool hasEpsilon = true;
	if (level > RECUR_LEVEL) {
		return false;
	}
	if (isTerminal(symbol)) {
		return symbol.compare("epsilon") == 0;
	} else {
		for (Productions::iterator it = grammar[symbol].begin(); it != grammar[symbol].end(); ++it) {
			if (canReduceToEpsilon(*it, level + 1)) {
				return true;
			}
		}
	}
	return false;
}

set<string> &getFirstSet(Production p) {
	static set<string> u;
	bool hasEpsilon;
	u.clear();
	if (isEpsilonProduct(p)) {
		u.insert("epsilon");
		return u;
	}
	for (int i = 0; i < p.size(); ++i) {
		set<string> v(getFirstSet(p[i]));
		hasEpsilon = v.find("epsilon") != v.end();
		v.erase("epsilon");
		for (set<string>::iterator s = v.begin(); s != v.end(); ++s) {
			u.insert(*s);
		}
		if (!hasEpsilon) {
			break;
		}
	}
	if (hasEpsilon) {
		u.insert("epsilon");
	}
	return u;
}

set<string> &getFirstSet(string symbol) {
	if (firstSet[symbol].size() != 0) {
		return firstSet[symbol];
	}
	if (isTerminal(symbol)) {
		firstSet[symbol].insert(symbol);
		return firstSet[symbol];
	} else {
		if (hasEpsilonProduct(symbol)) {
			firstSet[symbol].insert("epsilon");
		} else {
		}
	}
	for (Productions::iterator p = grammar[symbol].begin(); p != grammar[symbol].end(); ++p) {
		bool hasEpsilon;
		if (isEpsilonProduct(*p)) {
			continue;
		}
		for (int i = 0; i < p->size(); ++i) {
			set<string> v(getFirstSet((*p)[i]));
			hasEpsilon = v.find("epsilon") != v.end();
			v.erase("epsilon");
			for (set<string>::iterator s = v.begin(); s != v.end(); ++s) {
				firstSet[symbol].insert(*s);
			}
			if (!hasEpsilon) {
				break;
			}
		}
		if (hasEpsilon) {
			firstSet[symbol].insert("epsilon");
		}
	}
	return firstSet[symbol];
}

bool hasEpsilonProduct(string symbol) {
	for (Productions::iterator it = grammar[symbol].begin(); it != grammar[symbol].end(); ++it) {
		if (isEpsilonProduct(*it)) {
			return true;
		}
	}
	return false;
}

bool isEpsilonProduct(Production p) {
	return (p.size() == 1) && (p[0].compare("epsilon") == 0);
}

bool isTerminal(string symbol) {
	return grammar.find(symbol) == grammar.end();
}

void getGrammar() {
	char str[LEN_CHAR_ARRAY], varName[LEN_CHAR_ARRAY];
	FILE *fptr = fopen(GRAMMAR_FILE, "r");

	while (fgets(str, sizeof(str), fptr)) {
		if (str[strlen(str) - 1] == '\n') {
			str[strlen(str) - 1] = '\0';
		}
		if (str[0] != '\t') {
			strcpy(varName, str);
			string s(varName);
			symbolSet.insert(s);
		} else {
			Production p;
			char *tok = strtok(str, "\t ");
			while (tok != NULL) {
				string s(tok);
				p.push_back(s);
				symbolSet.insert(s);
				tok = strtok(NULL, "\t ");
			}
			string s(varName);
			grammar[s].push_back(p);
		}
	}

	fclose(fptr);
}
