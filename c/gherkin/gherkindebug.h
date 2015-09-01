#ifndef __GHERKIN_DEBUG_H__
#define __GHERKIN_DEBUG_H__

#include <glib.h>

G_BEGIN_DECLS

typedef enum {
  GHERKIN_DEBUG_AST_BUILDER = 1 << 0,
  GHERKIN_DEBUG_FORMATTER   = 1 << 1,
  GHERKIN_DEBUG_PARSER      = 1 << 2
} GherkinDebugFlags;

G_GNUC_INTERNAL
GherkinDebugFlags gherkin_get_debug_flags (void);

#define GHERKIN_HAS_DEBUG(flag)    (gherkin_get_debug_flags () & GHERKIN_DEBUG_##flag)

# ifdef __GNUC__

# define DEBUG(type,x,a...)                 G_STMT_START {  \
        if (GHERKIN_HAS_DEBUG (type)) {                            \
          g_message ("[" #type "] " G_STRLOC ": " x, ##a);      \
        }                                       } G_STMT_END

# else
/* Try the C99 version; unfortunately, this does not allow us to pass
 * empty arguments to the macro, which means we have to
 * do an intemediate printf.
 */
# define DEBUG(type,...)                    G_STMT_START {  \
        if (GHERKIN_HAS_DEBUG (type)) {                            \
            gchar * _fmt = g_strdup_printf (__VA_ARGS__);       \
            g_message ("[" #type "] " G_STRLOC ": %s",_fmt);    \
            g_free (_fmt);                                      \
        }                                       } G_STMT_END

# endif /* __GNUC__ */

#else

#define DEBUG(type,...)         G_STMT_START { } G_STMT_END

G_END_DECLS

#endif /* __GHERKIN_DEBUG_H__ */
