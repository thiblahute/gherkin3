#include <string.h>
#include <glib/gstdio.h>

#include "gherkindebug.h"
#include "gherkinscanner.h"
#include "gherkinparser.h"

#define BEGIN_OBJECT(name) G_STMT_START {\
  DEBUG ("--> begin %s (file line %d)", name, priv->scanner->line); \
  json_builder_begin_object (priv->builder);} G_STMT_END

#define END_OBJECT(name) G_STMT_START {\
  DEBUG ("<-- End %s (file line %d)", name, priv->scanner->line); \
  json_builder_end_object (priv->builder);} G_STMT_END

#define BEGIN_ARRAY(name) G_STMT_START {\
  DEBUG ("--> Begin Array: %s (file line %d)", name, priv->scanner->line); \
  json_builder_begin_array (priv->builder);} G_STMT_END

#define END_ARRAY(name) G_STMT_START {\
  DEBUG ("--> End Array: %s (file line %d)", name, priv->scanner->line); \
  json_builder_end_array (priv->builder);} G_STMT_END

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

} GherkinParserPrivate;

struct _GherkinParser
{
  GObject parent;
  GherkinParserPrivate *priv;
};

typedef struct
{
  guint column;
  guint line;
  char *name;
} GherkinTag;

static void
_free_tag (GherkinTag * tag)
{
  g_free (tag->name);
}

G_DEFINE_TYPE_WITH_PRIVATE (GherkinParser, gherkin_parser, G_TYPE_OBJECT)

     static void _set_current_indent (GherkinParser * self)
{
  gint i = 0;
  gchar *line;

  line = self->priv->lines[self->priv->symbol_line];
  while (line[i++] == ' ')
    continue;

  self->priv->indent_level = i;
}

static void
_set_location (GherkinParser * self, gint override_column, gint override_line)
{
  GherkinParserPrivate *priv = self->priv;
  gint column = override_column < 0 ? priv->indent_level : override_column;
  gint line = override_line < 0 ? priv->symbol_line + 1 : override_line;

  json_builder_set_member_name (priv->builder, "location"); {
    BEGIN_OBJECT ("Location"); {
      json_builder_set_member_name (priv->builder, "column");
      json_builder_add_int_value (priv->builder, column);

      json_builder_set_member_name (priv->builder, "line");
      json_builder_add_int_value (priv->builder, line);
    }
    END_OBJECT ("Location");
  }
}

static void
_set_tag (GherkinParser * self, gint column, guint line, gchar * name)
{
  GherkinParserPrivate *priv = self->priv;

  BEGIN_OBJECT ("Tag");
  _set_location (self, column, line);
  json_builder_set_member_name (priv->builder, "name");
  json_builder_add_string_value (priv->builder, name);
  json_builder_set_member_name (priv->builder, "type");
  json_builder_add_string_value (priv->builder, "Tag");
  END_OBJECT ("Tag");

}

static void
_end_cells (GherkinParser * self)
{
  GherkinParserPrivate *priv = self->priv;

  END_ARRAY ("Cells");
  _set_location (self, priv->last_row_column, priv->last_row_line);
  json_builder_set_member_name (priv->builder, "type");
  json_builder_add_string_value (priv->builder, "TableRow");
  END_OBJECT ("Cells");

  priv->last_row_line = -1;
  priv->last_row_column = -1;
}


static void
_maybe_end_mode (GherkinParser * self, guint token)
{
  GherkinParserPrivate *priv = self->priv;

  if (priv->mode == GHERKIN_PARSER_MODE_TAGS) {
    END_ARRAY ("Tags");

    priv->mode = GHERKIN_PARSER_MODE_NONE;
  } else if (token != GHERKIN_TOKEN_TABLE
      && priv->mode == GHERKIN_PARSER_MODE_TABLE) {
    priv->mode = GHERKIN_PARSER_MODE_NONE;
    _end_cells (self);
    END_ARRAY ("Rows");
    json_builder_set_member_name (priv->builder, "type");
    json_builder_add_string_value (priv->builder, "DataTable");
    END_OBJECT ("Arguments");
    END_OBJECT ("Step");
  } else if (priv->mode == GHERKIN_PARSER_MODE_STEP
      && token != GHERKIN_TOKEN_TABLE) {
    END_OBJECT ("Step");
  }
}

static guint
_parse_feature (GherkinParser * self)
{
  guint i;
  gchar *tmp;
  GherkinParserPrivate *priv = self->priv;
  GString *description = g_string_new (NULL);

  _maybe_end_mode (self, GHERKIN_TOKEN_FEATURE);

  if (priv->lines[priv->symbol_line][strlen ("Feature")] != ':')
    return ':';

  _set_current_indent (self);

  json_builder_set_member_name (priv->builder, "comments"); {
    BEGIN_ARRAY ("comments");
    END_ARRAY ("comments");
  }

  if (!gherkin_scanner_peek_next_token (priv->scanner)) {
    json_builder_set_member_name (priv->builder, "description"); {
      for (i = priv->symbol_line + 1; i < priv->scanner->next_line + 1; i++) {
        g_string_append (description, priv->lines[i]);
      }
      json_builder_add_string_value (priv->builder,
          g_strstrip (description->str));
    }
  }

  json_builder_set_member_name (priv->builder, "keyword");
  json_builder_add_string_value (priv->builder, "Feature");

  json_builder_set_member_name (priv->builder, "language");
  json_builder_add_string_value (priv->builder, "en");  /* FIXME! */

  _set_location (self, -1, -1);

  json_builder_set_member_name (priv->builder, "name");
  tmp =
      g_strdup (&priv->lines[priv->symbol_line][priv->indent_level - 1 +
          strlen ("Feature:")]);
  json_builder_add_string_value (priv->builder, g_strstrip (tmp));
  g_free (tmp);


  return G_TOKEN_NONE;
}

static void
_end_scenario (GherkinParser * self)
{
  guint i;
  GherkinParserPrivate *priv = self->priv;

  END_ARRAY ("Steps");
  json_builder_set_member_name (priv->builder, "tags");
  BEGIN_ARRAY ("tags");
  for (i = 0; i < priv->scenario_tags->len; i++) {
    GherkinTag *tag = &g_array_index (priv->scenario_tags, GherkinTag, i);

    _set_tag (self, tag->column, tag->line, tag->name);
  }
  END_ARRAY ("tags");

  json_builder_set_member_name (priv->builder, "type");
  json_builder_add_string_value (priv->builder, "Scenario");
  END_OBJECT ("Scenario");
}

static guint
_parse_scenario (GherkinParser * self)
{
  GherkinParserPrivate *priv = self->priv;

  _maybe_end_mode (self, GHERKIN_TOKEN_SCENARIO);

  if (!priv->scenarios) {
    priv->scenario_tags = g_array_new (TRUE, TRUE, sizeof (GherkinTag));
    g_array_set_clear_func (priv->scenario_tags, (GDestroyNotify) _free_tag);

    json_builder_set_member_name (priv->builder, "scenarioDefinitions");
    BEGIN_ARRAY ("scenarioDefinitions");

    priv->scenarios = TRUE;
  } else {
    _end_scenario (self);
  }
  priv->mode = GHERKIN_PARSER_MODE_NONE;

  _set_current_indent (self);

  BEGIN_OBJECT ("Scenario");
  json_builder_set_member_name (priv->builder, "keyword");
  json_builder_add_string_value (priv->builder, "Scenario");

  _set_location (self, -1, -1);

  json_builder_set_member_name (priv->builder, "name");
  json_builder_add_string_value (priv->builder,
      &priv->lines[priv->symbol_line][priv->indent_level - 1 +
          strlen ("Scenario:")]);

  json_builder_set_member_name (priv->builder, "steps");
  BEGIN_ARRAY ("Steps");

  return G_TOKEN_NONE;
}

static guint
_parse_table (GherkinParser * self)
{
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

    json_builder_set_member_name (priv->builder, "argument");

    BEGIN_OBJECT ("Arguments");
    _set_location (self, priv->scanner->position - 3, -1);

    json_builder_set_member_name (priv->builder, "rows");
    BEGIN_ARRAY ("Rows");
  }

  if ((guint) priv->last_row_line != priv->scanner->line) {
    if (priv->last_row_line > 0)
      _end_cells (self);

    BEGIN_OBJECT ("Cells");
    json_builder_set_member_name (priv->builder, "cells");
    BEGIN_ARRAY ("Cells");
    priv->last_row_line = priv->scanner->line;
    priv->last_row_column = i + 1;
  }

  BEGIN_OBJECT ("TableCell");
  _set_location (self, i, -1);
  json_builder_set_member_name (priv->builder, "type");
  json_builder_add_string_value (priv->builder, "TableCell");

  json_builder_set_member_name (priv->builder, "value");
  json_builder_add_string_value (priv->builder, g_strstrip (value->str));
  END_OBJECT ("TableCell");

  g_string_free (value, TRUE);

  return G_TOKEN_NONE;
}

static guint
_parse_step (GherkinParser * self, const gchar * step_name, GherkinToken token)
{
  gchar *tmp;
  GherkinParserPrivate *priv = self->priv;

  _maybe_end_mode (self, token);

  if (!priv->scenarios)
    return G_TOKEN_NONE;

  _set_current_indent (self);

  BEGIN_OBJECT ("Step");

  json_builder_set_member_name (priv->builder, "keyword");
  json_builder_add_string_value (priv->builder, step_name);

  _set_location (self, -1, -1);

  json_builder_set_member_name (priv->builder, "text");
  tmp =
      g_strdup (&priv->lines[priv->symbol_line][priv->indent_level - 1 +
          strlen (step_name)]);
  json_builder_add_string_value (priv->builder, g_strstrip (tmp));

  json_builder_set_member_name (priv->builder, "type");
  json_builder_add_string_value (priv->builder, "Step");

  priv->mode = GHERKIN_PARSER_MODE_STEP;

  return G_TOKEN_NONE;
}

static gboolean
_parse_token (GherkinParser * self, GherkinToken token)
{
  guint res = G_TOKEN_NONE;
  gboolean new_symbol = FALSE;
  GherkinParserPrivate *priv = self->priv;

  priv->symbol_line = priv->scanner->line - 1;
  switch (token) {
    case GHERKIN_TOKEN_FEATURE:
      new_symbol = TRUE;
      res = _parse_feature (self);
      break;
    case GHERKIN_TOKEN_SCENARIO:
      res = _parse_scenario (self);
      break;
    case GHERKIN_TOKEN_WHEN:
      _parse_step (self, "When", token);
      new_symbol = TRUE;
      break;
    case GHERKIN_TOKEN_THEN:
      _parse_step (self, "Then", token);
      new_symbol = TRUE;
      break;
    case GHERKIN_TOKEN_GIVEN:
      _parse_step (self, "Given ", token);
      new_symbol = TRUE;
      break;
    case GHERKIN_TOKEN_AND:
      _parse_step (self, "And ", token);
      new_symbol = TRUE;
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
        GherkinTag tag =
            { priv->scanner->line, priv->scanner->position, tag_str->str };

        g_array_append_val (priv->scenario_tags, tag);
      } else {
        if (priv->mode != GHERKIN_PARSER_MODE_TAGS) {

          json_builder_set_member_name (priv->builder, "tags");
          BEGIN_ARRAY ("Tags");
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

  if (gherkin_scanner_is_ending_token (g_scanner_peek_next_token (priv->
              scanner))) {
    _maybe_end_mode (self, G_TOKEN_NONE);

    if (priv->scenarios) {
      _end_scenario (self);
      END_ARRAY ("ScenarioDefinitionsn");
    }

    END_OBJECT ("Feature");
  }

  return TRUE;
}

gboolean
gherkin_parser_parse (GherkinParser * self)
{
  GherkinToken token;
  GherkinParserPrivate *priv = self->priv;

  BEGIN_OBJECT ("Feature");
  do {

    token = gherkin_scanner_next_token (self->priv->scanner);
    if (!_parse_token (self, token))
      return FALSE;

    gherkin_scanner_peek_next_token (self->priv->scanner);
  }
  while (!gherkin_scanner_is_ending_token (self->priv->scanner->next_token));

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

JsonNode *
gherkin_parser_get_ast (GherkinParser * self)
{
  GherkinParserPrivate *priv = self->priv;

  DEBUG ("%p Builder is %s", self, G_OBJECT_TYPE_NAME (self->priv->builder));
  return json_builder_get_root (priv->builder);
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
  return;
}

static void
gherkin_parser_init (GherkinParser * self)
{
  self->priv = G_TYPE_INSTANCE_GET_PRIVATE (self, GHERKIN_TYPE_PARSER,
      GherkinParserPrivate);

  self->priv->builder = json_builder_new ();
  DEBUG ("%p Builder is %s", self, G_OBJECT_TYPE_NAME (self->priv->builder));
  self->priv->last_row_line = -1;
  self->priv->last_row_column = -1;

  return;
}
