#ifndef __GHERKINATYPES_H__
#define __GHERKINATYPES_H__

#include <glib.h>

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

typedef enum {
  GHERKIN_RULE_FEATURE,
  GHERKIN_RULE_SCENARIO,
  GHERKIN_RULE_TAG,
  GHERKIN_RULE_TAGS,
  GHERKIN_RULE_STEP,
  GHERKIN_RULE_STEPS,
  GHERKIN_RULE_COMMENTS,
  GHERKIN_RULE_SCENARIO_DEFINITIONS,
  GHERKIN_RULE_ARGUMENTS,
  GHERKIN_RULE_TABLE,
  GHERKIN_RULE_ROWS,
  GHERKIN_RULE_CELLS,
  GHERKIN_RULE_CELL,
} GherkinRuleType;

const gchar * gherkin_rule_type_get_name (GherkinRuleType rule_type);

#define GHERKIN_RULE_TYPE(rule)     (((GherkinRuleCommon*) (rule))->rule_type)
#define GHERKIN_RULE_KEYWORD(rule)  (((GherkinRuleCommon*) (rule))->keyword)
#define GHERKIN_RULE_COLUMN(rule)   (((GherkinRuleCommon*) (rule))->column)
#define GHERKIN_RULE_LINE(rule)     (((GherkinRuleCommon*) (rule))->line)
#define GHERKIN_RULE_NAME(rule)     (((GherkinRuleCommon*) (rule))->name)

typedef struct _GherkinRuleCommon
{
  GherkinRuleType rule_type;

  gchar *keyword;

  gint column; /* -1 if unknown */
  gint line; /* -1 if unknown */

  gchar *name;
} GherkinRuleCommon;

#define GHERKIN_FEATURE_DESCRIPTION(feature)    (((GherkinFeature*) feature)->description)
#define GHERKIN_FEATURE_LANGUAGE(feature)       (((GherkinFeature*) feature)->language)
typedef struct _GherkinFeature
{
  GherkinRuleCommon parent;

  gchar *description;
  gchar *language;
} GherkinFeature;

#define GHERKIN_STEP_TEXT(feature)       (((GherkinStep*) feature)->text)
typedef struct _GherkinStep
{
  GherkinRuleCommon parent;

  gchar *text;
} GherkinStep;

#define GHERKIN_CELL_VALUE(cell)       (((GherkinCell*) cell)->value)
typedef struct _GherkinCell
{
  GherkinRuleCommon parent;

  gchar *value;
} GherkinCell;

typedef union _GherkinRule
{
    GherkinFeature feature;
    GherkinRuleCommon scenario;
    GherkinStep step;
    GherkinCell cell;
} GherkinRule;

GherkinRule * gherkin_rule_new (GherkinRuleType rule_type);
void gherkin_rule_free (GherkinRule *rule);

void gherkin_rule_set_keyword (GherkinRule *rule, const gchar *keyword);
void gherkin_rule_set_name (GherkinRule *rule, const gchar *name);
void gherkin_feature_set_language (GherkinRule *rule, const gchar *name);
void gherkin_feature_set_description (GherkinRule *rule, const gchar *name);
void gherkin_step_set_text (GherkinRule *rule, const gchar *text);
void gherkin_rule_set_location (GherkinRule * rule, gint line, gint column);
void gherkin_cell_set_value (GherkinRule * rule, gchar *value);

G_END_DECLS

#endif /* __GHERKINATYPES_H__ */ 
