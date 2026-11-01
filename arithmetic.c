/* Copyright (c) 2026 Dennis Wölfing
 *
 * Permission to use, copy, modify, and/or distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
 * ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 * ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
 * OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 */

/* arithmetic.c
 * Shell arithmetic.
 */

#include <config.h>
#include <ctype.h>
#include <err.h>
#include <errno.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "arithmetic.h"
#include "variables.h"

#if HAVE_STDCKDINT_H
#  include <stdckdint.h>
#else
static inline bool ckd_add(ShellInt* result, ShellInt a, ShellInt b) {
    if ((b > 0 && a > MAX_SHELL_INT - b) || (b < 0 && a < MIN_SHELL_INT - b)) {
        return true;
    }

    *result = a + b;
    return false;
}

static inline bool ckd_sub(ShellInt* result, ShellInt a, ShellInt b) {
    if ((b < 0 && a > MAX_SHELL_INT + b) || (b > 0 && a < MIN_SHELL_INT + b)) {
        return true;
    }

    *result = a - b;
    return false;
}

static inline bool ckd_mul(ShellInt* result, ShellInt a, ShellInt b) {
    if (a > 0) {
        if ((b > 0 && a > MAX_SHELL_INT / b) ||
                (b < 0 && b < MIN_SHELL_INT / a)) {
            return true;
        }
    } else if (a < 0) {
        if ((b > 0 && a < MIN_SHELL_INT / b) ||
                (b < 0 && (a == -1 || b == -1 ? 0 : b < MAX_SHELL_INT / a))) {
            return true;
        }
    }

    *result = a * b;
    return false;
}
#endif

static bool interpretExpression(const char** expression, ShellInt* result);

struct Rule {
    size_t (*acceptOperator)(const char* expression);
    bool (*evaluate)(ShellInt left, const char* accepted, size_t acceptedLength,
            ShellInt right, ShellInt* result);
    const struct Rule* nextRule;
};

static void trim(const char** expression) {
    while (isblank((unsigned char) **expression)) {
        (*expression)++;
    }
}

// primary : identifier
//         | constant
//         | '(' expression ')'
static bool interpretPrimaryExpression(const char** expression,
        ShellInt* result) {
    trim(expression);

    if (**expression == '(') {
        (*expression)++;
        if (!interpretExpression(expression, result)) {
            return false;
        }

        trim(expression);

        if (**expression != ')') {
            return false;
        }
        (*expression)++;
        return true;
    }

    if (isdigit((unsigned char) **expression) || **expression == '-') {
        errno = 0;
        char* endptr;
        ShellInt value = parse_shell_int(*expression, &endptr, 0);
        if (errno || endptr == *expression) {
            return false;
        }
        *expression = endptr;
        if (result) {
            *result = value;
        }
        return true;
    }

    const char* name = *expression;
    size_t nameLength = getRegularVariableNameLength(name);
    if (nameLength == 0) return false;
    (*expression) += nameLength;

    if (result) {
        char* variableName = strndup(name, nameLength);
        if (!variableName) err(1, "malloc");
        const char* value = getVariable(variableName);
        free(variableName);

        if (!value) {
            *result = 0;
            return true;
        }

        errno = 0;
        char* endptr;
        ShellInt parsedValue = parse_shell_int(value, &endptr, 0);
        if (errno || *endptr) {
            return false;
        }
        *result = parsedValue;
    }
    return true;
}

// unary : ('+' | '-' | '~' | '!')* primary
static bool interpretUnaryExpression(const char** expression,
        ShellInt* result) {
    trim(expression);

    char operator = (*expression)[0];
    if (operator == '+' || operator == '!') {
        if ((*expression)[1] == '=') {
            operator = '\0';
        }
    } else if (operator == '-') {
        // We do not handle a minus sign followed by a digit as a unary minus
        // even though the grammar treats it as such, because MIN_SHELL_INT
        // would overflow if the value after the sign was parsed as a positive
        // integer. This overflow needs to be supported so that $((MIN)) and
        // $(($MIN)) result in the same value as required by POSIX.
        if ((*expression)[1] == '=' ||
                isdigit((unsigned char) (*expression)[1])) {
            operator = '\0';
        }
    } else if (operator != '~') {
        operator = '\0';
    }

    if (!operator) {
        return interpretPrimaryExpression(expression, result);
    }
    (*expression)++;

    ShellInt right;
    if (!interpretUnaryExpression(expression, result ? &right : NULL)) {
        return false;
    }

    if (!result) return true;

    switch (operator) {
    case '+':
        *result = +right;
        return true;
    case '-':
        if (right == MIN_SHELL_INT) {
            return false;
        }
        *result = -right;
        return true;
    case '~':
        *result = ~right;
        return true;
    case '!':
        *result = !right;
        return true;
    default:
        return false;
    }
}

// multiplicative : unary (('*' | '/' | '%') unary)*
static size_t acceptMultiplicative(const char* expression) {
    switch (expression[0]) {
    case '*': case '/': case '%':
        return expression[1] != '=' ? 1 : 0;
    default: return 0;
    }
}

static bool evalMultiplicative(ShellInt left, const char* accepted,
        size_t acceptedLength, ShellInt right, ShellInt* result) {
    (void) acceptedLength;

    switch (*accepted) {
    case '*':
        return !ckd_mul(result, left, right);
    case '/':
        if (right == 0 || (left == MIN_SHELL_INT && right == -1)) {
            return false;
        }
        *result = left / right;
        return true;
    case '%':
        if (right == 0 || (left == MIN_SHELL_INT && right == -1)) {
            return false;
        }
        *result = left % right;
        return true;
    default: return false;
    }
}

static const struct Rule ruleMultiplicative =
        { acceptMultiplicative, evalMultiplicative, NULL };

// additive : multiplicative (('+' | '-' ) multiplicative)*
static size_t acceptAdditive(const char* expression) {
    switch (expression[0]) {
    case '+': case '-':
        return expression[1] != '=' ? 1 : 0;
    default: return 0;
    }
}

static bool evalAdditive(ShellInt left, const char* accepted,
        size_t acceptedLength, ShellInt right, ShellInt* result) {
    (void) acceptedLength;

    switch (*accepted) {
    case '+':
        return !ckd_add(result, left, right);
    case '-':
        return !ckd_sub(result, left, right);
    default: return false;
    }
}

static const struct Rule ruleAdditive =
        { acceptAdditive, evalAdditive, &ruleMultiplicative };

// shift : additive (('<<' | '>>') additive)*
static size_t acceptShift(const char* expression) {
    return (expression[0] == '<' || expression[0] == '>') &&
            expression[0] == expression[1] && expression[2] != '=' ? 2 : 0;
}

static bool evalShift(ShellInt left, const char* accepted,
        size_t acceptedLength, ShellInt right, ShellInt* result) {
    (void) acceptedLength;

    switch (*accepted) {
    case '<':
        if (left < 0 || right < 0 || right >= (ShellInt) SHELL_INT_WIDTH) {
            return false;
        }
        if (left > (MAX_SHELL_INT >> right)) {
            return false;
        }

        *result = left << right;
        return true;
    case '>':
        if (left < 0 || right < 0 || right >= (ShellInt) SHELL_INT_WIDTH) {
            return false;
        }

        *result = left >> right;
        return true;
    default: return false;
    }
}

static const struct Rule ruleShift = { acceptShift, evalShift, &ruleAdditive };

// relational : shift (('<' | '>' | '<=' | '>=') shift)*
static size_t acceptRelational(const char* expression) {
    if (expression[0] == '<' || expression[0] == '>') {
        if (expression[0] == expression[1]) {
            return 0;
        } else if (expression[1] == '=') {
            return 2;
        } else {
            return 1;
        }
    }
    return 0;
}

static bool evalRelational(ShellInt left, const char* accepted,
        size_t acceptedLength, ShellInt right, ShellInt* result) {
    switch (*accepted) {
    case '<':
        if (acceptedLength == 2) {
            *result = left <= right;
        } else {
            *result = left < right;
        }

        break;
    case '>':
        if (acceptedLength == 2) {
            *result = left >= right;
        } else {
            *result = left > right;
        }

        break;
    default:;
    }
    return true;
}

static const struct Rule ruleRelational =
        { acceptRelational, evalRelational, &ruleShift };

// equality : relational (('==' | '!=') relational)*
static size_t acceptEquality(const char* expression) {
    return (expression[0] == '=' || expression[0] == '!') &&
            expression[1] == '=' ? 2 : 0;
}

static bool evalEquality(ShellInt left, const char* accepted,
        size_t acceptedLength, ShellInt right, ShellInt* result) {
    (void) acceptedLength;

    switch (*accepted) {
    case '=':
        *result = left == right;

        break;
    case '!':
        *result = left != right;

        break;
    default:;
    }
    return true;
}

static const struct Rule ruleEquality =
        { acceptEquality, evalEquality, &ruleRelational };

// AND : equality ('&' equality)*
static size_t acceptAnd(const char* expression) {
    return expression[0] == '&' && expression[1] != '=' && expression[1] != '&';
}

static bool evalAnd(ShellInt left, const char* accepted,
        size_t acceptedLength, ShellInt right, ShellInt* result) {
    (void) accepted; (void) acceptedLength;

    *result = left & right;
    return true;
}

static const struct Rule ruleAnd = { acceptAnd, evalAnd, &ruleEquality };

// exclusive-OR : AND ('^' AND)*
static size_t acceptExclusiveOr(const char* expression) {
    return expression[0] == '^' && expression[1] != '=' ? 1 : 0;
}

static bool evalExclusiveOr(ShellInt left, const char* accepted,
        size_t acceptedLength, ShellInt right, ShellInt* result) {
    (void) accepted; (void) acceptedLength;

    *result = left ^ right;
    return true;
}

static const struct Rule ruleExclusiveOr =
        { acceptExclusiveOr, evalExclusiveOr, &ruleAnd };

// inclusive-OR : exclusive-OR ('|' exclusive-OR)*
static size_t acceptInclusiveOr(const char* expression) {
    return expression[0] == '|' && expression[1] != '=' && expression[1] != '|';
}

static bool evalInclusiveOr(ShellInt left, const char* accepted,
        size_t acceptedLength, ShellInt right, ShellInt* result) {
    (void) accepted; (void) acceptedLength;

    *result = left | right;
    return true;
}

static const struct Rule ruleInclusiveOr =
        { acceptInclusiveOr, evalInclusiveOr, &ruleExclusiveOr };

// If result is null, only parse the expression but don't evaluate it.
static bool interpretRule(const char** expression, const struct Rule* rule,
        ShellInt* result) {
    ShellInt left;
    if (rule->nextRule) {
        if (!interpretRule(expression, rule->nextRule, result ? &left : NULL)) {
            return false;
        }
    } else if (!interpretUnaryExpression(expression, result ? &left : NULL)) {
        return false;
    }

    while (true) {
        trim(expression);

        size_t length = rule->acceptOperator(*expression);
        if (length == 0) {
            if (result) {
                *result = left;
            }
            return true;
        }

        const char* operator = *expression;
        (*expression) += length;

        ShellInt right;
        if (rule->nextRule) {
            if (!interpretRule(expression, rule->nextRule,
                    result ? &right : NULL)) {
                return false;
            }
        } else if (!interpretUnaryExpression(expression,
                result ? &right : NULL)) {
            return false;
        }

        if (result) {
            if (!rule->evaluate(left, operator, length, right, &left)) {
                return false;
            }
        }

    }
}

// logical-AND : inclusive-OR ('&&' inclusive-OR)*
static bool interpretLogicalAnd(const char** expression, ShellInt* result) {
    ShellInt left;
    if (!interpretRule(expression, &ruleInclusiveOr, result ? &left : NULL)) {
        return false;
    }

    trim(expression);

    if ((*expression)[0] != '&' || (*expression)[1] != '&') {
        if (result) {
            *result = left;
        }
        return true;
    }
    (*expression) += 2;

    ShellInt right;
    if (!interpretLogicalAnd(expression, result && left ? &right : NULL)) {
        return false;
    }

    if (result) {
        *result = left && right;
    }
    return true;
}

// logical-OR : logical-AND ('||' logical-AND)*
static bool interpretLogicalOr(const char** expression, ShellInt* result) {
    ShellInt left;
    if (!interpretLogicalAnd(expression, result ? &left : NULL)) {
        return false;
    }

    trim(expression);

    if ((*expression)[0] != '|' || (*expression)[1] != '|') {
        if (result) {
            *result = left;
        }
        return true;
    }
    (*expression) += 2;

    ShellInt right;
    if (!interpretLogicalOr(expression, result && !left ? &right : NULL)) {
        return false;
    }

    if (result) {
        *result = left || right;
    }
    return true;
}

// conditional : logical-OR
//             | logical-OR '?' expression ':' conditional
static bool interpretConditionalExpression(const char** expression,
        ShellInt* result) {
    ShellInt condition = 0;
    if (!interpretLogicalOr(expression, result ? &condition : NULL)) {
        return false;
    }

    trim(expression);

    if (**expression != '?') {
        if (result) {
            *result = condition;
        }
        return true;
    }
    (*expression)++;

    if (!interpretExpression(expression, condition ? result : NULL)) {
        return false;
    }

    trim(expression);

    if (**expression != ':') {
        return false;
    }
    (*expression)++;

    if (!interpretConditionalExpression(expression,
            condition ? NULL : result)) {
        return false;
    }

    return true;
}

// assignment : conditional
//            | lvalue-unary assignment-operator assignment
//
// lvalue-unary : identifier
//              | '(' lvalue-unary ')'
//
// assignment-operator : '='  | '*=' | '/=' | '%=' | '+=' | '-=' | '<<=' | '>>='
//                     | '&=' | '^=' | '|='
static bool interpretAssignment(const char** expression, ShellInt* result) {
    // First determine whether this is an assignment at all.
    const char* firstPass = *expression;
    if (!interpretConditionalExpression(&firstPass, NULL)) {
        return false;
    }

    trim(&firstPass);
    size_t assignmentOpLength = 0;
    if (firstPass[0] == '=' && firstPass[1] != '=') {
        assignmentOpLength = 1;
    } else if (strchr("*/%+-&^|", firstPass[0]) && firstPass[1] == '=') {
        assignmentOpLength = 2;
    } else if ((firstPass[0] == '<' || firstPass[0] == '>') &&
            firstPass[1] == firstPass[0] && firstPass[2] == '=') {
        assignmentOpLength = 3;
    }

    if (assignmentOpLength == 0) {
        return interpretConditionalExpression(expression, result);
    }

    // The left side of the assignment needs to be a unary expression that is a
    // modifiable lvalue. That means that out of the supported operators, it can
    // only be a variable name optionally surrounded by parentheses.

    trim(expression);
    size_t parentheses = 0;
    while ((*expression)[0] == '(') {
        parentheses++;
        (*expression)++;
        trim(expression);
    }

    const char* name = *expression;
    size_t nameLength = getRegularVariableNameLength(name);
    if (nameLength == 0) return false;
    (*expression) += nameLength;

    for (size_t i = 0; i < parentheses; i++) {
        trim(expression);
        if ((*expression)[0] != ')') {
            return false;
        }
        (*expression)++;
    }

    trim(expression);

    if (*expression != firstPass) {
        // If we ended up in a different place of the expression than in the
        // first pass, that means that left part of the assigment was not an
        // lvalue.
        return false;
    }

    const char* operator = *expression;

    (*expression) += assignmentOpLength;

    ShellInt right;
    if (!interpretAssignment(expression, result ? &right : 0)) {
        return false;
    }

    if (!result) {
        return true;
    }

    ShellInt left;
    if (*operator != '=') {
        char* variableName = strndup(name, nameLength);
        if (!variableName) err(1, "malloc");
        const char* nameExpr = variableName;
        if (!interpretPrimaryExpression(&nameExpr, &left)) {
            free(variableName);
            return false;
        }
        free(variableName);
    }

    switch (*operator) {
    case '=':
        *result = right;
        break;
    case '*':
        if (ckd_mul(result, left, right)) {
            return false;
        }
        break;
    case '/':
        if (right == 0 || (left == MIN_SHELL_INT && right == -1)) {
            return false;
        }
        *result = left / right;
        break;
    case '%':
        if (right == 0 || (left == MIN_SHELL_INT && right == -1)) {
            return false;
        }
        *result = left % right;
        break;
    case '+':
        if (ckd_add(result, left, right)) {
            return false;
        }
        break;
    case '-':
        if (ckd_sub(result, left, right)) {
            return false;
        }
        break;
    case '<':
        if (left < 0 || right < 0 || right >= (ShellInt) SHELL_INT_WIDTH) {
            return false;
        }
        if (left > (MAX_SHELL_INT >> right)) {
            return false;
        }

        *result = left << right;
        break;
    case '>':
        if (left < 0 || right < 0 || right >= (ShellInt) SHELL_INT_WIDTH) {
            return false;
        }

        *result = left >> right;
        break;
    case '&':
        *result = left & right;
        break;
    case '^':
        *result = left ^ right;
        break;
    case '|':
        *result = left | right;
        break;
    default: return false;
    }

    char buffer[sizeof(ShellInt) * 3];
    snprintf(buffer, sizeof(buffer), "%"SHELL_INT_PRId, *result);
    char* variableName = strndup(name, nameLength);
    if (!variableName) err(1, "malloc");
    setVariable(variableName, buffer, 0);
    free(variableName);

    return true;
}

// expression : assignment
static bool interpretExpression(const char** expression, ShellInt* result) {
    return interpretAssignment(expression, result);
}

bool evaluateArithmeticExpression(const char* expandedExpression,
        ShellInt* result) {
    // First interpret the expression without evaluation to detect syntax errors
    // before assignments take place.
    const char* expression = expandedExpression;
    if (!interpretExpression(&expression, NULL)) {
        // Syntax error.
        return false;
    }

    trim(&expression);
    if (*expression != '\0') {
        // Trailing tokens at the end of the expression.
        return false;
    }

    expression = expandedExpression;
    return interpretExpression(&expression, result);
}
