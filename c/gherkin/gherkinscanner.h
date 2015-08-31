#ifndef __GHERKIN_SCANNER_H__
#define __GHERKIN_SCANNER_H__

#include <glib.h>
#include <json-glib/json-glib.h>
#include <json-glib/json-gobject.h>

#include "gherkintypes.h"

G_BEGIN_DECLS

GScanner * gherkin_scanner_new             (gchar * input_text,
                                            guint text_length,
                                            gchar *input_name);
guint      gherkin_scanner_next_token      (GScanner *scanner);
gboolean   gherkin_is_scanner              (GScanner *scanner);
gboolean   gherkin_scanner_is_ending_token (guint token);
gboolean   gherkin_scanner_peek_next_token (GScanner * scanner);

G_END_DECLS

#endif /* __GHERKIN_SCANNER_H__ */

