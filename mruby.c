#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include <gawkapi.h>

#include <mruby.h>
#include <mruby/compile.h>
#include <mruby/string.h>

int plugin_is_GPL_compatible;

static const gawk_api_t *api;

static awk_ext_id_t ext_id;
static const char *ext_version = "awk-mruby-ext v1.0.0";

static mrb_state *mrb;

static awk_value_t *do_mruby_eval(int nargs, awk_value_t *result, struct awk_ext_func *unused)
{
    awk_value_t code;

    if (nargs != 1)
        fatal(ext_id, "mruby_eval: expects 1 arguments but called with %d", nargs);

    if (!get_argument(0, AWK_STRING, &code))
        fatal(ext_id, "mruby_eval: cannot retrieve 1st argument as a string");

    mrb_value e = mrb_funcall(mrb, mrb_load_string(mrb, code.str_value.str), "to_s", 0);
    char *str = mrb_str_to_cstr(mrb, e);
    make_const_string(str, strlen(str), result);

    return result;
}

static awk_ext_func_t func_table[] = {
    {"mruby_eval", do_mruby_eval, 1, 1, awk_false, NULL},
};

static void at_exit(void *data, int exit_status)
{
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

    awk_atexit(at_exit, NULL);
    return awk_true;
}

static awk_bool_t (*init_func)(void) = init_mruby;

dl_load_func(func_table, mruby, "")
