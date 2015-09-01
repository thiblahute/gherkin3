#include <string.h>
#include <glib/gstdio.h>

#include "gherkindebug.h"
#include "gherkinscanner.h"
#include "gherkinparser.h"
#include "gherkinastbuilder.h"

typedef enum
{
  GHERKIN_PARSER_MODE_NONE,
  GHERKIN_PARSER_MODE_TABLE,
  GHERKIN_PARSER_MODE_TAGS,
  GHERKIN_PARSER_MODE_STEP
} GherkinParserMode;

typedef struct
{
  GScanner *scanner;

  gint indent_level;
  GherkinToken symbol;
  gint symbol_line;

  gchar **lines;
  JsonBuilder *builder;

  gboolean scenarios;
  GherkinParserMode mode;

  GArray *scenario_tags;

  gint last_row_line;
  gint last_row_column;

  GherkinAstBuilder *ast_builder;

} GherkinParserPrivate;

struct _GherkinParser
{
  GObject parent;
  GherkinParserPrivate *priv;
};

/*  *INDENT-OFF* */
G_DEFINE_TYPE_WITH_PRIVATE (GherkinParser, gherkin_parser, G_TYPE_OBJECT)
/*  *INDENT-ON* */

static void
_set_current_indent (GherkinParser * self)
{
  gint i = 0;
  gchar *line;

  line = self->priv->lines[self->priv->symbol_line];
  while (line[i++] == ' ')
    continue;

  self->priv->indent_level = i;
}

static void
_set_tag (GherkinParser * self, gint column, guint line, gchar * name)
{
  GherkinRule * tag;

  tag = gherkin_ast_builder_start_rule (self->priv->ast_builder, GHERKIN_RULE_TAG);
  gherkin_rule_set_location (tag, line, column);
  gherkin_rule_set_name (tag, name);
  gherkin_ast_builder_end_rule (self->priv->ast_builder, GHERKIN_RULE_TAG);
}

static void
_end_cells (GherkinParser * self)
{
  GherkinParserPrivate *priv = self->priv;

  gherkin_ast_builder_end_rule (self->priv->ast_builder, GHERKIN_RULE_CELLS);

  priv->last_row_line = -1;
  priv->last_row_column = -1;
}


static void
_maybe_end_mode (GherkinParser * self, guint token)
{
  GherkinParserPrivate *priv = self->priv;

  if (priv->mode == GHERKIN_PARSER_MODE_TAGS) {
    gherkin_ast_builder_end_rule (self->priv->ast_builder, GHERKIN_RULE_TAGS);

    priv->mode = GHERKIN_PARSER_MODE_NONE;
  } else if (token != GHERKIN_TOKEN_TABLE
      && priv->mode == GHERKIN_PARSER_MODE_TABLE) {
    priv->mode = GHERKIN_PARSER_MODE_NONE;
    _end_cells (self);
    gherkin_ast_builder_end_rule (self->priv->ast_builder, GHERKIN_RULE_ROWS);
    gherkin_ast_builder_end_rule (self->priv->ast_builder, GHERKIN_RULE_ARGUMENTS);
    gherkin_ast_builder_end_rule (self->priv->ast_builder, GHERKIN_RULE_STEP);
  } else if (priv->mode == GHERKIN_PARSER_MODE_STEP
      && token != GHERKIN_TOKEN_TABLE) {
    gherkin_ast_builder_end_rule (self->priv->ast_builder, GHERKIN_RULE_STEP);
  }
}

static guint
_parse_feature (GherkinParser * self)
{
  guint i;
  gchar *tmp;
  GherkinRule *rule;
  GherkinParserPrivate *priv = self->priv;
  GString *description = g_string_new (NULL);

  rule = gherkin_ast_builder_start_rule (priv->ast_builder, GHERKIN_RULE_FEATURE);
  _maybe_end_mode (self, GHERKIN_TOKEN_FEATURE);

  if (priv->lines[priv->symbol_line][strlen ("Feature")] != ':')
    return ':';

  _set_current_indent (self);

  gherkin_ast_builder_start_rule (self->priv->ast_builder, GHERKIN_RULE_COMMENT);
  gherkin_ast_builder_end_rule (self->priv->ast_builder, GHERKIN_RULE_COMMENT);

  if (!gherkin_scanner_peek_next_token (priv->scanner)) {
    for (i = priv->symbol_line + 1; i < priv->scanner->next_line + 1; i++) {
      g_string_append (description, priv->lines[i]);
    }

    gherkin_feature_set_description (rule, g_strstrip (description->str));
  }

  gherkin_rule_set_keyword (rule, "Feature");
  gherkin_feature_set_language (rule, "en");
  gherkin_rule_set_location (rule, priv->symbol_line, priv->indent_level);

  tmp = g_strdup (&priv->lines[priv->symbol_line][priv->indent_level - 1 +
          strlen ("Feature:")]);
  gherkin_rule_set_name (rule, g_strstrip (tmp));
  g_free (tmp);

  return G_TOKEN_NONE;
}

static void
_end_scenario (GherkinParser * self)
{
  guint i;
  GherkinParserPrivate *priv = self->priv;

  gherkin_ast_builder_end_rule (self->priv->ast_builder, GHERKIN_RULE_STEPS);
  gherkin_ast_builder_start_rule (self->priv->ast_builder, GHERKIN_RULE_TAGS);
  for (i = 0; i < priv->scenario_tags->len; i++) {
    GherkinRule *tag = &g_array_index (priv->scenario_tags, GherkinRule, i);

    _set_tag (self, GHERKIN_RULE_COLUMN(tag), GHERKIN_RULE_LINE (tag),
        GHERKIN_RULE_NAME (tag));
  }
  gherkin_ast_builder_end_rule (self->priv->ast_builder, GHERKIN_RULE_TAGS);
  gherkin_ast_builder_end_rule (self->priv->ast_builder, GHERKIN_RULE_SCENARIO);
}

static guint
_parse_scenario (GherkinParser * self)
{
  GherkinRule *scenario;
  GherkinParserPrivate *priv = self->priv;

  _maybe_end_mode (self, GHERKIN_TOKEN_SCENARIO);

  if (!priv->scenarios) {
    priv->scenario_tags = g_array_new (TRUE, TRUE, sizeof (GherkinRule));

    gherkin_ast_builder_start_rule (self->priv->ast_builder, GHERKIN_RULE_SCENARIO_DEFINITIONS);

    priv->scenarios = TRUE;
  } else {
    _end_scenario (self);
  }
  priv->mode = GHERKIN_PARSER_MODE_NONE;

  _set_current_indent (self);

  scenario = gherkin_ast_builder_start_rule (self->priv->ast_builder, GHERKIN_RULE_SCENARIO);
  gherkin_rule_set_keyword (scenario, "Scenario");

  gherkin_rule_set_location (scenario, priv->symbol_line, priv->indent_level);
  gherkin_rule_set_name (scenario,
      &priv->lines[priv->symbol_line][priv->indent_level - 1 + strlen ("Scenario:")]);

  gherkin_ast_builder_start_rule (self->priv->ast_builder, GHERKIN_RULE_STEPS);

  return G_TOKEN_NONE;
}

static guint
_parse_table (GherkinParser * self)
{
  GherkinRule *args, *cell;
  GString *value = g_string_new (NULL);
  guint i = -1;
  GherkinParserPrivate *priv = self->priv;

  _maybe_end_mode (self, GHERKIN_TOKEN_TABLE);

  if (!gherkin_scanner_peek_next_token (priv->scanner))
    return G_TOKEN_NONE;

  if (priv->scanner->line != priv->scanner->next_line)
    return G_TOKEN_NONE;

  if ((guint) priv->scanner->next_token == GHERKIN_TOKEN_TABLE) {
    gchar *line = priv->lines[priv->scanner->line - 1];

    for (i = priv->scanner->position - 1; line[i] != '|'; i--) {
      g_string_prepend_c (value, line[i]);
    }

    i++;
    while (line[i] == ' ') {
      i++;
    }
  }

  if (priv->mode != GHERKIN_PARSER_MODE_TABLE) {
    priv->mode = GHERKIN_PARSER_MODE_TABLE;


    args = gherkin_ast_builder_start_rule (self->priv->ast_builder, GHERKIN_RULE_ARGUMENTS);
    gherkin_rule_set_location (args, priv->symbol_line, priv->scanner->position - 3);

    gherkin_ast_builder_start_rule (self->priv->ast_builder, GHERKIN_RULE_ROWS);
  }

  if ((guint) priv->last_row_line != priv->scanner->line) {
    if (priv->last_row_line > 0)
      _end_cells (self);

    gherkin_ast_builder_start_rule (self->priv->ast_builder, GHERKIN_RULE_CELLS);
    priv->last_row_line = priv->scanner->line;
    priv->last_row_column = i + 1;
  }

  cell = gherkin_ast_builder_start_rule (self->priv->ast_builder, GHERKIN_RULE_CELL);
  gherkin_rule_set_location (cell, priv->symbol_line, i);

  gherkin_cell_set_value (cell, g_strstrip (value->str));
  gherkin_ast_builder_end_rule (self->priv->ast_builder, GHERKIN_RULE_CELL);

  g_string_free (value, TRUE);

  return G_TOKEN_NONE;
}

static guint
_parse_step (GherkinParser * self, const gchar * step_name, GherkinToken token)
{
  gchar *tmp;
  GherkinParserPrivate *priv = self->priv;
  GherkinRule *step;

  _maybe_end_mode (self, token);

  if (!priv->scenarios)
    return G_TOKEN_NONE;

  _set_current_indent (self);

  step = gherkin_ast_builder_start_rule (self->priv->ast_builder, GHERKIN_RULE_STEP);

  gherkin_rule_set_keyword (step, "step_name");

  gherkin_rule_set_location (step, priv->symbol_line, priv->indent_level);

  tmp =
      g_strdup (&priv->lines[priv->symbol_line][priv->indent_level - 1 +
          strlen (step_name)]);
  gherkin_step_set_text (step, g_strstrip (tmp));


  priv->mode = GHERKIN_PARSER_MODE_STEP;

  return G_TOKEN_NONE;
}

static gboolean
_parse_token (GherkinParser * self, GherkinToken token)
{
  GherkinParserPrivate *priv = self->priv;

  priv->symbol_line = priv->scanner->line - 1;
  switch (token) {
    case GHERKIN_TOKEN_FEATURE:
      _parse_feature (self);
      break;
    case GHERKIN_TOKEN_SCENARIO:
      _parse_scenario (self);
      break;
    case GHERKIN_TOKEN_WHEN:
      _parse_step (self, "When", token);
      break;
    case GHERKIN_TOKEN_THEN:
      _parse_step (self, "Then", token);
      break;
    case GHERKIN_TOKEN_GIVEN:
      _parse_step (self, "Given ", token);
      break;
    case GHERKIN_TOKEN_AND:
      _parse_step (self, "And ", token);
      break;
    case GHERKIN_TOKEN_TABLE:
      _parse_table (self);
      break;
    case GHERKIN_TOKEN_TAG:
    {
      gint i;
      GString *tag_str = g_string_new (NULL);
      gchar *line = priv->lines[priv->scanner->line - 1];

      for (i = priv->scanner->position - 1; line[i] != ' ' && line[i] != '\0';
          i++)
        g_string_append_c (tag_str, line[i]);

      if (priv->scenarios) {
        GherkinRule tag;

        GHERKIN_RULE_LINE (&tag) = priv->scanner->line;
        GHERKIN_RULE_COLUMN (&tag) = priv->scanner->position;
        GHERKIN_RULE_NAME (&tag) = tag_str->str;

        g_array_append_val (priv->scenario_tags, tag);
      } else {
        if (priv->mode != GHERKIN_PARSER_MODE_TAGS) {

          priv->mode = GHERKIN_PARSER_MODE_TAGS;
        }

        _set_tag (self, priv->scanner->position, priv->scanner->line,
            tag_str->str);
      }

      g_string_free (tag_str, FALSE);
    }
      break;
    case GHERKIN_TOKEN_EOF:
      break;
  }

  _set_current_indent (self);

  return TRUE;
}

gboolean
gherkin_parser_parse (GherkinParser * self)
{
  GherkinToken token;
  GherkinParserPrivate *priv = self->priv;

  do {

    token = gherkin_scanner_next_token (self->priv->scanner);
    if (!_parse_token (self, token))
      return FALSE;

    gherkin_scanner_peek_next_token (self->priv->scanner);
  }
  while (!gherkin_scanner_is_ending_token (self->priv->scanner->next_token));

  _maybe_end_mode (self, G_TOKEN_NONE);

  if (priv->scenarios) {
    _end_scenario (self);
    gherkin_ast_builder_end_rule (self->priv->ast_builder, GHERKIN_RULE_SCENARIO_DEFINITIONS);
  }

  gherkin_ast_builder_end_rule (self->priv->ast_builder, GHERKIN_RULE_FEATURE);
  gherkin_ast_builder_dump_ast (priv->ast_builder);

  return TRUE;
}

GherkinParser *
gherkin_parser_new (GScanner * scanner)
{
  GherkinParser *self;

  g_return_val_if_fail (gherkin_is_scanner (scanner), NULL);
  g_return_val_if_fail (scanner->line == 1, NULL);

  self = g_object_new (GHERKIN_TYPE_PARSER, NULL);

  self->priv->scanner = scanner;
  self->priv->lines = g_strsplit (scanner->text, "\n", -1);

  return self;
}

/*  GObject vmethods implementation */
static void
_finalize (GObject * object)
{
  GherkinParserPrivate *priv = GHERKIN_PARSER (object)->priv;

  g_strfreev (priv->lines);
}

static void
gherkin_parser_class_init (GherkinParserClass * klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  object_class->finalize = _finalize;
}

static void
gherkin_parser_init (GherkinParser * self)
{
  self->priv = G_TYPE_INSTANCE_GET_PRIVATE (self, GHERKIN_TYPE_PARSER,
      GherkinParserPrivate);

  self->priv->last_row_line = -1;
  self->priv->last_row_column = -1;

  self->priv->ast_builder = gherkin_ast_builder_new ();

  return;
}