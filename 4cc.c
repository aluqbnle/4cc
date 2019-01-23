#include <assert.h>
#include <ctype.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>


// Tokenizer

enum {
	TK_NUM = 256, // Number literal
	TK_EOF,       // End marker
};

// Token type
typedef struct {
	int ty;      // Token type
	int val;     // Number literal
	char *input; // Token string (for error reporting)
} Token;

// Tokenized input is stored to this array.
Token tokens[100];

//pが指している文字列をトークンに分割してtokensに保存する
void tokenize(char *p) {
	int i = 0;
	while (*p) {
		// Skip whitespace
		if (isspace(*p)) {
			p++;
			continue;
		}

		// + or - or * or /
		if (*p == '+' || *p == '-' ||*p=='*'||*p=='/'||*p=='('||*p==')') {
			tokens[i].ty = *p;
			tokens[i].input = p;
			i++;
			p++;
			continue;
		}

		// Number
		if (isdigit(*p)) {
			tokens[i].ty = TK_NUM;
			tokens[i].input = p;
			tokens[i].val = strtol(p, &p, 10);
			i++;
			continue;
		}

		fprintf(stderr, "cannot tokenize: %s", p);
		exit(1);
	}

	tokens[i].ty = TK_EOF;
}

// Recursive-descendent parser

int pos = 0;

enum {
	ND_NUM = 256,     // Number literal
};

typedef struct Node {
	int ty;           // Node type
	struct Node *lhs; // left-hand side
	struct Node *rhs; // right-hand side
	int val;          // Number literal
} Node;

Node *mul();
Node *term();
void error(char *fmt, ...);

Node *new_node(int op, Node *lhs, Node *rhs)
{
	Node *node = malloc(sizeof(Node));
	node->ty = op;
	node->lhs = lhs;
	node->rhs = rhs;
	return node;
}

Node *new_node_num(int val) {
	Node *node = malloc(sizeof(Node));
	node->ty = ND_NUM;
	node->val = val;
	return node;
}

int consume(int ty){
	if(tokens[pos].ty != ty)
		return 0;
	pos++;
	return 1;
}

Node *add(){
	Node *node=mul();
	for(;;){
		if(consume('+'))
			node = new_node('+',node,mul());
		else if(consume('-'))
			node=new_node('-',node,mul());
		else
		return node;
	}
}

Node *mul(){
	Node *node =term();
	for(;;){
		if(consume('*'))
			node = new_node('*',node,term());
		else if(consume('/'))
			node =new_node('/',node,term());
		else
			return node;
	}
}
Node *term(){
	if(consume('(')){
		Node *node =add();
		if(!consume(')'))
			error("開きカッコに対応する閉じカッコがありません: %s",tokens[pos].input);
		return node;
	}
	if(tokens[pos].ty==TK_NUM)
		return new_node_num(tokens[pos++].val);

	error("数値でも開きカッコでもないトークンです： %s",tokens[pos].input);
}


// An error reporting function.
void error(char *fmt, ...) {
	va_list ap;
	va_start(ap, fmt);
	vfprintf(stderr, fmt, ap);
	fprintf(stderr, "\n");
	exit(1);
}

Node *number() {
	if (tokens[pos].ty == TK_NUM)
		return new_node_num(tokens[pos++].val);
	error("number expected, but got %s", tokens[pos].input);
}

Node *expr() {
	Node *lhs = number();
	for (;;) {
		int op = tokens[pos].ty;
		if (op != '+' && op != '-')
			break;
		pos++;
		lhs = new_node(op, lhs, number());
	}

	if (tokens[pos].ty != TK_EOF)
		error("stray token: %s", tokens[pos].input);
	return lhs;
}

// Code generator

char *regs[] = {"rdi", "rsi", "r10", "r11", "r12", "r13", "r14", "r15", NULL};
int cur;

void gen(Node *node) {
	if (node->ty == ND_NUM) {
		printf("	push %d\n",node->val);
		return;
	}
	gen(node->lhs);
	gen(node->rhs);

	printf("	pop rdi\n");
	printf("	pop rax\n");

	switch (node->ty) {
	case '+':
		printf("  add rax, rdi\n");
		break;
	case '-':
		printf("  sub rax, rdi\n");
		break;
	case '*':
		printf("	mul rdi\n");
		break;
	case '/':
		printf("	mov rdx, 0\n");
		printf("	div rdi\n");
	}
	printf("	push rax\n");
}

int main(int argc, char **argv) {
	if (argc != 2) {
		fprintf(stderr, "引数の個数が正しくありません　\n");
		return 1;
	}

	// Tokenize and parse.
	tokenize(argv[1]);
	Node* node = add();

	// Print the prologue.
	printf(".intel_syntax noprefix\n");
	printf(".global main\n");
	printf("main:\n");

	//抽象僕を下りながらコード生成
	gen(node);

	// Generate code while descending the parse tree.
	printf("  pop rax\n");
	printf("  ret\n");
	return 0;
}