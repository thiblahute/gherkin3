#include "gherkindebug.h"
#include "gherkintypes.h"
#include "gherkinastbuilder.h"

static gboolean debug_ast_builder = 1;

typedef struct _GherkinAstBuilderPrivate
{
  GQueue stack;
} GherkinAstBuilderPrivate;

struct _GherkinAstBuilder
{
  GherkinFormatter parent;

  GherkinAstBuilderPrivate *priv;
};

/*  *INDENT-OFF* */
G_DEFINE_TYPE_WITH_PRIVATE (GherkinAstBuilder, gherkin_ast_builder, GHERKIN_TYPE_FORMATTER)
/*  *INDENT-ON* */

#define priv(self) (((GherkinAstBuilderPrivate*) gherkin_ast_builder_get_instance_private (self)))

static gboolean
_print_stack (GNode * node, gpointer unused)
{
  GherkinRule *rule = node->data;

  g_print ("%*s%s '%p:%s' (parent: %p:%s)",
      (g_node_depth (node) -1) * 4, "",
      gherkin_rule_type_get_name (GHERKIN_RULE_TYPE (rule)),
      (gpointer) rule, GHERKIN_RULE_NAME (rule) ? GHERKIN_RULE_NAME (rule) : "No name",
      node->parent ? node->parent->data : "",
      node->parent ? gherkin_rule_type_get_name (GHERKIN_RULE_TYPE (node->parent->data)) :
      "None");


  if (GHERKIN_RULE_KEYWORD (rule))
    g_print (" | Keyword '%s'", GHERKIN_RULE_KEYWORD (rule));

  if (GHERKIN_RULE_TYPE (rule) == GHERKIN_RULE_Step)
    g_print (" %s ", GHERKIN_STEP_TEXT (rule));
  else if (GHERKIN_RULE_TYPE (rule) == GHERKIN_RULE_CELL)
    g_print (" |Value: '%s' ", GHERKIN_CELL_VALUE (rule));

  g_print ("\n");
  return FALSE;
}

static void
gherkin_ast_builder_serialize_results (GherkinFormatter * formatter)
{
  GherkinAstBuilder *self = GHERKIN_AST_BUILDER (formatter);

  g_node_traverse (priv (self)->stack.head->data,
      G_PRE_ORDER, G_TRAVERSE_ALL, -1, _print_stack, NULL);
}

static GNode *
_current_node (GherkinAstBuilder * self)
{
  return priv (self)->stack.head ? priv (self)->stack.head->data : NULL;
}

static GherkinRule *
gherkin_ast_builder_start_rule (GherkinFormatter *formatter,
    GherkinRuleType rule_type)
{
  GherkinAstBuilder *self = GHERKIN_AST_BUILDER (formatter);

  GherkinRule *rule = gherkin_rule_new (rule_type);

  if (debug_ast_builder)
    DEBUG (AST_BUILDER, "Starting rule: %s", gherkin_rule_type_get_name (rule_type));
  g_queue_push_head (&priv (self)->stack, g_node_new (rule));

  return rule;
}

static void
gherkin_ast_builder_end_rule (GherkinFormatter *formatter,
    GherkinRuleType rule_type)
{
  GNode *node;
  GherkinAstBuilder *self = GHERKIN_AST_BUILDER (formatter);

  g_return_if_fail (g_list_length (priv (self)->stack.head) > 1);

  node = g_queue_pop_head (&priv (self)->stack);

  if (debug_ast_builder)
    DEBUG (AST_BUILDER, "Ending rule: %s", gherkin_rule_type_get_name (rule_type));
  g_node_append (priv (self)->stack.head->data, node);
}

static void
gherkin_ast_builder_init (GherkinAstBuilder * self)
{
  g_queue_init (&priv (self)->stack);

  g_queue_push_head (&priv (self)->stack,
      g_node_new (gherkin_rule_new (GHERKIN_RULE_Feature)));
}

static void
gherkin_ast_builder_class_init (GherkinAstBuilderClass * klass)
{
  GherkinFormatterClass *formatter_class = GHERKIN_FORMATTER_CLASS (klass);

  formatter_class->serialize_results = gherkin_ast_builder_serialize_results;
  formatter_class->start_rule = gherkin_ast_builder_start_rule;
  formatter_class->end_rule = gherkin_ast_builder_end_rule;
  return;
}

GNode *
gherkin_ast_builder_get_root (GherkinAstBuilder * self)
{
  return _current_node (self);
}

GherkinAstBuilder *
gherkin_ast_builder_new (void)
{
  return g_object_new (GHERKIN_TYPE_AST_BUILDER, NULL);
}
