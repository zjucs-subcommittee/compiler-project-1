#pragma once

#include <sstream>
#include <string>
#include <vector>
#include <map>
#include <memory>
#include <variant>
#include "utils.h"
#include "parse_tree.h"
#include "type.h"
#include "function.h"
#include "stack_map.h"
#include "exception.h"

struct syntax_var_def
{
    std::string name;
    syntax_type type;
};

struct syntax_type_def
{
    std::string name;
    syntax_type type;
};

struct syntax_literal
{
    syntax_type type;
    std::variant<unsigned long long, double, float, std::string> val;
};

struct syntax_new_expr
{
    syntax_type type;
    std::shared_ptr<syntax_expr> count;
};

struct syntax_var
{
};

struct syntax_type_convert
{
    std::shared_ptr<syntax_expr> source_expr;
    syntax_type target_type;
};

struct syntax_arr_member
{
    std::shared_ptr<syntax_expr> base;
    std::shared_ptr<syntax_expr> idx;
};

struct syntax_dot
{
    std::string field;
    std::shared_ptr<syntax_expr> val;
};

struct syntax_construct
{
    std::vector<std::string> label;
    std::vector<std::shared_ptr<syntax_expr>> val;
};

struct syntax_expr
{
    using val_t = std::variant<syntax_fun_call, syntax_literal, syntax_var,
                               syntax_type_convert, syntax_dot, syntax_arr_member, syntax_new_expr>;

    bool immutable = true;
    syntax_type type;

    val_t val;

    void *reserved = nullptr;

    syntax_expr() = default;

    syntax_expr(const val_t &v, syntax_type t) : type(t), val(v)
    {
        immutable = true;
        reserved = nullptr;
    }

    std::string to_string() const
    {
        if (std::get_if<syntax_fun_call>(&val))
        {
            return "call";
        }
        else if (std::get_if<syntax_literal>(&val))
        {
            return "lit";
        }
        else if (std::get_if<syntax_var>(&val))
        {
            return "var";
        }
        else if (std::get_if<syntax_type_convert>(&val))
        {
            return "convert";
        }
        else if (std::get_if<syntax_dot>(&val))
        {
            return "dot";
        }
        else if (std::get_if<syntax_arr_member>(&val))
        {
            return "arr";
        }
        else if (std::get_if<syntax_new_expr>(&val))
        {
            return "new";
        }
        return "expr";
    }

    // 不是则返回 0
    int is_sum_dot() const
    {
        if (auto pdot = std::get_if<syntax_dot>(&val))
        {
            auto inner = pdot->val;
            auto choose = pdot->field;

            if (auto psum = std::get_if<sum_type>(&inner->type.type))
            {
                return psum->get_index(choose) + 1;
            }
        }
        return 0;
    }

    bool is_left_value() const
    {
        if (auto p = std::get_if<syntax_dot>(&val))
        {
            return p->val->is_left_value();
        }
        else if (auto p = std::get_if<syntax_arr_member>(&val))
        {
            return true;
        }
        else if (auto p = std::get_if<syntax_var>(&val))
        {
            return true;
        }
        else
        {
            return false;
        }
    }

    bool is_immutable() const
    {
        if (auto p = std::get_if<syntax_var>(&val))
        {
            return immutable;
        }
        else if (auto p = std::get_if<syntax_dot>(&val))
        {
            return p->val->is_immutable();
        }
        else if (auto p = std::get_if<syntax_arr_member>(&val))
        {
            return false;
        }
        else
        {
            return true;
        }
    }
};

struct syntax_assign
{
    std::shared_ptr<syntax_expr> lval, rval;
};

struct syntax_return
{
    std::shared_ptr<syntax_expr> val;
};

struct syntax_if_block;
struct syntax_while_block;
struct syntax_for_block;

struct syntax_stmt
{
    std::variant<
        std::shared_ptr<syntax_expr>,
        std::shared_ptr<syntax_if_block>,
        std::shared_ptr<syntax_while_block>,
        std::shared_ptr<syntax_for_block>,
        syntax_assign,
        syntax_return>
        stmt;

    std::string to_string() const
    {
        if (auto e = std::get_if<std::shared_ptr<syntax_expr>>(&stmt))
        {
            return e->get()->to_string();
        }
        else if (std::get_if<std::shared_ptr<syntax_if_block>>(&stmt))
        {
            return "if";
        }
        else if (std::get_if<std::shared_ptr<syntax_while_block>>(&stmt))
        {
            return "while";
        }
        else if (std::get_if<syntax_assign>(&stmt))
        {
            return "assign";
        }
        else if (std::get_if<syntax_return>(&stmt))
        {
            return "return";
        }
        return "nothing";
    }
};

struct syntax_if_block
{
    std::shared_ptr<syntax_expr> condition;
    std::vector<syntax_stmt> cond_stmt;
    std::vector<syntax_stmt> then_stmt;
    std::vector<syntax_stmt> else_stmt;
};

struct syntax_merged_if_block
{
    std::vector<std::shared_ptr<syntax_expr>> condition;
    std::vector<std::vector<syntax_stmt>> condition_stmt;
    std::vector<std::vector<syntax_stmt>> branch;
    std::vector<syntax_stmt> default_branch;

    std::shared_ptr<syntax_if_block> reduce(int index);
};

struct syntax_while_block
{
    std::shared_ptr<syntax_expr> condition;
    std::vector<syntax_stmt> condition_stmt;
    std::vector<syntax_stmt> body;
};

struct syntax_for_block
{
    std::shared_ptr<syntax_expr> begin_test;
    std::vector<syntax_stmt> begin_test_stmt;
    std::vector<syntax_stmt> init_stmt;
    std::vector<syntax_stmt> end_process_stmt;
    std::vector<syntax_stmt> body;
};

std::shared_ptr<syntax_expr> expr_convert_to(std::shared_ptr<syntax_expr> expr, const syntax_type &target, std::vector<syntax_stmt> &stmts);

void try_sum_tag_assign(const std::shared_ptr<syntax_expr> expr, std::vector<syntax_stmt> &stmts);

class syntax_module
{
    std::shared_ptr<syntax_expr> expr_analysis(const node_expression &node, std::vector<syntax_stmt> &stmts);

    std::shared_ptr<syntax_expr> assign_analysis(const node_assign_expr &node, std::vector<syntax_stmt> &stmts);

    std::shared_ptr<syntax_expr> binary_expr_analysis(const node_binary_expr &node, std::vector<syntax_stmt> &stmts);

    std::shared_ptr<syntax_expr> unary_expr_analysis(const node_unary_expr &node, std::vector<syntax_stmt> &stmts);

    std::shared_ptr<syntax_expr> post_expr_analysis(const node_post_expr &node, std::vector<syntax_stmt> &stmts);

    std::vector<syntax_fun> fundef_analysis(const node_module &module);

    void typedef_analysis(const node_module &module);

    void global_var_analysis(const node_module &module);

    void function_analysis(const syntax_fun &node);

    syntax_type ret_type;

    void primary_assign(std::shared_ptr<syntax_expr> &lval, std::shared_ptr<syntax_expr> &rval, std::vector<syntax_stmt> &stmts);

    syntax_stmt if_analysis(const node_if_statement &node);

    syntax_stmt while_analysis(const node_while_statement &node);

    syntax_stmt for_analysis(const node_for_statement &node);

    std::vector<syntax_stmt> statement_analysis(std::vector<node_statement> origin_stmts);

    void add_var(const node_var_def_statement &def, std::vector<syntax_stmt> &stmts, bool is_global = false);

public:
    type_table env_type;
    function_table env_fun;

    std::vector<std::string> fun_name;
    std::vector<std::vector<syntax_stmt>> fun_impl;
    std::vector<std::vector<std::shared_ptr<syntax_expr>>> fun_args;

    stack_map<std::shared_ptr<syntax_expr>> env_var;

    void syntax_analysis(const node_module &module);
};
