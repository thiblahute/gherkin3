#ifndef __GERKINASTNODE_H_
#define __GERKINASTNODE_H_

#include <glib.h>

G_BEGIN_DECLS

typedef struct _GherkinParser GErkinParser;

#define GHERKIN_TYPE_PARSER (gherkin_parser_get_type())

G_DECLARE_FINAL_TYPE(GherkinParser, ghenkin_parser, GHERKIN, PARSER, GObject)

G_END_DECLS

#endif /* __GERKINASTNODE_H_ */ 

