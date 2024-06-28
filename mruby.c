#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <stdio.h>
#include <string.h>
#include <stdbool.h>
#include <sys/stat.h>
#include <gawkapi.h>

#include <mruby.h>
#include <mruby/array.h>
#include <mruby/compile.h>
#include <mruby/hash.h>
#include <mruby/error.h>
#include <mruby/internal.h>
#include <mruby/string.h>
#include <mruby/variable.h>

int plugin_is_GPL_compatible;

static const gawk_api_t *api;

static awk_ext_id_t ext_id;
static const char *ext_version = "awk-mruby-ext v1.1.0";

static mrb_state *mrb;
static mrbc_context *cxt;

#define MRUBY_EVAL_RESULT_SYMBOL "MRUBY_EVAL_RESULT"

static void mruby_value_to_awk_value(mrb_value val, awk_value_t *result, bool in_array)
{
    if (mrb_nil_p(val))
    {
        make_null_string(result);
        return;
    }

    awk_array_t ary;
    switch (mrb_type(val))
    {
    case MRB_TT_FALSE:
        make_bool(awk_false, result);
        break;
    case MRB_TT_TRUE:
        make_bool(awk_true, result);
        break;
    case MRB_TT_FIXNUM:
        make_number(mrb_fixnum(val), result);
        break;
    case MRB_TT_FLOAT:
        make_number(mrb_float(val), result);
        break;
    case MRB_TT_STRING:
        make_const_string(RSTRING_PTR(val), RSTRING_LEN(val), result);
        break;

    case MRB_TT_ARRAY:
        if (in_array)
            fatal(ext_id, "mruby_eval: nested array is not supported");

        ary = create_array();
        result->val_type = AWK_ARRAY;
        result->array_cookie = ary;

        size_t arr_len = RARRAY_LEN(val);
        for (size_t i = 0; i < arr_len; i++)
        {
            awk_value_t index, value;
            make_number(i, &index);
            mruby_value_to_awk_value(mrb_ary_ref(mrb, val, i), &value, true);
            set_array_element(ary, &index, &value);
        }
        break;

    case MRB_TT_HASH:
        if (in_array)
            fatal(ext_id, "mruby_eval: nested hash is not supported");

        ary = create_array();
        result->val_type = AWK_ARRAY;
        result->array_cookie = ary;

        mrb_value keys = mrb_hash_keys(mrb, val);
        size_t keys_len = RARRAY_LEN(keys);
        for (size_t i = 0; i < keys_len; i++)
        {
            awk_value_t key, value;
            mrb_value k = mrb_ary_ref(mrb, keys, i);
            mruby_value_to_awk_value(k, &key, true);
            mruby_value_to_awk_value(mrb_hash_get(mrb, val, k), &value, true);
            set_array_element(ary, &key, &value);
        }
        break;

    default:
        mrb_value s = mrb_funcall(mrb, val, "to_s", 0);
        mruby_value_to_awk_value(s, result, false);
        break;
    }
}

static awk_value_t *do_mruby_eval(int nargs, awk_value_t *result, struct awk_ext_func *unused)
{
    awk_value_t code;

    if (nargs != 1)
        fatal(ext_id, "mruby_eval: expects 1 arguments but called with %d", nargs);

    if (!get_argument(0, AWK_STRING, &code))
        fatal(ext_id, "mruby_eval: cannot retrieve 1st argument as a string");

    mrbc_filename(mrb, cxt, "mruby_eval");
    int ai = mrb_gc_arena_save(mrb);
    mrb_value eval_result = mrb_load_string_cxt(mrb, code.str_value.str, cxt);
    if (mrb->exc != NULL)
    {
        const char *e = mrb_str_to_cstr(mrb, mrb_exc_inspect(mrb, mrb_obj_value(mrb->exc)));
        fatal(ext_id, "mruby_eval: %s", e);
    }

    awk_value_t r;
    mruby_value_to_awk_value(eval_result, &r, false);
    if (!sym_update(MRUBY_EVAL_RESULT_SYMBOL, &r))
        fatal(ext_id, "mruby_eval: cannot update the result symbol");

    if (r.val_type == AWK_ARRAY)
    {
        mrb_value s = mrb_funcall(mrb, eval_result, "to_s", 0);
        make_const_string(RSTRING_PTR(s), RSTRING_LEN(s), result);
    }
    else
    {
        result->val_type = r.val_type;
        result->u = r.u;
    }

    mrb_gc_arena_restore(mrb, ai);

    return result;
}

static awk_value_t *do_mruby_set_val(int nargs, awk_value_t *result, struct awk_ext_func *unused)
{
    awk_value_t key, value;

    if (nargs != 2)
        fatal(ext_id, "mruby_set_val: expects 2 arguments but called with %d", nargs);

    if (!get_argument(0, AWK_STRING, &key))
        fatal(ext_id, "mruby_set_val: cannot retrieve 1st argument as a string");

    if (!get_argument(1, AWK_STRING, &value))
        fatal(ext_id, "mruby_set_val: cannot retrieve 2nd argument as a string");

    mrb_sym k = mrb_intern(mrb, key.str_value.str, key.str_value.len);
    mrb_value v = mrb_str_new_cstr(mrb, value.str_value.str);
    mrb_gv_set(mrb, k, v);

    return make_bool(awk_true, result);
}

static awk_ext_func_t func_table[] = {
    {"mruby_eval", do_mruby_eval, 1, 1, awk_false, NULL},
    {"mruby_set_val", do_mruby_set_val, 2, 2, awk_false, NULL},
};

static void at_exit(void *data, int exit_status)
{
    mrbc_context_free(mrb, cxt);
    mrb_close(mrb);
}

static void *allocf(mrb_state *mrb, void *p, size_t size, void *ud)
{
    if (size == 0)
    {
        gawk_free(p);
        return NULL;
    }

    return gawk_realloc(p, size);
}

static awk_bool_t init_mruby(void)
{
    mrb = mrb_open_allocf(allocf, NULL);
    cxt = mrbc_context_new(mrb);
    cxt->capture_errors = TRUE;

    awk_atexit(at_exit, NULL);
    return awk_true;
}

static awk_bool_t (*init_func)(void) = init_mruby;

dl_load_func(func_table, mruby, "")
