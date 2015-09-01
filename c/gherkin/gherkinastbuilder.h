#ifndef __GHERKIN_AST_BUILDER_H__
#define __GHERKIN_AST_BUILDER_H__

#include <glib.h>
#include <glib-object.h>

#include "gherkinformatter.h"

G_BEGIN_DECLS

#define GHERKIN_TYPE_AST_BUILDER (gherkin_ast_builder_get_type())
G_DECLARE_FINAL_TYPE(GherkinAstBuilder, gherkin_ast_builder, GHERKIN, AST_BUILDER, GherkinFormatter)

GherkinAstBuilder * gherkin_ast_builder_new        (void);
GNode             * gherkin_ast_builder_get_root   (GherkinAstBuilder *self);

G_END_DECLS

#endif /* __GHERKIN_AST_BUILDER_H__ */ 

