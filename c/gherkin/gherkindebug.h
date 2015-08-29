#ifndef __GHERKIN_DEBUG_G__
#define __GHERKIN_DEBUG_G__

# ifdef __GNUC__
# define DEBUG(x,a...)                 G_STMT_START {  \
  if (g_getenv ("GHERKIN_DEBUG")) \
    g_message (G_STRLOC ": " x, ##a);      \
} G_STMT_END

# else
/* Try the C99 version; unfortunately, this does not allow us to pass
 * empty arguments to the macro, which means we have to
 * do an intemediate printf.
 */
# define DEBUG(type,...)                    G_STMT_START {  \
  if (g_getenv ("GHERKIN_DEBUG")) { \
    gchar * _fmt = g_strdup_printf (__VA_ARGS__);       \
    g_message (G_STRLOC ": %s",_fmt);    \
    g_free (_fmt);                                      \
  }\
} G_STMT_END

# endif /* __GNUC__ */
# endif /*__GHERKIN_DEBUG_G__ */
