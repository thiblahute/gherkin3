#include "gherkindebug.h"
#include "gherkintypes.h"
#include "gherkinastbuilder.h"

static gboolean debug_ast_builder = 1;

typedef struct 
{
  GObject parent;

  GQueue stack;
} GherkinAstBuilderPrivate;

struct _GherkinAstBuilder
{
  GObject parent;

  GherkinAstBuilderPrivate *priv;
};

/*  *INDENT-OFF* */
G_DEFINE_TYPE_WITH_PRIVATE (GherkinAstBuilder, gherkin_ast_builder, G_TYPE_OBJECT)
/*  *INDENT-ON* */

static gboolean
_print_stack (GNode *node, gpointer unused)
{
  GherkinRule *rule = node->data;

  g_print ("%*s%s '%p:%s' (parent: %p:%s) (depth %d)\n",
      (g_node_depth (node) -1) * 4, "", 
      gherkin_rule_type_get_name (GHERKIN_RULE_TYPE (rule)),
      (gpointer) rule, GHERKIN_RULE_NAME (rule) ? GHERKIN_RULE_NAME (rule) : "No name",
      node->parent ? node->parent->data : "",
      node->parent ? gherkin_rule_type_get_name (GHERKIN_RULE_TYPE (node->parent->data)) :
      "None", g_node_depth (node));

  return FALSE;
}

void
gherkin_ast_builder_dump_ast (GherkinAstBuilder *self)
{
  g_node_traverse (self->priv->stack.head->data,
      G_PRE_ORDER, G_TRAVERSE_ALL, -1, _print_stack, NULL);
}

GherkinRule * 
gherkin_ast_builder_start_rule (GherkinAstBuilder * self,
    GherkinRuleType rule_type)
{
  GherkinRule *rule = gherkin_rule_new (rule_type);

  if (debug_ast_builder) DEBUG ("Starting rule: %s", gherkin_rule_type_get_name (rule_type));
  g_queue_push_head (&self->priv->stack,
      g_node_new (rule));

  return rule;
}

void
gherkin_ast_builder_end_rule (GherkinAstBuilder * self,
    GherkinRuleType rule_type)
{
  GNode *node = g_queue_pop_head (&self->priv->stack);

  if (debug_ast_builder) DEBUG ("Ending rule: %s", gherkin_rule_type_get_name (rule_type));
  g_node_append (self->priv->stack.head->data, node);
}

static void
gherkin_ast_builder_init (GherkinAstBuilder * self)
{
  self->priv = G_TYPE_INSTANCE_GET_PRIVATE (self, GHERKIN_TYPE_AST_BUILDER,
      GherkinAstBuilderPrivate);

  g_queue_init (&self->priv->stack);

  g_queue_push_head (&self->priv->stack,
      g_node_new (gherkin_rule_new (-1)));
}

static void
gherkin_ast_builder_class_init (GherkinAstBuilderClass * klass)
{
  return;
}

GherkinAstBuilder *
gherkin_ast_builder_new (void)
{
  return g_object_new (GHERKIN_TYPE_AST_BUILDER, NULL);
}
