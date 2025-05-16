#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <ctype.h>
#include <string.h>
#include "eval.h"
#include "stack.h"

bool isOperator(char c) {
    return (c == '+' || c == '-' || c == '*' || c == '/' || c == '%' || c == '(' || c == ')');
}

// Returns precedence level: higher value means higher precedence
int precedence(char c) {
    if (c == '+' || c == '-') return 1;
    if (c == '*' || c == '/' || c == '%') return 2;
    return 0;
}

int operation(char o, int n1, int n2) {
    switch (o) {
        case '+': return n1 + n2;
        case '-': return n1 - n2;
        case '*': return n1 * n2;
        case '/': 
        if (n2 ==0) {printf("Divide by zero error"); exit(EXIT_FAILURE); }
        return n1 / n2;
        case '%':
        if (n2 ==0) {printf("Divide by zero error"); exit(EXIT_FAILURE); }
        return n1 % n2;
        default: printf("Unknown operator %c\n", o); exit(EXIT_FAILURE);
    }
}


void removeSpaces(char buffer[]){
    int i=0, j=0;

    while(buffer[i]!= '\0'){
        if (!isspace(buffer[i])){
            buffer[j++] =  buffer[i];
        }
        i++;
    }
    buffer[j] = '\0';
    
}


bool ambiguous_spacing(char buffer[]) {
    int i;
    bool space = false;

    for (i =0 ; buffer[i]!= '\n' && buffer[i] != '\0' ;i++){
        while(isspace(buffer[i])){
            i++;
        }
        // finding numbers like 34  2
        if (isdigit(buffer[i])){
            while(isdigit(buffer[i])){
                i++;
            }
        }
        if (isspace(buffer[i])){
            while (isspace(buffer[i]))
            {
                i++;
                space = true;
            }            
        }

        if(space && isdigit(buffer[i])){
            return true;
        }
        space = false;
    }
    return space;
}


bool validate_infix(char buffer[]) {
    char lastChar = ' ';
    int openParen = 0;
    int i = 0;

    for (; buffer[i] != '\n' && buffer[i] != '\0'; i++) {

        char current = buffer[i];

        if (current == '(') {
            if (lastChar != ')' && !isdigit(lastChar)) {
                openParen++;
                lastChar = '(';
                continue;
            }
            return false;
        }

        if (current == ')') {
            if (isdigit(lastChar) || lastChar == ')') {
                if (openParen <= 0) return false;
                openParen--;
                lastChar = ')';
                continue;
            }
            return false;
        }

        if (current == '-') {
            // (-4), -3, (24 + 5) - 2,  * - 5
            if ((lastChar == '(' || isOperator(lastChar) || lastChar == ' ') && isdigit(buffer[i+1])) {
                lastChar = '-';
                continue;
            } // ( 25 + 11 ) - , 43 - 
            else if (lastChar == ')' || isdigit(lastChar)) {
                lastChar = '-';
                continue;
            } // - (30 ), ( - (4 + 5) + 1) 
            else if ((lastChar == ' ' || isOperator(lastChar)) && buffer[i+1] == '(' ){
                lastChar = '-';
                continue;
            }
            return false;
        }

        if (isOperator(current)) {
            if (isdigit(lastChar) || lastChar == ')') {
                lastChar = current;
                continue;
            }
            return false;
        }

        if (isdigit(current)) {
            lastChar = current;
            continue;
        }

        return false; // invalid character
    }

    return (openParen == 0) && (isdigit(lastChar) || lastChar == ')');
}


int solve(char buffer[]) {
    Stack *operand = createStack(sizeof(int), 100);
    Stack *operator = createStack(sizeof(char), 100);
    int i = 0, res = 0;
    char lastChar = ' ', negate= 'N';

    while (buffer[i] != '\0' && buffer[i] != '\n') {
        
        if (buffer[i] == '(') {
            push(operator, &buffer[i]);
            lastChar = '(';
            i++;
            continue;
            
        }

        if (buffer[i] == ')') {
            while (!isEmpty(operator) && *(char *)peek(operator) != '(' ){   //&& *(char *)peek(operator) != 'N') {
                int n2 = *(int *)pop(operand);
                int n1 = *(int *)pop(operand);
                char op = *(char *)pop(operator);
                res = operation(op, n1, n2);
                push(operand, &res);
            }
            // Pop the '('
            *(char *)pop(operator);

            // If top is 'N', apply unary minus
            if (!isEmpty(operator) && *(char *)peek(operator) == 'N') {
                pop(operator); // remove 'N'
                int o = *(int *)pop(operand);
                res = -o;
                push(operand, &res);
            }
            lastChar = ')';
            i++;
            continue;
        }

        // Unary minus (e.g., -5 or (-10))
        if (buffer[i] == '-' &&   isdigit(buffer[i+1])  &&  (lastChar == ' ' ||  strchr("(+-/%*",  lastChar)  )) {
            i++;
            int num = 0;
            while (isdigit(buffer[i])) {
                num = num * 10 + (buffer[i] - '0');
                i++;
            }
            num = -num;
            push(operand, &num);
            lastChar = '0';
            continue;
        }

        // Negating results -( 45 + 10) . Patterns like -(
        if (buffer[i] == '-' &&  buffer[i+1] == '('  &&  (lastChar == ' ' ||  strchr("(+-/%*",  lastChar)  )    ){
            push(operator, &negate);
            i++;
            lastChar = 'N';
            continue;
        }

        // Binary operator
        if (isOperator(buffer[i])) {
            char currOp = buffer[i];
            while (!isEmpty(operator)
                   && *(char *)peek(operator) != '('
                   && precedence(currOp) <= precedence(*(char *)peek(operator))) {
                int n2 = *(int *)pop(operand);
                int n1 = *(int *)pop(operand);
                char op = *(char *)pop(operator);
                res = operation(op, n1, n2);
                push(operand, &res);
            }
            push(operator, &currOp);
            lastChar = currOp;
            i++;
            continue;
        }

        // Positive number
        if (isdigit(buffer[i])) {
            int num = 0;
            while (isdigit(buffer[i])) {
                num = num * 10 + (buffer[i] - '0');
                i++;
            }
            push(operand, &num);
            lastChar = '0';
            continue;
        }

        // Skip unknown characters
        i++;
    }

    while (!isEmpty(operator)) {
        char op = *(char *)peek(operator);
    
        if (op == '(') {
            pop(operator);  // discard unmatched (
            continue;
        }
    
        if (op == 'N') {
            pop(operator);
            int o = *(int *)pop(operand);
            res = -o;
            push(operand, &res);
            continue;
        }
    
        // otherwise, it's a real binary operator
        op = *(char *)pop(operator);
        int n2 = *(int *)pop(operand);
        int n1 = *(int *)pop(operand);
        res = operation(op, n1, n2);
        push(operand, &res);
    }
    
    

    if (!isEmpty(operand)) res = *(int *)pop(operand);

    freeStack(operand);
    freeStack(operator);
    return res;
}
