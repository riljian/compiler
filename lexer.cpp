#include <cstdio>
#include <cstring>
#define NUM_STATE 8
#define NUM_SBL 128
#define NUM_KEY 11
#define NUM_OP 7
#define MAXL 1000

enum {START, OPERATOR, SPECIAL, IDENTIFIER, NUMBER, CHAR, ERROR, SPACE};
const char *keywords[NUM_KEY] = {"int", "char", "float", "double", "return", "if", "else", "while", "break", "for", "print"};
const char *operators[NUM_OP] = {"!=", "==", ">=", "<=", "&&", "||", "//"};

int main(int argc, char **argv) {
	char input[MAXL], DFA[NUM_STATE][NUM_SBL], buffer[MAXL];
	unsigned char c;
	int i, statePre, stateNow, lineCnt, flagH, flagT;
	
	// Open files for reading and writing.
	freopen("main.c", "r", stdin);
	freopen("token.txt", "w", stdout);
	
	// Initialize
	lineCnt = 0;
	memset(DFA, -1, sizeof(DFA));
	for (c = 0; c < NUM_SBL; ++c) {
		if ((c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z') || c == '_') {
			DFA[START][c] = DFA[OPERATOR][c] = DFA[SPECIAL][c] = DFA[IDENTIFIER][c] = DFA[SPACE][c] = IDENTIFIER;
			DFA[NUMBER][c] = DFA[ERROR][c] = ERROR;
		} else if ((c >= '0' && c <= '9') || c == '.') {
			DFA[START][c] = DFA[OPERATOR][c] = DFA[SPECIAL][c] = DFA[NUMBER][c] =DFA[SPACE][c] = NUMBER;
			DFA[IDENTIFIER][c] = IDENTIFIER;
			DFA[ERROR][c] = ERROR;
		} else {
			switch (c) {
				case '\0':
				case '\n':
				case '\r':
				case ' ':
				case '\t':
					for (i = START; i < NUM_STATE; ++i)
						DFA[i][c] = SPACE;
					break;
				case '!':
				case '=':
				case '+':
				case '-':
				case '*':
				case '/':
				case '<':
				case '>':
				case '&':
				case '|':
					for (i = START; i < NUM_STATE; ++i)
						DFA[i][c] = OPERATOR;
					break;
				case '(':
				case ')':
				case '[':
				case ']':
				case '{':
				case '}':
				case ';':
				case ',':
					for (i = START; i < NUM_STATE; ++i)
						DFA[i][c] = SPECIAL;
					DFA[SPECIAL][c] = SPACE;
					break;
				case '\'':
					for (i = START; i < NUM_STATE; ++i)
						DFA[i][c] = CHAR;
					break;
				default:
					break;
			}
		}
		DFA[CHAR][c] = (c == '\'') ? SPACE : CHAR;
	}

	// Process
	while (fgets(input, sizeof(input), stdin)) {
		printf("Line%3d:\n", ++lineCnt);
		flagH = flagT = 0;
		while (flagH < strlen(input)) {
			statePre = START;
			stateNow = DFA[START][input[flagT]];
			do {
				if (statePre == OPERATOR) {
					strncpy(buffer, input + flagH, 2);
					buffer[2] = '\0';
					for (i = 0; i < NUM_OP; ++i)
						if (strcmp(operators[i], buffer) == 0)
							break;
					if (i < NUM_OP)
						++flagT;
					break;
				}
				statePre = stateNow;
				++flagT;
				if (flagT >= strlen(input))
					break;
				stateNow = DFA[statePre][input[flagT]];
			} while (statePre == stateNow || stateNow == ERROR);
			strncpy(buffer, input + flagH, flagT - flagH);
			buffer[flagT - flagH] = '\0';
			if (statePre == SPACE) {
				flagH = flagT;
				continue;
			}
			else if (statePre == OPERATOR && strcmp("//", buffer) == 0)
				break;
			putchar('\t');
			switch (statePre) {
				case OPERATOR:
					printf("<Operator>\t\t: ");
					break;
				case SPECIAL:
					printf("<Special>\t\t: ");
					break;
				case IDENTIFIER:
					// Check for keywords
					for (i = 0; i < NUM_KEY; ++i)
						if (strcmp(keywords[i], buffer) == 0)
							break;
					if (i < NUM_KEY)
						printf("<Keyword>\t\t: ");
					else
						printf("<Identifier>\t: ");
					break;
				case NUMBER:
					printf("<Number>\t\t: ");
					break;
				case CHAR:
					printf("<Char>\t\t\t: ");
					++flagT;
					buffer[strlen(buffer) + 1] = '\0';
					buffer[strlen(buffer)] = '\'';
					break;
				case ERROR:
					printf("<Error>\t\t\t: ");
					break;
				default:
					break;
			}// switch
			printf("%s\n", buffer);
			flagH = flagT;
		}// process line while
	}// process file while
}
