#ifndef __GHERKIN_DEBUG_G__
#define __GHERKIN_DEBUG_G__

/* Try the C99 version; unfortunately, this does not allow us to pass
 * empty arguments to the macro, which means we have to
 * do an intemediate printf.
 */
# define DEBUG(fmt,...)                    G_STMT_START {  \
  if (g_getenv ("GHERKIN_DEBUG")) { \
    gchar * _fmt = g_strdup_printf (fmt, __VA_ARGS__);       \
    g_message (G_STRLOC ": %s",_fmt);    \
    g_free (_fmt);                                      \
  }\
} G_STMT_END

# endif /*__GHERKIN_DEBUG_G__ */
