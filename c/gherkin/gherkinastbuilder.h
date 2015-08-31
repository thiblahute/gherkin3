#ifndef __GHERKIN_AST_BUILDER_H__
#define __GHERKIN_AST_BUILDER_H__

#include <glib.h>
#include <glib-object.h>


G_BEGIN_DECLS

typedef struct _GherkinAstBuilder GherkinAstBuilder;

#define GHERKIN_TYPE_AST_BUILDER (gherkin_ast_builder_get_type())

G_DECLARE_FINAL_TYPE(GherkinAstBuilder, gherkin_ast_builder, GHERKIN, AST_BUILDER, GObject)

GherkinAstBuilder * gherkin_ast_builder_new        (void);
GherkinRule *       gherkin_ast_builder_start_rule (GherkinAstBuilder * self,
                                                    GherkinRuleType rule_type);
void                gherkin_ast_builder_end_rule   (GherkinAstBuilder * self,
                                                    GherkinRuleType rule_type);
void                gherkin_ast_builder_dump_ast   (GherkinAstBuilder *self);

G_END_DECLS

#endif /* __GHERKIN_AST_BUILDER_H__ */ 

