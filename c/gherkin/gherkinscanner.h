#ifndef __GHERKIN_SCANNER_H__
#define __GHERKIN_SCANNER_H__

#include <glib.h>
#include <json-glib/json-glib.h>
#include <json-glib/json-gobject.h>

G_BEGIN_DECLS

typedef enum {
  GHERKIN_TOKEN_SCENARIO = G_TOKEN_LAST + 1,
  GHERKIN_TOKEN_FEATURE = G_TOKEN_LAST + 2,
  GHERKIN_TOKEN_GIVEN = G_TOKEN_LAST + 3,
  GHERKIN_TOKEN_WHEN = G_TOKEN_LAST + 4,
  GHERKIN_TOKEN_THEN = G_TOKEN_LAST + 5,
  GHERKIN_TOKEN_TABLE = '|',
  GHERKIN_TOKEN_TAG = '@',
  GHERKIN_TOKEN_AND = G_TOKEN_LAST + 6,
  GHERKIN_TOKEN_EOF = G_TOKEN_EOF
} GherkinToken;

GScanner * gherkin_scanner_new             (gchar * input_text,
                                            guint text_length,
                                            gchar *input_name);
guint      gherkin_scanner_next_token      (GScanner *scanner);
gboolean   gherkin_is_scanner              (GScanner *scanner);
gboolean   gherkin_scanner_is_ending_token (guint token);
gboolean   gherkin_scanner_peek_next_token (GScanner * scanner);

G_END_DECLS

#endif /* __GHERKIN_SCANNER_H__ */

