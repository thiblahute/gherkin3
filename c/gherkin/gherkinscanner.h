#ifndef __GHERKIN_SCANNER_H__
#define __GHERKIN_SCANNER_H__

#include <glib.h>
#include <json-glib/json-glib.h>
#include <json-glib/json-gobject.h>

G_BEGIN_DECLS

GScanner * gherkin_scanner_new          (gchar * input_text,
                                        guint text_length);

guint      gherkin_scanner_parse_symbol (GScanner *scanner);

JsonNode * gherkin_scanner_get_root     (GScanner * scanner);
gboolean   gherkin_scanner_parse        (GScanner * scanner);

G_END_DECLS

#endif /* __GHERKIN_SCANNER_H__ */

