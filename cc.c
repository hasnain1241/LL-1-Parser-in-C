#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <stdbool.h>
#include <locale.h>


#define MAX_PRODS 100        // Maximum number of productions
#define MAX_SYMBOLS 100      // Maximum number of symbols in the grammar
#define MAX_RHS 50           // Maximum number of RHS alternatives per production
#define MAX_PROD_LEN 100     // Maximum length of a production
#define MAX_LINE_LEN 256     // Maximum line length in input file
#define MAX_TERMINALS 100    // Maximum number of terminals
#define MAX_NON_TERMINALS 50 // Maximum number of non-terminals
#define EPSILON "ε"          // Epsilon symbol

// Structure for a production rule
typedef struct {
    char lhs[20];                  // Left-hand side non-terminal
    char rhs[MAX_RHS][MAX_PROD_LEN]; // Right-hand side alternatives
    int numRHS;                    // Number of RHS alternatives
} Production;

// Structure for a grammar
typedef struct {
    Production productions[MAX_PRODS];
    int numProductions;
    char terminals[MAX_TERMINALS][20];
    int numTerminals;
    char nonTerminals[MAX_NON_TERMINALS][20];
    int numNonTerminals;
    char startSymbol[20];
} Grammar;

// Structure for FIRST and FOLLOW sets
typedef struct {
    char symbol[20];
    char elements[MAX_TERMINALS][20];
    int numElements;
} Set;

// Structure for LL(1) parsing table
typedef struct {
    char nonTerminal[20];
    char terminal[20];
    char production[MAX_PROD_LEN];
} ParseTableEntry;

typedef struct {
    ParseTableEntry entries[MAX_NON_TERMINALS * MAX_TERMINALS];
    int numEntries;
    char terminals[MAX_TERMINALS][20];
    int numTerminals;
    char nonTerminals[MAX_NON_TERMINALS][20];
    int numNonTerminals;
} ParseTable;

// Function prototypes
Grammar readGrammarFromFile(const char* filename);
void displayGrammar(Grammar grammar);
Grammar leftFactoring(Grammar grammar);
Grammar leftRecursionRemoval(Grammar grammar);
Set* computeFirstSets(Grammar grammar);
Set* computeFollowSets(Grammar grammar, Set* firstSets);
ParseTable constructLL1Table(Grammar grammar, Set* firstSets, Set* followSets);
void displayFirstSets(Set* firstSets, int numNonTerminals);
void displayFollowSets(Set* followSets, int numNonTerminals);
void displayParseTable(ParseTable table);
bool isTerminal(Grammar grammar, const char* symbol);
bool isNonTerminal(Grammar grammar, const char* symbol);
void addToSet(Set* set, const char* element);
bool isInSet(Set set, const char* element);
char** splitString(const char* str, const char* delimiter, int* count);
char* trimString(char* str);
void addNewProduction(Grammar* grammar, const char* lhs, char rhs[][MAX_PROD_LEN], int numRHS);
bool hasCommonPrefix(char* rhs1, char* rhs2, char* prefix);
char* getPrefix(char* rhs1, char* rhs2);
void removePrefix(char* str, const char* prefix, char* result);
bool hasDirectLeftRecursion(Production prod);
char* getSymbol(const char* rhs, int* pos);
void freeSet(Set* set, int count);
void writeOutputToFile(Grammar original, Grammar leftFactored, Grammar withoutLeftRecursion, 
                      Set* firstSets, Set* followSets, ParseTable parseTable, const char* filename);

int main() {
    Grammar grammar = readGrammarFromFile("g1.txt");
    printf("Original Grammar:\n");
    displayGrammar(grammar);
    
    // Left Factoring
    Grammar leftFactoredGrammar = leftFactoring(grammar);
    printf("\nGrammar after Left Factoring:\n");
    displayGrammar(leftFactoredGrammar);
    
    // Left Recursion Removal
    Grammar grammarWithoutLeftRecursion = leftRecursionRemoval(leftFactoredGrammar);
    printf("\nGrammar after Left Recursion Removal:\n");
    displayGrammar(grammarWithoutLeftRecursion);
    
    // Compute FIRST sets
    Set* firstSets = computeFirstSets(grammarWithoutLeftRecursion);
    printf("\nFIRST Sets:\n");
    displayFirstSets(firstSets, grammarWithoutLeftRecursion.numNonTerminals);
    
    // Compute FOLLOW sets
    Set* followSets = computeFollowSets(grammarWithoutLeftRecursion, firstSets);
    printf("\nFOLLOW Sets:\n");
    displayFollowSets(followSets, grammarWithoutLeftRecursion.numNonTerminals);
    
    // Construct LL(1) parsing table
    ParseTable parseTable = constructLL1Table(grammarWithoutLeftRecursion, firstSets, followSets);
    printf("\nLL(1) Parsing Table:\n");
    displayParseTable(parseTable);
    
    // Write output to file
    writeOutputToFile(grammar, leftFactoredGrammar, grammarWithoutLeftRecursion, 
                     firstSets, followSets, parseTable, "output.txt");
    
    // Free allocated memory
    freeSet(firstSets, grammarWithoutLeftRecursion.numNonTerminals);
    freeSet(followSets, grammarWithoutLeftRecursion.numNonTerminals);
    
    return 0;
}


/*
Grammar readGrammarFromFile(const char* filename) {
    FILE* file = fopen(filename, "r");
    Grammar grammar;
    grammar.numProductions = 0;
    grammar.numTerminals = 0;
    grammar.numNonTerminals = 0;

    if (file == NULL) {
        printf("Error opening file: %s\n", filename);
        return grammar;
    }

    char line[MAX_LINE_LEN];
    int lineNum = 0;

    while (fgets(line, MAX_LINE_LEN, file) != NULL) {
        line[strcspn(line, "\n")] = 0;  // Remove newline character
        if (strlen(line) == 0) continue; // Skip empty lines

        printf("\nProcessing Line %d: %s\n", lineNum + 1, line);  // Debugging

        char* trimmedLine = trimString(line);

        // Split line into LHS and RHS
        char* arrow = strstr(trimmedLine, "->");
        if (arrow == NULL) {
            printf("Invalid grammar format at line %d\n", lineNum + 1);
            continue;
        }

        // Extract LHS
        *arrow = '\0';
        char* lhs = trimString(trimmedLine);
        printf("  - Found LHS: %s\n", lhs);  // Debugging

        // Ensure LHS is a non-terminal (it must be uppercase)
        if (isupper(lhs[0])) {
            bool found = false;
            for (int i = 0; i < grammar.numNonTerminals; i++) {
                if (strcmp(grammar.nonTerminals[i], lhs) == 0) {
                    found = true;
                    break;
                }
            }
            if (!found) {
                strcpy(grammar.nonTerminals[grammar.numNonTerminals], lhs);
                printf("  - Added Non-Terminal: %s\n", lhs);  // Debugging
                grammar.numNonTerminals++;

                // The first non-terminal is the start symbol
                if (grammar.numNonTerminals == 1) {
                    strcpy(grammar.startSymbol, lhs);
                    printf("  - Start Symbol Set: %s\n", lhs);  // Debugging
                }
            }
        } else {
            printf("  - ERROR: LHS is not an uppercase non-terminal: %s\n", lhs);  // Debugging
        }

        // Extract RHS
        char* rhsStr = trimString(arrow + 2);
        printf("  - Found RHS: %s\n", rhsStr);  // Debugging

        // Split RHS by '|'
        int numAlternatives;
        char** alternatives = splitString(rhsStr, "|", &numAlternatives);

        // Create a new production
        Production* prod = &grammar.productions[grammar.numProductions];
        strcpy(prod->lhs, lhs);
        prod->numRHS = numAlternatives;

        for (int i = 0; i < numAlternatives; i++) {
            char* trimmedAlt = trimString(alternatives[i]);
            strcpy(prod->rhs[i], trimmedAlt);
            printf("  - Added RHS Alternative: %s\n", trimmedAlt);  // Debugging

            // **NEW FIX: Correctly handle uppercase followed by lowercase (Aa case)**
            int pos = 0;
            while (trimmedAlt[pos] != '\0') {
                char symbol[3] = {trimmedAlt[pos], '\0', '\0'};  // Single character symbol

                // If next character exists and is lowercase, handle it separately
                if (isupper(trimmedAlt[pos]) && islower(trimmedAlt[pos + 1])) {
                    symbol[0] = trimmedAlt[pos];   // First uppercase letter
                    symbol[1] = '\0';             // Ensure single-character symbol

                    // Add non-terminal
                    bool foundNT = false;
                    for (int j = 0; j < grammar.numNonTerminals; j++) {
                        if (strcmp(grammar.nonTerminals[j], symbol) == 0) {
                            foundNT = true;
                            break;
                        }
                    }
                    if (!foundNT) {
                        strcpy(grammar.nonTerminals[grammar.numNonTerminals], symbol);
                        grammar.numNonTerminals++;
                        printf("  - Added Non-Terminal: %s\n", symbol);  // Debugging
                    }

                    // Now handle the lowercase letter as a terminal
                    symbol[0] = trimmedAlt[pos + 1];  
                    symbol[1] = '\0';  

                    bool foundT = false;
                    for (int j = 0; j < grammar.numTerminals; j++) {
                        if (strcmp(grammar.terminals[j], symbol) == 0) {
                            foundT = true;
                            break;
                        }
                    }
                    if (!foundT) {
                        strcpy(grammar.terminals[grammar.numTerminals], symbol);
                        grammar.numTerminals++;
                        printf("  - Added Terminal: %s\n", symbol);  // Debugging
                    }
                    pos += 2;  // Move ahead since we processed two characters
                    continue;
                }

                // If it's a non-terminal (uppercase)
                if (isupper(symbol[0])) {
                    bool foundNT = false;
                    for (int j = 0; j < grammar.numNonTerminals; j++) {
                        if (strcmp(grammar.nonTerminals[j], symbol) == 0) {
                            foundNT = true;
                            break;
                        }
                    }
                    if (!foundNT) {
                        strcpy(grammar.nonTerminals[grammar.numNonTerminals], symbol);
                        grammar.numNonTerminals++;
                        printf("  - Added Non-Terminal: %s\n", symbol);  // Debugging
                    }
                } else {  // It's a terminal
                    bool foundT = false;
                    for (int j = 0; j < grammar.numTerminals; j++) {
                        if (strcmp(grammar.terminals[j], symbol) == 0) {
                            foundT = true;
                            break;
                        }
                    }
                    if (!foundT) {
                        strcpy(grammar.terminals[grammar.numTerminals], symbol);
                        grammar.numTerminals++;
                        printf("  - Added Terminal: %s\n", symbol);  // Debugging
                    }
                }
                pos++;  // Move to the next character
            }
            free(trimmedAlt);
        }

        grammar.numProductions++;

        // Free allocated memory
        for (int i = 0; i < numAlternatives; i++) {
            free(alternatives[i]);
        }
        free(alternatives);
        free(trimmedLine);
        free(lhs);
        free(rhsStr);

        lineNum++;
    }

    fclose(file);
    return grammar;
}
*/


Grammar readGrammarFromFile(const char* filename) {
    FILE* file = fopen(filename, "r");
    Grammar grammar;
    grammar.numProductions = 0;
    grammar.numTerminals = 0;
    grammar.numNonTerminals = 0;

    if (file == NULL) {
        printf("Error opening file: %s\n", filename);
        return grammar;
    }

    char line[MAX_LINE_LEN];
    int lineNum = 0;

    while (fgets(line, MAX_LINE_LEN, file) != NULL) {
        line[strcspn(line, "\n")] = 0;  // Remove newline character
        if (strlen(line) == 0) continue; // Skip empty lines

        printf("\nProcessing Line %d: %s\n", lineNum + 1, line);  // Debugging

        char* trimmedLine = trimString(line);

        // Split line into LHS and RHS
        char* arrow = strstr(trimmedLine, "->");
        if (arrow == NULL) {
            printf("Invalid grammar format at line %d\n", lineNum + 1);
            continue;
        }

        // Extract LHS
        *arrow = '\0';
        char* lhs = trimString(trimmedLine);
        printf("  - Found LHS: %s\n", lhs);  // Debugging

        // Ensure LHS is a non-terminal (must be uppercase)
        if (isupper(lhs[0])) {
            bool found = false;
            for (int i = 0; i < grammar.numNonTerminals; i++) {
                if (strcmp(grammar.nonTerminals[i], lhs) == 0) {
                    found = true;
                    break;
                }
            }
            if (!found) {
                strcpy(grammar.nonTerminals[grammar.numNonTerminals], lhs);
                printf("  - Added Non-Terminal: %s\n", lhs);  // Debugging
                grammar.numNonTerminals++;

                // The first non-terminal is the start symbol
                if (grammar.numNonTerminals == 1) {
                    strcpy(grammar.startSymbol, lhs);
                    printf("  - Start Symbol Set: %s\n", lhs);  // Debugging
                }
            }
        } else {
            printf("  - ERROR: LHS is not an uppercase non-terminal: %s\n", lhs);  // Debugging
        }

        // Extract RHS
        char* rhsStr = trimString(arrow + 2);
        printf("  - Found RHS: %s\n", rhsStr);  // Debugging

        // Split RHS by '|'
        int numAlternatives;
        char** alternatives = splitString(rhsStr, "|", &numAlternatives);

        // Create a new production
        Production* prod = &grammar.productions[grammar.numProductions];
        strcpy(prod->lhs, lhs);
        prod->numRHS = numAlternatives;

        for (int i = 0; i < numAlternatives; i++) {
            char* trimmedAlt = trimString(alternatives[i]);
            strcpy(prod->rhs[i], trimmedAlt);
            printf("  - Added RHS Alternative: %s\n", trimmedAlt);  // Debugging

            // **NEW FIX: Properly handle uppercase and lowercase character sequences**
            int pos = 0;
            while (trimmedAlt[pos] != '\0') {
                char symbol[3] = {trimmedAlt[pos], '\0', '\0'};  // Single character symbol

                // If uppercase, treat as non-terminal
                if (isupper(symbol[0])) {
                    bool foundNT = false;
                    for (int j = 0; j < grammar.numNonTerminals; j++) {
                        if (strcmp(grammar.nonTerminals[j], symbol) == 0) {
                            foundNT = true;
                            break;
                        }
                    }
                    if (!foundNT) {
                        strcpy(grammar.nonTerminals[grammar.numNonTerminals], symbol);
                        grammar.numNonTerminals++;
                        printf("  - Added Non-Terminal: %s\n", symbol);  // Debugging
                    }
                } 
                // If lowercase or special symbol, treat as terminal
                else {
                    bool foundT = false;
                    for (int j = 0; j < grammar.numTerminals; j++) {
                        if (strcmp(grammar.terminals[j], symbol) == 0) {
                            foundT = true;
                            break;
                        }
                    }
                    if (!foundT) {
                        strcpy(grammar.terminals[grammar.numTerminals], symbol);
                        grammar.numTerminals++;
                        printf("  - Added Terminal: %s\n", symbol);  // Debugging
                    }
                }
                pos++;  // Move to the next character
            }
            free(trimmedAlt);
        }

        grammar.numProductions++;

        // Free allocated memory
        for (int i = 0; i < numAlternatives; i++) {
            free(alternatives[i]);
        }
        free(alternatives);
        free(trimmedLine);
        free(lhs);
        free(rhsStr);

        lineNum++;
    }

    fclose(file);
    return grammar;
}


// Display the grammar
void displayGrammar(Grammar grammar) {
    printf("Productions:\n");
    for (int i = 0; i < grammar.numProductions; i++) {
        Production prod = grammar.productions[i];
        printf("%s -> ", prod.lhs);
        for (int j = 0; j < prod.numRHS; j++) {
            printf("%s", prod.rhs[j]);
            if (j < prod.numRHS - 1) {
                printf(" | ");
            }
        }
        printf("\n");
    }
    
    printf("\nNon-terminals: ");
    for (int i = 0; i < grammar.numNonTerminals; i++) {
        printf("%s", grammar.nonTerminals[i]);
        if (i < grammar.numNonTerminals - 1) {
            printf(", ");
        }
    }
    
    printf("\nTerminals: ");
    for (int i = 0; i < grammar.numTerminals; i++) {
        printf("%s", grammar.terminals[i]);
        if (i < grammar.numTerminals - 1) {
            printf(", ");
        }
    }
    
    printf("\nStart Symbol: %s\n", grammar.startSymbol);
}

// Get the common prefix of two strings
bool hasCommonPrefix(char* rhs1, char* rhs2, char* prefix) {
    int i = 0;
    while (rhs1[i] != '\0' && rhs2[i] != '\0' && rhs1[i] == rhs2[i]) {
        prefix[i] = rhs1[i];
        i++;
    }
    prefix[i] = '\0';
    return i > 0;
}

// Extract a symbol from a string at a given position


char* getSymbol(const char* rhs, int* pos) {
    char* symbol = (char*)malloc(MAX_PROD_LEN);
    int i = 0;
    
    // Skip whitespace
    while (rhs[*pos] != '\0' && isspace(rhs[*pos])) {
        (*pos)++;
    }
    
    if (rhs[*pos] == '\0') {
        //free(symbol);
        return NULL;
    }
    
    // Check if it's a multi-character symbol (non-terminal)
    if (isalpha(rhs[*pos]) && isupper(rhs[*pos])) {
        while (rhs[*pos] != '\0' && (isalnum(rhs[*pos]) || rhs[*pos] == '_' || rhs[*pos] == '\'')) {
            symbol[i++] = rhs[*pos];
            (*pos)++;
        }
    } 
    // Single character symbol (terminal)
    else {
        symbol[i++] = rhs[*pos];
        (*pos)++;
    }
    
    symbol[i] = '\0';
    return symbol;
}






char* getSymbol1(const char* rhs, int* pos) {
    char* symbol = (char*)malloc(MAX_PROD_LEN);
    
    // Skip whitespace
    while (rhs[*pos] != '\0' && isspace(rhs[*pos])) {
        (*pos)++;
    }
    
    if (rhs[*pos] == '\0') {
        free(symbol);
        return NULL;
    }

    // Check if the symbol is epsilon (UTF-8 encoding 0xCE 0xB5)
    if ((unsigned char)rhs[*pos] == 0xCE && (unsigned char)rhs[*pos + 1] == 0xB5) {
        strcpy(symbol, EPSILON);
        (*pos) += 2; // Move past the two-byte UTF-8 character
        return symbol;
    }

    free(symbol);
    return NULL;
}


// Implementation of left factoring
Grammar leftFactoring(Grammar grammar) {
    Grammar result = grammar;
    result.numProductions = 0;
    
    for (int i = 0; i < grammar.numProductions; i++) {
        Production prod = grammar.productions[i];
        
        // Check if we need left factoring for this production
        bool needsFactoring = false;
        for (int j = 0; j < prod.numRHS; j++) {
            for (int k = j + 1; k < prod.numRHS; k++) {
                char prefix[MAX_PROD_LEN] = "";
                if (hasCommonPrefix(prod.rhs[j], prod.rhs[k], prefix) && strlen(prefix) > 0) {
                    needsFactoring = true;
                    break;
                }
            }
            if (needsFactoring) break;
        }
        
        if (!needsFactoring) {
            // No factoring needed, add as is
            result.productions[result.numProductions] = prod;
            result.numProductions++;
        } else {
            // Group RHS alternatives by their common prefixes
            bool processed[MAX_RHS] = {false};
            
            for (int j = 0; j < prod.numRHS; j++) {
                if (processed[j]) continue;
                
                char prefix[MAX_PROD_LEN] = "";
                char newRHS[MAX_RHS][MAX_PROD_LEN];
                int numNewRHS = 0;
                
                // Find all RHS with the same prefix
                for (int k = j; k < prod.numRHS; k++) {
                    if (processed[k]) continue;
                    
                    if (j == k) {
                        // First occurrence, use it as a prefix candidate
                        int pos = 0;
                        char* symbol = getSymbol(prod.rhs[j], &pos);
                        strcpy(prefix, symbol);
                        free(symbol);
                    } else {
                        // Check if this RHS has the same prefix
                        int pos1 = 0, pos2 = 0;
                        char* symbol1 = getSymbol(prod.rhs[j], &pos1);
                        char* symbol2 = getSymbol(prod.rhs[k], &pos2);
                        
                        if (strcmp(symbol1, symbol2) != 0) {
                            free(symbol1);
                            free(symbol2);
                            continue;
                        }
                        free(symbol1);
                        free(symbol2);
                    }
                    
                    // Extract the remainder after the prefix
                    char remainder[MAX_PROD_LEN] = "";
                    int pos = 0;
                    char* firstSymbol = getSymbol(prod.rhs[k], &pos);
                    
                    // Skip first symbol (prefix)
                    if (strlen(prod.rhs[k]) > strlen(firstSymbol)) {
                        strcpy(remainder, prod.rhs[k] + pos);
                    } else if (strlen(prod.rhs[k]) == strlen(firstSymbol)) {
                        strcpy(remainder, EPSILON);
                    }
                    free(firstSymbol);
                    
                    strcpy(newRHS[numNewRHS++], remainder);
                    processed[k] = true;
                }
                
                if (numNewRHS > 0) {
                    // Create a new production with the common prefix
                    char newLHS[MAX_PROD_LEN];
                    sprintf(newLHS, "%s'", prod.lhs);
                    
                    // Make sure the new non-terminal is not already in use
                    int suffix = 1;
                    char tempLHS[MAX_PROD_LEN];
                    strcpy(tempLHS, newLHS);
                    while (isNonTerminal(result, tempLHS)) {
                        sprintf(tempLHS, "%s'%d", prod.lhs, suffix++);
                    }
                    strcpy(newLHS, tempLHS);
                    
                    // Add the new non-terminal to the grammar
                    strcpy(result.nonTerminals[result.numNonTerminals], newLHS);
                    result.numNonTerminals++;
                    
                    // Create the factored production
                    char factoredRHS[MAX_PROD_LEN];
                    sprintf(factoredRHS, "%s %s", prefix, newLHS);
                    
                    // Add the main production
                    strcpy(result.productions[result.numProductions].lhs, prod.lhs);
                    strcpy(result.productions[result.numProductions].rhs[0], factoredRHS);
                    result.productions[result.numProductions].numRHS = 1;
                    result.numProductions++;
                    
                    // Add the new production for the factored part
                    strcpy(result.productions[result.numProductions].lhs, newLHS);
                    for (int k = 0; k < numNewRHS; k++) {
                        strcpy(result.productions[result.numProductions].rhs[k], newRHS[k]);
                    }
                    result.productions[result.numProductions].numRHS = numNewRHS;
                    result.numProductions++;
                }
            }
            
            // Add any unfactored alternatives
            char unfactoredRHS[MAX_RHS][MAX_PROD_LEN];
            int numUnfactored = 0;
            
            for (int j = 0; j < prod.numRHS; j++) {
                if (!processed[j]) {
                    strcpy(unfactoredRHS[numUnfactored++], prod.rhs[j]);
                }
            }
            
            if (numUnfactored > 0) {
                strcpy(result.productions[result.numProductions].lhs, prod.lhs);
                for (int j = 0; j < numUnfactored; j++) {
                    strcpy(result.productions[result.numProductions].rhs[j], unfactoredRHS[j]);
                }
                result.productions[result.numProductions].numRHS = numUnfactored;
                result.numProductions++;
            }
        }
    }
    
    return result;
}

// Check if a production has direct left recursion
bool hasDirectLeftRecursion(Production prod) {
    for (int i = 0; i < prod.numRHS; i++) {
        int pos = 0;
        char* firstSymbol = getSymbol(prod.rhs[i], &pos);
        
        if (firstSymbol != NULL && strcmp(firstSymbol, prod.lhs) == 0) {
            free(firstSymbol);
            return true;
        }
        
        if (firstSymbol != NULL) {
            free(firstSymbol);
        }
    }
    
    return false;
}

// Implementation of left recursion removal
Grammar leftRecursionRemoval(Grammar grammar) {
    Grammar result = grammar;
    result.numProductions = 0;
    
    // For each non-terminal
    for (int i = 0; i < grammar.numNonTerminals; i++) {
        char nonTerminal[20];
        strcpy(nonTerminal, grammar.nonTerminals[i]);
        
        // Find the production for this non-terminal
        Production* prod = NULL;
        for (int j = 0; j < grammar.numProductions; j++) {
            if (strcmp(grammar.productions[j].lhs, nonTerminal) == 0) {
                prod = &grammar.productions[j];
                break;
            }
        }
        
        if (prod == NULL) continue;
        
        // Check if this production has direct left recursion
        if (!hasDirectLeftRecursion(*prod)) {
            // No left recursion, add as is
            result.productions[result.numProductions++] = *prod;
            continue;
        }
        
        // Separate recursive and non-recursive parts
        char recursiveParts[MAX_RHS][MAX_PROD_LEN];
        char nonRecursiveParts[MAX_RHS][MAX_PROD_LEN];
        int numRecursive = 0;
        int numNonRecursive = 0;
        
        for (int j = 0; j < prod->numRHS; j++) {
            int pos = 0;
            char* firstSymbol = getSymbol(prod->rhs[j], &pos);
            
            if (firstSymbol != NULL && strcmp(firstSymbol, prod->lhs) == 0) {
                // This is a recursive part, extract the suffix
                char suffix[MAX_PROD_LEN] = "";
                if (strlen(prod->rhs[j]) > strlen(firstSymbol)) {
                    strcpy(suffix, prod->rhs[j] + pos);
                }
                strcpy(recursiveParts[numRecursive++], suffix);
            } else {
                // This is a non-recursive part
                strcpy(nonRecursiveParts[numNonRecursive++], prod->rhs[j]);
            }
            
            if (firstSymbol != NULL) {
                free(firstSymbol);
            }
        }
        
        // Create a new non-terminal for the recursive part
        char newNonTerminal[20];
        sprintf(newNonTerminal, "%s'", nonTerminal);
        
        // Make sure the new non-terminal is not already in use
        int suffix = 1;
        char tempNT[MAX_PROD_LEN];
        strcpy(tempNT, newNonTerminal);
        while (isNonTerminal(result, tempNT)) {
            sprintf(tempNT, "%s'%d", nonTerminal, suffix++);
        }
        strcpy(newNonTerminal, tempNT);
        
        // Add the new non-terminal to the grammar
        strcpy(result.nonTerminals[result.numNonTerminals], newNonTerminal);
        result.numNonTerminals++;
        
        // Create the non-recursive production
        strcpy(result.productions[result.numProductions].lhs, nonTerminal);
        for (int j = 0; j < numNonRecursive; j++) {
            char newRHS[MAX_PROD_LEN];
            if (strcmp(nonRecursiveParts[j], EPSILON) == 0) {
                strcpy(newRHS, newNonTerminal);
            } else {
                sprintf(newRHS, "%s %s", nonRecursiveParts[j], newNonTerminal);
            }
            strcpy(result.productions[result.numProductions].rhs[j], newRHS);
        }
        result.productions[result.numProductions].numRHS = numNonRecursive;
        result.numProductions++;
        
        // Create the recursive production
        strcpy(result.productions[result.numProductions].lhs, newNonTerminal);
        for (int j = 0; j < numRecursive; j++) {
            char newRHS[MAX_PROD_LEN];
            if (strcmp(recursiveParts[j], "") == 0) {
                sprintf(newRHS, "%s", newNonTerminal);
            } else {
                sprintf(newRHS, "%s %s", recursiveParts[j], newNonTerminal);
            }
            strcpy(result.productions[result.numProductions].rhs[j], newRHS);
        }
        // Add epsilon to the recursive production
        strcpy(result.productions[result.numProductions].rhs[numRecursive], EPSILON);
        result.productions[result.numProductions].numRHS = numRecursive + 1;
        result.numProductions++;
    }
    
    return result;
}

// Check if a symbol is a terminal
bool isTerminal(Grammar grammar, const char* symbol) {
    for (int i = 0; i < grammar.numTerminals; i++) {
        if (strcmp(grammar.terminals[i], symbol) == 0) {
            return true;
        }
    }
    return false;
}

// Check if a symbol is a non-terminal
bool isNonTerminal(Grammar grammar, const char* symbol) {
    for (int i = 0; i < grammar.numNonTerminals; i++) {
        if (strcmp(grammar.nonTerminals[i], symbol) == 0) {
            return true;
        }
    }
    return false;
}

// Add an element to a set if not already present
void addToSet(Set* set, const char* element) {
    for (int i = 0; i < set->numElements; i++) {
        if (strcmp(set->elements[i], element) == 0) {
            return;
        }
    }
    strcpy(set->elements[set->numElements], element);
    set->numElements++;
}

// Check if an element is in a set
bool isInSet(Set set, const char* element) {
    for (int i = 0; i < set.numElements; i++) {
        if (strcmp(set.elements[i], element) == 0) {
            return true;
        }
    }
    return false;
}

// Compute the FIRST sets for all non-terminals
Set* computeFirstSets(Grammar grammar) {
    Set* firstSets = (Set*)malloc(grammar.numNonTerminals * sizeof(Set));
    
    // Initialize FIRST sets
    for (int i = 0; i < grammar.numNonTerminals; i++) {
        strcpy(firstSets[i].symbol, grammar.nonTerminals[i]);
        firstSets[i].numElements = 0;
    }
    
    bool changes = true;
    
    // Continue until no more changes
    while (changes) {
        changes = false;
        
        // For each production
        for (int i = 0; i < grammar.numProductions; i++) {
            Production prod = grammar.productions[i];
            
            // Find the index of this non-terminal in firstSets
            int ntIndex = -1;
            for (int j = 0; j < grammar.numNonTerminals; j++) {
                if (strcmp(firstSets[j].symbol, prod.lhs) == 0) {
                    ntIndex = j;
                    break;
                }
            }
            
            if (ntIndex == -1) continue;
            
            // For each RHS
            for (int j = 0; j < prod.numRHS; j++) {
                int pos = 0;
                char* symbol = getSymbol(prod.rhs[j], &pos);
                
                if (symbol == NULL) continue;
                
                // If it's epsilon
                if (strcmp(symbol, EPSILON) == 0) {
                    // Add epsilon to FIRST(prod.lhs)
                    int prevSize = firstSets[ntIndex].numElements;
                    addToSet(&firstSets[ntIndex], EPSILON);
                    if (prevSize < firstSets[ntIndex].numElements) {
                        changes = true;
                    }
                }
                // If it's a terminal
                else if (isTerminal(grammar, symbol)) {
                    // Add symbol to FIRST(prod.lhs)
                    int prevSize = firstSets[ntIndex].numElements;
                    addToSet(&firstSets[ntIndex], symbol);
                    if (prevSize < firstSets[ntIndex].numElements) {
                        changes = true;
                    }
                }
                // If it's a non-terminal
                else if (isNonTerminal(grammar, symbol)) {
                    // Find FIRST(symbol)
                    int symbolIndex = -1;
                    for (int k = 0; k < grammar.numNonTerminals; k++) {
                        if (strcmp(firstSets[k].symbol, symbol) == 0) {
                            symbolIndex = k;
                            break;
                        }
                    }
                    
                    if (symbolIndex != -1) {
                        // Add all elements of FIRST(symbol) except epsilon to FIRST(prod.lhs)
                        int prevSize = firstSets[ntIndex].numElements;
                        for (int k = 0; k < firstSets[symbolIndex].numElements; k++) {
                            if (strcmp(firstSets[symbolIndex].elements[k], EPSILON) != 0) {
                                addToSet(&firstSets[ntIndex], firstSets[symbolIndex].elements[k]);
                            }
                        }
                        if (prevSize < firstSets[ntIndex].numElements) {
                            changes = true;
                        }
                        
                        // Check if FIRST(symbol) contains epsilon
                        bool hasEpsilon = false;
                        for (int k = 0; k < firstSets[symbolIndex].numElements; k++) {
                            if (strcmp(firstSets[symbolIndex].elements[k], EPSILON) == 0) {
                                hasEpsilon = true;
                                break;
                            }
                        }
                        
                        // If FIRST(symbol) doesn't contain epsilon, we're done
                        if (!hasEpsilon) {
                            free(symbol);
                            break;
                        }
                        
                        // Otherwise, continue to the next symbol
                        free(symbol);
                        symbol = getSymbol(prod.rhs[j], &pos);
                        if (symbol == NULL) {
                            // End of production, add epsilon to FIRST(prod.lhs)
                            int prevSize = firstSets[ntIndex].numElements;
                            addToSet(&firstSets[ntIndex], EPSILON);
                            if (prevSize < firstSets[ntIndex].numElements) {
                                changes = true;
                            }
                            break;
                        }
                    }
                }
                
                free(symbol);
                break; // Only consider the first symbol
            }
        }
    }
    
    return firstSets;
}

// Compute the FOLLOW sets for all non-terminals
Set* computeFollowSets(Grammar grammar, Set* firstSets) {
    Set* followSets = (Set*)malloc(grammar.numNonTerminals * sizeof(Set));
    
    // Initialize FOLLOW sets
    for (int i = 0; i < grammar.numNonTerminals; i++) {
        strcpy(followSets[i].symbol, grammar.nonTerminals[i]);
        followSets[i].numElements = 0;
        
        // Add $ to FOLLOW(S) where S is the start symbol
        if (strcmp(grammar.nonTerminals[i], grammar.startSymbol) == 0) {
            addToSet(&followSets[i], "$");
        }
    }
    
    bool changes = true;
    
    // Continue until no more changes
    while (changes) {
        changes = false;
        
        // For each production
        for (int i = 0; i < grammar.numProductions; i++) {
            Production prod = grammar.productions[i];
            
            // Find the index of this non-terminal in followSets
            int lhsIndex = -1;
            for (int j = 0; j < grammar.numNonTerminals; j++) {
                if (strcmp(followSets[j].symbol, prod.lhs) == 0) {
                    lhsIndex = j;
                    break;
                }
            }
            
            if (lhsIndex == -1) continue;
            
            // For each RHS
            for (int j = 0; j < prod.numRHS; j++) {
                char* rhs = prod.rhs[j];
                int rhsLen = strlen(rhs);
                
                // For each symbol in RHS
                int pos = 0;
                while (pos < rhsLen) {
                    char* symbol = getSymbol(rhs, &pos);
                    
                    if (symbol == NULL) break;
                    
                    // If it's a non-terminal
                    if (isNonTerminal(grammar, symbol)) {
                        // Find the index of this non-terminal in followSets
                        int ntIndex = -1;
                        for (int k = 0; k < grammar.numNonTerminals; k++) {
                            if (strcmp(followSets[k].symbol, symbol) == 0) {
                                ntIndex = k;
                                break;
                            }
                        }
                        
                        if (ntIndex == -1) {
                            free(symbol);
                            continue;
                        }
                        
                        // Get the next symbol
                        int savedPos = pos;
                        char* nextSymbol = getSymbol(rhs, &pos);
                        
                        // If there's no next symbol
                        if (nextSymbol == NULL) {
                            // Add FOLLOW(LHS) to FOLLOW(symbol)
                            int prevSize = followSets[ntIndex].numElements;
                            for (int k = 0; k < followSets[lhsIndex].numElements; k++) {
                                addToSet(&followSets[ntIndex], followSets[lhsIndex].elements[k]);
                            }
                            if (prevSize < followSets[ntIndex].numElements) {
                                changes = true;
                            }
                        } else {
                            // If next symbol is a terminal
                            if (isTerminal(grammar, nextSymbol)) {
                                // Add nextSymbol to FOLLOW(symbol)
                                int prevSize = followSets[ntIndex].numElements;
                                addToSet(&followSets[ntIndex], nextSymbol);
                                if (prevSize < followSets[ntIndex].numElements) {
                                    changes = true;
                                }
                            }
                            // If next symbol is a non-terminal
                            else if (isNonTerminal(grammar, nextSymbol)) {
                                // Find FIRST(nextSymbol)
                                int nextIndex = -1;
                                for (int k = 0; k < grammar.numNonTerminals; k++) {
                                    if (strcmp(firstSets[k].symbol, nextSymbol) == 0) {
                                        nextIndex = k;
                                        break;
                                    }
                                }
                                
                                if (nextIndex != -1) {
                                    // Add FIRST(nextSymbol) - {epsilon} to FOLLOW(symbol)
                                    int prevSize = followSets[ntIndex].numElements;
                                    for (int k = 0; k < firstSets[nextIndex].numElements; k++) {
                                        if (strcmp(firstSets[nextIndex].elements[k], EPSILON) != 0) {
                                            addToSet(&followSets[ntIndex], firstSets[nextIndex].elements[k]);
                                        }
                                    }
                                    if (prevSize < followSets[ntIndex].numElements) {
                                        changes = true;
                                    }
                                    
                                    // If FIRST(nextSymbol) contains epsilon
                                    bool hasEpsilon = false;
                                    for (int k = 0; k < firstSets[nextIndex].numElements; k++) {
                                        if (strcmp(firstSets[nextIndex].elements[k], EPSILON) == 0) {
                                            hasEpsilon = true;
                                            break;
                                        }
                                    }
                                    
                                    if (hasEpsilon) {
                                        // Add FOLLOW(LHS) to FOLLOW(symbol)
                                        int prevSize = followSets[ntIndex].numElements;
                                        for (int k = 0; k < followSets[lhsIndex].numElements; k++) {
                                            addToSet(&followSets[ntIndex], followSets[lhsIndex].elements[k]);
                                        }
                                        if (prevSize < followSets[ntIndex].numElements) {
                                            changes = true;
                                        }
                                    }
                                }
                            }
                            
                            free(nextSymbol);
                            pos = savedPos; // Restore position for the outer loop
                        }
                    }
                    
                    free(symbol);
                }
            }
        }
    }
    
    return followSets;
}

// Construct the LL(1) parsing table
/*
ParseTable constructLL1Table(Grammar grammar, Set* firstSets, Set* followSets) {
    ParseTable table;
    table.numEntries = 0;
    
    // Copy terminals and non-terminals
    table.numTerminals = grammar.numTerminals;
    for (int i = 0; i < grammar.numTerminals; i++) {
        strcpy(table.terminals[i], grammar.terminals[i]);
    }
    
    // Add $ as a terminal
    strcpy(table.terminals[table.numTerminals], "$");
    table.numTerminals++;
    
    table.numNonTerminals = grammar.numNonTerminals;
    for (int i = 0; i < grammar.numNonTerminals; i++) {
        strcpy(table.nonTerminals[i], grammar.nonTerminals[i]);
    }
    
    // For each production
    for (int i = 0; i < grammar.numProductions; i++) {
        Production prod = grammar.productions[i];
        
        // Find the index of this non-terminal in firstSets
        int ntIndex = -1;
        for (int j = 0; j < grammar.numNonTerminals; j++) {
            if (strcmp(firstSets[j].symbol, prod.lhs) == 0) {
                ntIndex = j;
                break;
            }
        }
        
        if (ntIndex == -1) continue;
        
        // For each RHS
        for (int j = 0; j < prod.numRHS; j++) {
            char* rhs = prod.rhs[j];
            
            // Get the first symbol of RHS
            int pos = 0;
            char* firstSymbol = getSymbol(rhs, &pos);
            
            // If RHS is epsilon or starts with a terminal
            if (firstSymbol == NULL || strcmp(firstSymbol, EPSILON) == 0 || isTerminal(grammar, firstSymbol)) {
                if (firstSymbol == NULL || strcmp(firstSymbol, EPSILON) == 0) {
                    // For each terminal in FOLLOW(LHS)
                    for (int k = 0; k < followSets[ntIndex].numElements; k++) {
                        char* terminal = followSets[ntIndex].elements[k];
                        
                        // Add entry to the parsing table
                        strcpy(table.entries[table.numEntries].nonTerminal, prod.lhs);
                        strcpy(table.entries[table.numEntries].terminal, terminal);
                        
                        if (strcmp(rhs, EPSILON) == 0) {
                            strcpy(table.entries[table.numEntries].production, EPSILON);
                        } else {
                            strcpy(table.entries[table.numEntries].production, rhs);
                        }
                        
                        table.numEntries++;
                    }
                } else {
                    // Add entry to the parsing table
                    strcpy(table.entries[table.numEntries].nonTerminal, prod.lhs);
                    strcpy(table.entries[table.numEntries].terminal, firstSymbol);
                    strcpy(table.entries[table.numEntries].production, rhs);
                    table.numEntries++;
                }
            }
            // If RHS starts with a non-terminal
            else if (isNonTerminal(grammar, firstSymbol)) {
                // Find FIRST(firstSymbol)
                int symbolIndex = -1;
                for (int k = 0; k < grammar.numNonTerminals; k++) {
                    if (strcmp(firstSets[k].symbol, firstSymbol) == 0) {
                        symbolIndex = k;
                        break;
                    }
                }
                
                if (symbolIndex != -1) {
                    // For each terminal in FIRST(firstSymbol)
                    for (int k = 0; k < firstSets[symbolIndex].numElements; k++) {
                        char* terminal = firstSets[symbolIndex].elements[k];
                        
                        // Skip epsilon
                        if (strcmp(terminal, EPSILON) == 0) continue;
                        
                        // Add entry to the parsing table
                        strcpy(table.entries[table.numEntries].nonTerminal, prod.lhs);
                        strcpy(table.entries[table.numEntries].terminal, terminal);
                        strcpy(table.entries[table.numEntries].production, rhs);
                        table.numEntries++;
                    }
                    
                    // Check if FIRST(firstSymbol) contains epsilon
                    bool hasEpsilon = false;
                    for (int k = 0; k < firstSets[symbolIndex].numElements; k++) {
                        if (strcmp(firstSets[symbolIndex].elements[k], EPSILON) == 0) {
                            hasEpsilon = true;
                            break;
                        }
                    }
                    
                    if (hasEpsilon) {
                        // For each terminal in FOLLOW(LHS)
                        for (int k = 0; k < followSets[ntIndex].numElements; k++) {
                            char* terminal = followSets[ntIndex].elements[k];
                            
                            // Add entry to the parsing table
                            strcpy(table.entries[table.numEntries].nonTerminal, prod.lhs);
                            strcpy(table.entries[table.numEntries].terminal, terminal);
                            strcpy(table.entries[table.numEntries].production, rhs);
                            table.numEntries++;
                        }
                    }
                }
            }
            
            if (firstSymbol != NULL) {
                free(firstSymbol);
            }
        }
    }
    
    return table;
}
*/

// Function to correctly read epsilon as a UTF-8 character


ParseTable constructLL1Table(Grammar grammar, Set* firstSets, Set* followSets) {
    ParseTable table;
    table.numEntries = 0;
    
    // Copy terminals and non-terminals
    table.numTerminals = grammar.numTerminals;
    printf("Initializing parse table with %d terminals\n", grammar.numTerminals);
    
    for (int i = 0; i < grammar.numTerminals; i++) {
        strcpy(table.terminals[i], grammar.terminals[i]);
        printf("Terminal[%d]: %s\n", i, table.terminals[i]);
    }
    
    // Add $ as a terminal
    strcpy(table.terminals[table.numTerminals], "$");
    printf("Added terminal: $\n");
    table.numTerminals++;
    
    table.numNonTerminals = grammar.numNonTerminals;
    printf("Initializing parse table with %d non-terminals\n", grammar.numNonTerminals);
    
    for (int i = 0; i < grammar.numNonTerminals; i++) {
        strcpy(table.nonTerminals[i], grammar.nonTerminals[i]);
        printf("Non-terminal[%d]: %s\n", i, table.nonTerminals[i]);
    }
    
    // For each production
    for (int i = 0; i < grammar.numProductions; i++) {
        Production prod = grammar.productions[i];
        printf("\nProcessing production: %s -> ...\n", prod.lhs);
        
        // Find the index of this non-terminal in firstSets
        int ntIndex = -1;
        for (int j = 0; j < grammar.numNonTerminals; j++) {
            if (strcmp(firstSets[j].symbol, prod.lhs) == 0) {
                ntIndex = j;
                break;
            }
        }
        
        if (ntIndex == -1) {
            printf("Error: Non-terminal %s not found in firstSets!\n", prod.lhs);
            continue;
        }
        
        // For each RHS
        for (int j = 0; j < prod.numRHS; j++) {
            char* rhs = prod.rhs[j];
            printf("  Processing RHS: %s\n", rhs);
            
            // Get the first symbol of RHS
              int pos = 0;
             char* firstSymbol = getSymbol(rhs, &pos);
            
            if (strcmp(rhs, EPSILON) == 0) {
             printf("RHS is epsilon...\n");
              //pos += strlen(EPSILON); // Skip epsilon
              firstSymbol = getSymbol1(rhs, &pos);
              }           
              else {
                firstSymbol = getSymbol(rhs, &pos);
                  } 
            
            //int pos = 0;
            //char* firstSymbol = getSymbol(rhs, &pos);
            printf("Extracted first symbol: '%s' from RHS: '%s'\n", firstSymbol ? firstSymbol : "NULL", rhs);

            
            
            if (firstSymbol == NULL || strcmp(firstSymbol, EPSILON) == 0 || isTerminal(grammar, firstSymbol)) {
                if (firstSymbol == NULL || strcmp(firstSymbol, EPSILON) == 0) {
                    printf("  RHS is epsilon. Adding entries from FOLLOW(%s)\n", prod.lhs);
                    
                    // For each terminal in FOLLOW(LHS)
                    for (int k = 0; k < followSets[ntIndex].numElements; k++) {
                        char* terminal = followSets[ntIndex].elements[k];
                        printf("    Adding table entry: [%s, %s] -> %s\n", prod.lhs, terminal, EPSILON);
                        
                        strcpy(table.entries[table.numEntries].nonTerminal, prod.lhs);
                        strcpy(table.entries[table.numEntries].terminal, terminal);
                        strcpy(table.entries[table.numEntries].production, EPSILON);
                        table.numEntries++;
                    }
                } else {
                    printf("  RHS starts with terminal %s. Adding table entry.\n", firstSymbol);
                    
                    // Add entry to the parsing table
                    strcpy(table.entries[table.numEntries].nonTerminal, prod.lhs);
                    strcpy(table.entries[table.numEntries].terminal, firstSymbol);
                    strcpy(table.entries[table.numEntries].production, rhs);
                    printf("    Added table entry: [%s, %s] -> %s\n", prod.lhs, firstSymbol, rhs);
                    table.numEntries++;
                }
            } else if (isNonTerminal(grammar, firstSymbol)) {
                printf("  RHS starts with non-terminal %s. Finding FIRST(%s)\n", firstSymbol, firstSymbol);
                
                // Find FIRST(firstSymbol)
                int symbolIndex = -1;
                for (int k = 0; k < grammar.numNonTerminals; k++) {
                    if (strcmp(firstSets[k].symbol, firstSymbol) == 0) {
                        symbolIndex = k;
                        break;
                    }
                }
                
                if (symbolIndex != -1) {
                    printf("  FIRST(%s) found. Adding entries.\n", firstSymbol);
                    
                    // For each terminal in FIRST(firstSymbol)
                    for (int k = 0; k < firstSets[symbolIndex].numElements; k++) {
                        char* terminal = firstSets[symbolIndex].elements[k];
                        if (strcmp(terminal, EPSILON) == 0) continue;
                        
                        printf("    Adding table entry: [%s, %s] -> %s\n", prod.lhs, terminal, rhs);
                        strcpy(table.entries[table.numEntries].nonTerminal, prod.lhs);
                        strcpy(table.entries[table.numEntries].terminal, terminal);
                        strcpy(table.entries[table.numEntries].production, rhs);
                        table.numEntries++;
                    }
                    
                    // Check if FIRST(firstSymbol) contains epsilon
                    bool hasEpsilon = false;
                    for (int k = 0; k < firstSets[symbolIndex].numElements; k++) {
                        if (strcmp(firstSets[symbolIndex].elements[k], EPSILON) == 0) {
                            hasEpsilon = true;
                            break;
                        }
                    }
                    
                    if (hasEpsilon) {
                        printf("  FIRST(%s) contains epsilon. Adding FOLLOW(%s) entries.\n", firstSymbol, prod.lhs);
                        
                        // For each terminal in FOLLOW(LHS)
                        for (int k = 0; k < followSets[ntIndex].numElements; k++) {
                            char* terminal = followSets[ntIndex].elements[k];
                            printf("    Adding table entry: [%s, %s] -> %s\n", prod.lhs, terminal, rhs);
                            
                            strcpy(table.entries[table.numEntries].nonTerminal, prod.lhs);
                            strcpy(table.entries[table.numEntries].terminal, terminal);
                            strcpy(table.entries[table.numEntries].production, rhs);
                            table.numEntries++;
                        }
                    }
                }
            }
            
            if (firstSymbol != NULL) {
                free(firstSymbol);
            }
        }
    }
    
    printf("\nParse table construction complete. Total entries: %d\n", table.numEntries);
    return table;
}

// Display the FIRST sets
void displayFirstSets(Set* firstSets, int numNonTerminals) {
    for (int i = 0; i < numNonTerminals; i++) {
        printf("FIRST(%s) = { ", firstSets[i].symbol);
        for (int j = 0; j < firstSets[i].numElements; j++) {
            printf("%s", firstSets[i].elements[j]);
            if (j < firstSets[i].numElements - 1) {
                printf(", ");
            }
        }
        printf(" }\n");
    }
}

// Display the FOLLOW sets
void displayFollowSets(Set* followSets, int numNonTerminals) {
    for (int i = 0; i < numNonTerminals; i++) {
        printf("FOLLOW(%s) = { ", followSets[i].symbol);
        for (int j = 0; j < followSets[i].numElements; j++) {
            printf("%s", followSets[i].elements[j]);
            if (j < followSets[i].numElements - 1) {
                printf(", ");
            }
        }
        printf(" }\n");
    }
}

// Display the parsing table
void displayParseTable(ParseTable table) {
    printf("%-10s | ", "");
    for (int i = 0; i < table.numTerminals; i++) {
        printf("%-10s | ", table.terminals[i]);
    }
    printf("\n");
    
    for (int i = 0; i < (table.numTerminals + 1) * 13; i++) {
        printf("-");
    }
    printf("\n");
    
    for (int i = 0; i < table.numNonTerminals; i++) {
        printf("%-10s | ", table.nonTerminals[i]);
        
        for (int j = 0; j < table.numTerminals; j++) {
            bool found = false;
            
            for (int k = 0; k < table.numEntries; k++) {
                if (strcmp(table.entries[k].nonTerminal, table.nonTerminals[i]) == 0 &&
                    strcmp(table.entries[k].terminal, table.terminals[j]) == 0) {
                    printf("%-10s | ", table.entries[k].production);
                    found = true;
                    break;
                }
            }
            
            if (!found) {
                printf("%-10s | ", "");
            }
        }
        
        printf("\n");
    }
}

// Split a string by a delimiter
char** splitString(const char* str, const char* delimiter, int* count) {
    char* copy = strdup(str);
    *count = 0;
    
    // Count the number of tokens
    char* tmp = copy;
    char* token = strtok(tmp, delimiter);
    while (token != NULL) {
        (*count)++;
        token = strtok(NULL, delimiter);
    }
    
    // Allocate memory for the result
    char** result = (char**)malloc((*count) * sizeof(char*));
    
    // Split the string
    free(copy);
    copy = strdup(str);
    tmp = copy;
    token = strtok(tmp, delimiter);
    int i = 0;
    while (token != NULL) {
        result[i] = strdup(token);
        i++;
        token = strtok(NULL, delimiter);
    }
    
    free(copy);
    return result;
}

// Trim whitespace from a string
char* trimString(char* str) {
    char* result = strdup(str);
    
    // Trim leading whitespace
    while (*result && isspace(*result)) {
        result++;
    }
    
    // Trim trailing whitespace
    char* end = result + strlen(result) - 1;
    while (end > result && isspace(*end)) {
        *end = '\0';
        end--;
    }
    
    return result;
}

// Free memory allocated for sets
void freeSet(Set* set, int count) {
    free(set);
}

// Write output to a file
void writeOutputToFile(Grammar original, Grammar leftFactored, Grammar withoutLeftRecursion, 
    Set* firstSets, Set* followSets, ParseTable parseTable, const char* filename)
{
    FILE* file = fopen(filename, "w");
    
    if (file == NULL) {
        printf("Error opening file: %s\n", filename);
        return;
    }

    printf("Debug: Writing to %s\n", filename); // Debug message
    
    // Write original grammar
    fprintf(file, "Original Grammar:\n");
    for (int i = 0; i < original.numProductions; i++) {
        Production prod = original.productions[i];
        fprintf(file, "%s -> ", prod.lhs);
        for (int j = 0; j < prod.numRHS; j++) {
            fprintf(file, "%s", prod.rhs[j]);
            if (j < prod.numRHS - 1) {
                fprintf(file, " | ");
            }
        }
        fprintf(file, "\n");
    }

    // Write left factored grammar
    fprintf(file, "\nGrammar after Left Factoring:\n");
    for (int i = 0; i < leftFactored.numProductions; i++) {
        Production prod = leftFactored.productions[i];
        fprintf(file, "%s -> ", prod.lhs);
        for (int j = 0; j < prod.numRHS; j++) {
            fprintf(file, "%s", prod.rhs[j]);
            if (j < prod.numRHS - 1) {
                fprintf(file, " | ");
            }
        }
        fprintf(file, "\n");
    }
    
    // Write grammar without left recursion
    fprintf(file, "\nGrammar after Left Recursion Removal:\n");
    for (int i = 0; i < withoutLeftRecursion.numProductions; i++) {
        Production prod = withoutLeftRecursion.productions[i];
        fprintf(file, "%s -> ", prod.lhs);
        for (int j = 0; j < prod.numRHS; j++) {
            fprintf(file, "%s", prod.rhs[j]);
            if (j < prod.numRHS - 1) {
                fprintf(file, " | ");
            }
        }
        fprintf(file, "\n");
    }
    
    // Write FIRST sets
    fprintf(file, "\nFIRST Sets:\n");
    for (int i = 0; i < withoutLeftRecursion.numNonTerminals; i++) {
        fprintf(file, "FIRST(%s) = { ", firstSets[i].symbol);
        for (int j = 0; j < firstSets[i].numElements; j++) {
            fprintf(file, "%s", firstSets[i].elements[j]);
            if (j < firstSets[i].numElements - 1) {
                fprintf(file, ", ");
            }
        }
        fprintf(file, " }\n");
    }
    
    // Write FOLLOW sets
    fprintf(file, "\nFOLLOW Sets:\n");
    for (int i = 0; i < withoutLeftRecursion.numNonTerminals; i++) {
        fprintf(file, "FOLLOW(%s) = { ", followSets[i].symbol);
        for (int j = 0; j < followSets[i].numElements; j++) {
            fprintf(file, "%s", followSets[i].elements[j]);
            if (j < followSets[i].numElements - 1) {
                fprintf(file, ", ");
            }
        }
        fprintf(file, " }\n");
    }
    
    // Write LL(1) parsing table
    fprintf(file, "\nLL(1) Parsing Table:\n");
    
    fprintf(file, "%-10s | ", "");
    for (int i = 0; i < parseTable.numTerminals; i++) {
        fprintf(file, "%-10s | ", parseTable.terminals[i]);
    }
    fprintf(file, "\n");
    
    for (int i = 0; i < (parseTable.numTerminals + 1) * 13; i++) {
        fprintf(file, "-");
    }
    fprintf(file, "\n");
    
    for (int i = 0; i < parseTable.numNonTerminals; i++) {
        fprintf(file, "%-10s | ", parseTable.nonTerminals[i]);
        
        for (int j = 0; j < parseTable.numTerminals; j++) {
            bool found = false;
            
            for (int k = 0; k < parseTable.numEntries; k++) {
                if (strcmp(parseTable.entries[k].nonTerminal, parseTable.nonTerminals[i]) == 0 &&
                    strcmp(parseTable.entries[k].terminal, parseTable.terminals[j]) == 0) {
                    fprintf(file, "%-10s | ", parseTable.entries[k].production);
                    found = true;
                    break;
                }
            }
            
            if (!found) {
                fprintf(file, "%-10s | ", "");
            }
        }
        
        fprintf(file, "\n");
    }
    
    fclose(file);
}
