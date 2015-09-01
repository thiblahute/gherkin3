#ifndef __GHERKINPARSER_H__
#define __GHERKINPARSER_H__

#include <glib.h>
#include <glib-object.h>

#include "gherkinastbuilder.h"

G_BEGIN_DECLS

typedef struct _GherkinParser GherkinParser;

#define GHERKIN_TYPE_PARSER (gherkin_parser_get_type())

G_DECLARE_FINAL_TYPE(GherkinParser, gherkin_parser, GHERKIN, PARSER, GObject)

GherkinParser * gherkin_parser_new   (GScanner * scanner,
                                      GherkinFormatter *formatter);
gboolean        gherkin_parser_parse (GherkinParser * parser);

G_END_DECLS

#endif /* __GHERKINPARSER_H__ */ 
