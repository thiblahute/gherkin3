#include "gherkintypes.generated.h"
#include "gherkintypes.h"

GherkinRule *
gherkin_rule_new (GherkinRuleType rule_type)
{
  GherkinRuleCommon *rule = g_malloc0 (sizeof (GherkinRule));

  rule->line = -1;
  rule->column = -1;

  GHERKIN_RULE_TYPE (rule) = rule_type;

  return (GherkinRule *) rule;
}

void
gherkin_rule_free (GherkinRule * rule)
{
  g_free (GHERKIN_RULE_KEYWORD (rule));
  g_free (GHERKIN_RULE_NAME (rule));
  switch (GHERKIN_RULE_TYPE (rule)) {
    case GHERKIN_RULE_Feature:
      g_free (GHERKIN_FEATURE_DESCRIPTION (rule));
      g_free (GHERKIN_FEATURE_LANGUAGE (rule));
      break;
    default:
      break;
  }

  g_free (rule);
}

void
gherkin_rule_set_location (GherkinRule * rule, gint line, gint column)
{
  GHERKIN_RULE_LINE (rule) = line;
  GHERKIN_RULE_COLUMN (rule) = column;
}

void
gherkin_rule_set_keyword (GherkinRule * rule, const gchar * keyword)
{
  GHERKIN_RULE_KEYWORD (rule) = g_strdup (keyword);
}

void
gherkin_rule_set_name (GherkinRule * rule, const gchar * name)
{
  GHERKIN_RULE_NAME (rule) = g_strdup (name);
}

void
gherkin_feature_set_language (GherkinRule * rule, const gchar * language)
{
  g_return_if_fail (GHERKIN_RULE_TYPE (rule) == GHERKIN_RULE_Feature);

  GHERKIN_FEATURE_LANGUAGE (rule) = g_strdup (language);
}

void
gherkin_feature_set_description (GherkinRule * rule, const gchar * description)
{
  g_return_if_fail (GHERKIN_RULE_TYPE (rule) == GHERKIN_RULE_Feature);

  GHERKIN_FEATURE_LANGUAGE (rule) = g_strdup (description);
}

void gherkin_step_set_text (GherkinRule *rule, const gchar *text)
{
  g_return_if_fail (GHERKIN_RULE_TYPE (rule) == GHERKIN_RULE_Step);

  GHERKIN_STEP_TEXT (rule) = g_strdup (text);

}

void gherkin_cell_set_value (GherkinRule * rule, gchar *value)
{
  g_return_if_fail (GHERKIN_RULE_TYPE (rule) == GHERKIN_RULE_CELL);

  GHERKIN_CELL_VALUE (rule) = g_strdup (value);
}
