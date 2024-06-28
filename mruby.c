#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include <gawkapi.h>

#include <mruby.h>
#include <mruby/compile.h>
#include <mruby/error.h>
#include <mruby/internal.h>
#include <mruby/string.h>

int plugin_is_GPL_compatible;

static const gawk_api_t *api;

static awk_ext_id_t ext_id;
static const char *ext_version = "awk-mruby-ext v1.0.0";

static mrb_state *mrb;
static mrbc_context *cxt;

static awk_value_t *do_mruby_eval(int nargs, awk_value_t *result, struct awk_ext_func *unused)
{
    awk_value_t code;

    if (nargs != 1)
        fatal(ext_id, "mruby_eval: expects 1 arguments but called with %d", nargs);

    if (!get_argument(0, AWK_STRING, &code))
        fatal(ext_id, "mruby_eval: cannot retrieve 1st argument as a string");

    mrbc_filename(mrb, cxt, "mruby_eval");
    int ai = mrb_gc_arena_save(mrb);
    mrb_value rr = mrb_load_string_cxt(mrb, code.str_value.str, cxt);
    if (mrb->exc != NULL)
    {
        const char *e = mrb_str_to_cstr(mrb, mrb_exc_inspect(mrb, mrb_obj_value(mrb->exc)));
        fatal(ext_id, "mruby_eval: %s", e);
    }
    char *str = mrb_str_to_cstr(mrb, mrb_funcall(mrb, rr, "to_s", 0));
    make_const_string(str, strlen(str), result);

    mrb_gc_arena_restore(mrb, ai);

    return result;
}

static awk_ext_func_t func_table[] = {
    {"mruby_eval", do_mruby_eval, 1, 1, awk_false, NULL},
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
