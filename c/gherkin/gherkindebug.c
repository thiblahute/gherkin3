#include "gherkindebug.h"

static unsigned int gherkin_debug_flags = 0;

static const GDebugKey gherkin_debug_keys[] = {
  { "parser", GHERKIN_DEBUG_PARSER },
  { "astbuilder", GHERKIN_DEBUG_AST_BUILDER },
  { "formatter", GHERKIN_DEBUG_FORMATTER }
};

GherkinDebugFlags
gherkin_get_debug_flags (void)
{
  static gboolean gherkin_debug_flags_set;
  const gchar *env_str;

  if (G_LIKELY (gherkin_debug_flags_set))
    return gherkin_debug_flags;

  env_str = g_getenv ("GHERKIN_DEBUG");
  if (env_str != NULL && *env_str != '\0')
    {
      gherkin_debug_flags |= g_parse_debug_string (env_str,
                                                gherkin_debug_keys,
                                                G_N_ELEMENTS (gherkin_debug_keys));
    }

  gherkin_debug_flags_set = TRUE;

  return gherkin_debug_flags;
}
