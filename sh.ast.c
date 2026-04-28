/*
 * sh.ast.c: JSON AST dump for --ast-dump mode
 */
#include "sh.h"
#include <stdio.h>

/* Write directly to SHOUT (tcsh's private dup of stdout) */
#define AST_PRINTF(...) dprintf(SHOUT, __VA_ARGS__)

static void
ast_char(char c)
{
    write(SHOUT, &c, 1);
}

static void
ast_json_string(const Char *s)
{
    const char *p;

    if (s == NULL) {
        AST_PRINTF("null");
        return;
    }
    p = short2str(s);
    ast_char('"');
    for (; *p; p++) {
        unsigned char c = (unsigned char)*p;
        switch (c) {
        case '"':  AST_PRINTF("\\\""); break;
        case '\\': AST_PRINTF("\\\\"); break;
        case '\n': AST_PRINTF("\\n");  break;
        case '\r': AST_PRINTF("\\r");  break;
        case '\t': AST_PRINTF("\\t");  break;
        default:
            if (c < 0x20)
                AST_PRINTF("\\u%04x", c);
            else
                ast_char(c);
            break;
        }
    }
    ast_char('"');
}

static void
ast_node(const struct command *t, int depth)
{
    Char **v;
    int first;

    if (t == NULL) {
        AST_PRINTF("null");
        return;
    }

    ast_char('{');

    switch (t->t_dtyp) {

    case NODE_COMMAND:
        AST_PRINTF("\"type\":\"command\",\"argv\":[");
        first = 1;
        if (t->t_dcom) {
            for (v = t->t_dcom; *v; v++) {
                if (!first) ast_char(',');
                ast_json_string(*v);
                first = 0;
            }
        }
        ast_char(']');
        if (t->t_dlef) {
            AST_PRINTF(",\"stdin\":");
            ast_json_string(t->t_dlef);
            if (t->t_dflg & F_READ)
                AST_PRINTF(",\"stdin_heredoc\":true");
        }
        if (t->t_drit) {
            AST_PRINTF(",\"stdout\":");
            ast_json_string(t->t_drit);
            if (t->t_dflg & F_APPEND)
                AST_PRINTF(",\"stdout_append\":true");
            if (t->t_dflg & F_OVERWRITE)
                AST_PRINTF(",\"stdout_clobber\":true");
            if (t->t_dflg & F_STDERR)
                AST_PRINTF(",\"stdout_stderr\":true");
        }
        if (t->t_dflg & F_AMPERSAND)
            AST_PRINTF(",\"background\":true");
        break;

    case NODE_PAREN:
        AST_PRINTF("\"type\":\"subshell\",\"body\":");
        ast_node(t->t_dspr, depth + 1);
        if (t->t_dlef) {
            AST_PRINTF(",\"stdin\":");
            ast_json_string(t->t_dlef);
        }
        if (t->t_drit) {
            AST_PRINTF(",\"stdout\":");
            ast_json_string(t->t_drit);
        }
        if (t->t_dflg & F_AMPERSAND)
            AST_PRINTF(",\"background\":true");
        break;

    case NODE_PIPE:
        AST_PRINTF("\"type\":\"pipe\",\"left\":");
        ast_node(t->t_dcar, depth + 1);
        AST_PRINTF(",\"right\":");
        ast_node(t->t_dcdr, depth + 1);
        if (t->t_dflg & F_STDERR)
            AST_PRINTF(",\"pipe_stderr\":true");
        break;

    case NODE_LIST:
        AST_PRINTF("\"type\":\"list\",\"left\":");
        ast_node(t->t_dcar, depth + 1);
        AST_PRINTF(",\"right\":");
        ast_node(t->t_dcdr, depth + 1);
        break;

    case NODE_AND:
        AST_PRINTF("\"type\":\"and\",\"left\":");
        ast_node(t->t_dcar, depth + 1);
        AST_PRINTF(",\"right\":");
        ast_node(t->t_dcdr, depth + 1);
        break;

    case NODE_OR:
        AST_PRINTF("\"type\":\"or\",\"left\":");
        ast_node(t->t_dcar, depth + 1);
        AST_PRINTF(",\"right\":");
        ast_node(t->t_dcdr, depth + 1);
        break;

    default:
        AST_PRINTF("\"type\":\"unknown\",\"dtyp\":%d", (int)t->t_dtyp);
        break;
    }

    ast_char('}');
}

void
dump_ast_json(const struct command *t)
{
    ast_node(t, 0);
}
