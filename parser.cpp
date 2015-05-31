#include <vector>
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
#define SET_FILE "set.txt"
#define LLTABLE_FILE "LLtable.txt"
#define TREE_FILE "tree.txt"
#define INPUT_FILE "main.c"

typedef vector<vector<string> > Productions;
typedef vector<string> Production;
map<string, Productions> grammar;
map<string, set<string> > firstSet;
map<string, set<string> > followSet;
map<pair<string, string>, set<Production> > LLtable;
set<string> symbolSet;

void getGrammar();
bool hasEpsilonProduct(string symbol);
bool isEpsilonProduct(Production p);
bool isTerminal(string symbol);
bool canReduceToEpsilon(Production p, int level);
bool canReduceToEpsilon(string symbol, int level);
bool isKeyword(string token);
set<string> &getFirstSet(string symbol);
set<string> &getFirstSet(Production p);
void generateFollowSet();
void generateLLtable();
void outputSetFile();
void outputLLtable();
void outputTree();

int main(int argc, char **argv) {

	getGrammar();

	outputSetFile();

	outputLLtable();

	outputTree();

	return 0;
}

void outputTree() {
	int level;
	stack<string> symbolStack;
	stack<int> levelStack;
	string token, tmp;
	void *ptr;

	freopen(TREE_FILE, "w", stdout);
	freopen(INPUT_FILE, "r", stdin);

	symbolStack.push("S");
	levelStack.push(1);

	while (!symbolStack.empty()) {
		ptr = (cin >> token);
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
			for (int i = 0; i < level - 1; ++i) {
				cout << ' ';
			}
			cout << level << ' ';
			cout << symbolStack.top() << endl;
			symbolStack.pop();
			levelStack.pop();
			++level;
			for (int i = LLtable[p].begin()->size() - 1; i >= 0; --i) {
				symbolStack.push((*LLtable[p].begin())[i]);
				levelStack.push(level);
			}
			while (symbolStack.top().compare("epsilon") == 0) {
				for (int i = 0; i < levelStack.top() - 1; ++i) {
					cout << ' ';
				}
				cout << levelStack.top() << ' ';
				cout << symbolStack.top() << endl;
				symbolStack.pop();
				levelStack.pop();
			}
		}
		for (int i = 0; i < levelStack.top() - 1; ++i) {
			cout << ' ';
		}
		cout << levelStack.top() << ' ' << token << endl;
		if (token.compare("id") == 0 || token.compare("num") == 0) {
			for (int i = 0; i < levelStack.top(); ++i) {
				cout << ' ';
			}
			cout << levelStack.top() + 1 << ' ' << tmp << endl;
		}
		symbolStack.pop();
		levelStack.pop();
	}
	
	fclose(stdin);
	fclose(stdout);
}

bool isKeyword(string token) {
	for (set<string>::iterator it = symbolSet.begin(); it != symbolSet.end(); ++it) {
		if (token.compare(*it) == 0) {
			return true;
		}
	}
	return false;
}

void outputLLtable() {
	map<pair<string, string>, set<Production> >::iterator it;
	set<Production>::iterator itS;

	freopen(LLTABLE_FILE, "w", stdout);

	generateLLtable();

	cout << "S" << endl;

	for (it = LLtable.begin(); it != LLtable.end(); ++it) {
		for (itS = LLtable[it->first].begin(); itS != LLtable[it->first].end(); ++itS) {
			cout << setw(25) << left << it->first.first << setw(25) << left << it->first.second;
			for (int i = 0; i < itS->size(); ++i) {
				cout << (*itS)[i] << ' ';
			}
			cout << endl;
		}
	}

	fclose(stdout);
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

void outputSetFile() {

	freopen(SET_FILE, "w", stdout);

	cout << "First" << endl;

	for (map<string, Productions>::iterator it = grammar.begin(); it != grammar.end(); ++it) {
		set<string> tmp = getFirstSet(it->first);
		cout << setw(25) << left << it->first << ':';
		for (set<string>::iterator s = tmp.begin(); s != tmp.end(); ++s) {
			cout << *s << ' ';
		}
		cout << endl;
	}

	generateFollowSet();

	cout << '\n' << "Follow" << endl;

	for (map<string, set<string> >::iterator it = followSet.begin(); it != followSet.end(); ++it) {
		cout << setw(25) << left << it->first << ':';
		for (set<string>::iterator s = it->second.begin(); s != it->second.end(); ++s) {
			cout << *s << ' ';
		}
		cout << endl;
	}

	fclose(stdout);

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
