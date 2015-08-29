/* #include <glib.h>
 */
#include <string.h>
#include <glib/gstdio.h>
#include "gherkinscanner.h"
#include "gherkindebug.h"

#define BEGIN_OBJECT(name) G_STMT_START {\
  DEBUG ("--> begin %s (file line %d)", name, scanner->line); \
  json_builder_begin_object (ctx->builder);} G_STMT_END

#define END_OBJECT(name) G_STMT_START {\
  DEBUG ("<-- End %s (file line %d)", name, scanner->line); \
  json_builder_end_object (ctx->builder);} G_STMT_END

#define BEGIN_ARRAY(name) G_STMT_START {\
  DEBUG ("--> Begin Array: %s (file line %d)", name, scanner->line); \
  json_builder_begin_array (ctx->builder);} G_STMT_END

#define END_ARRAY(name) G_STMT_START {\
  DEBUG ("--> End Array: %s (file line %d)", name, scanner->line); \
  json_builder_end_array (ctx->builder);} G_STMT_END

typedef enum
{
  GHERKIN_SCANNER_MODE_NONE,
  GHERKIN_SCANNER_MODE_TABLE,
  GHERKIN_SCANNER_MODE_STEP
} GherkinScannerMode;

typedef enum {
  GHERKIN_TOKEN_SCENARIO = G_TOKEN_LAST + 1,
  GHERKIN_TOKEN_FEATURE = G_TOKEN_LAST + 2,
  GHERKIN_TOKEN_GIVEN = G_TOKEN_LAST + 3,
  GHERKIN_TOKEN_WHEN = G_TOKEN_LAST + 4,
  GHERKIN_TOKEN_THEN = G_TOKEN_LAST + 5,
  GHERKIN_TOKEN_TABLE = '|',
  GHERKIN_TOKEN_COMMENT = '#'
} GherkinToken;

typedef struct
{
  gint indent_level;
  GherkinToken symbol;
  gint symbol_line;

  gchar **lines;
  JsonBuilder *builder;

  gboolean scenarios;
  GherkinScannerMode mode;

  gint last_row_line;
  gint last_row_column;

} GherkinScannerContext;

static void
_free_context (GherkinScannerContext * ctx)
{
  g_strfreev (ctx->lines);
  g_free (ctx);
}

static GherkinScannerContext *
_new_context (gchar * text)
{
  GherkinScannerContext *ctx = g_malloc0 (sizeof (GherkinScannerContext));
  ctx->lines = g_strsplit (text, "\n", -1);
  ctx->builder = json_builder_new ();
  ctx->last_row_line = -1;
  ctx->last_row_column = -1;

  return ctx;
}

/* define enumeration values to be returned for specific symbols */
/* symbol array */
static const struct
{
  gchar *symbol_name;
  guint symbol_token;
} symbols[] = {
  {
  "Scenario", GHERKIN_TOKEN_SCENARIO,}, {
  "scenario", GHERKIN_TOKEN_SCENARIO,}, {
  "Feature", GHERKIN_TOKEN_FEATURE,}, {
  "feature", GHERKIN_TOKEN_FEATURE,}, {
  "When", GHERKIN_TOKEN_WHEN,}, {
  "when", GHERKIN_TOKEN_WHEN,}, {
  "Given", GHERKIN_TOKEN_GIVEN,}, {
  "given", GHERKIN_TOKEN_GIVEN,}, {
  "Then", GHERKIN_TOKEN_THEN,}, {
  "then", GHERKIN_TOKEN_THEN,}, {
  "|", GHERKIN_TOKEN_TABLE,}, {
NULL, 0,},}, *symbol_p = symbols;

static gboolean
_is_gherkin_token (guint token)
{
  switch (token) {
    case GHERKIN_TOKEN_FEATURE:
    case GHERKIN_TOKEN_SCENARIO:
    case GHERKIN_TOKEN_WHEN:
    case GHERKIN_TOKEN_THEN:
    case GHERKIN_TOKEN_TABLE:
    case GHERKIN_TOKEN_GIVEN:
      return TRUE;
    default:
      return FALSE;
  }
}

static gboolean
_is_ending_token (guint token)
{
  switch (token) {
    case G_TOKEN_EOF:
    case G_TOKEN_ERROR:
    case G_TOKEN_STRING:
      return TRUE;
    default:
      return FALSE;
  }
}

static void
_set_current_indent (GScanner * scanner, GherkinScannerContext * ctx)
{
  gint i = 0;
  gchar *line;

  line = ctx->lines[ctx->symbol_line];
  while (line[i++] == ' ')
    continue;

  ctx->indent_level = i;
}

static void
_set_location (GScanner * scanner, GherkinScannerContext * ctx,
    gint override_column, gint override_line)
{
  gint column = override_column < 0 ? ctx->indent_level : override_column;
  gint line = override_line < 0 ? ctx->symbol_line + 1 : override_line;

  json_builder_set_member_name (ctx->builder, "location"); {
    BEGIN_OBJECT ("Location"); {
      json_builder_set_member_name (ctx->builder, "column");
      json_builder_add_int_value (ctx->builder, column);

      json_builder_set_member_name (ctx->builder, "line");
      json_builder_add_int_value (ctx->builder, line);
    }
    END_OBJECT ("Location");
  }
}

static void
_end_cells (GScanner * scanner, GherkinScannerContext * ctx)
{
  END_ARRAY ("Cells");
  _set_location (scanner, ctx, ctx->last_row_column, ctx->last_row_line);
  json_builder_set_member_name (ctx->builder, "type");
  json_builder_add_string_value (ctx->builder, "TableRow");
  END_OBJECT ("Cells");

  ctx->last_row_line = -1;
  ctx->last_row_column = -1;
}


static void
_maybe_end_mode (GScanner * scanner, GherkinScannerContext * ctx, guint token)
{
  if (token != GHERKIN_TOKEN_TABLE && ctx->mode == GHERKIN_SCANNER_MODE_TABLE) {
    ctx->mode = GHERKIN_SCANNER_MODE_NONE;
    _end_cells (scanner, ctx);
    END_ARRAY ("Rows");
    END_OBJECT ("Arguments");
    END_OBJECT ("Step");
  } else if (ctx->mode == GHERKIN_SCANNER_MODE_STEP
      && token != GHERKIN_TOKEN_TABLE) {
    END_OBJECT ("Step");
  }

}

static gboolean
_peek_next_gherkin_token (GScanner * scanner)
{
  while (!_is_gherkin_token (g_scanner_peek_next_token (scanner)) &&
      !_is_ending_token (scanner->next_token)) {
    g_scanner_get_next_token (scanner);
  }

  if (_is_ending_token (scanner->next_token))
    return FALSE;

  return TRUE;
}

static guint
_parse_feature (GScanner * scanner, GherkinScannerContext * ctx)
{
  guint i;
  gchar *tmp;
  GString *description = g_string_new (NULL);

  _maybe_end_mode (scanner, ctx, GHERKIN_TOKEN_FEATURE);

  if (ctx->lines[ctx->symbol_line][strlen ("Feature")] != ':')
    return ':';

  _set_current_indent (scanner, ctx);

  BEGIN_OBJECT ("Feature");

  json_builder_set_member_name (ctx->builder, "comments"); {
    BEGIN_ARRAY ("comments");
    END_ARRAY ("comments");
  }

  json_builder_set_member_name (ctx->builder, "description"); {
    if (!_peek_next_gherkin_token (scanner)) {
      for (i = ctx->symbol_line + 1; i < scanner->next_line + 1; i++) {
        g_string_append (description, ctx->lines[i]);
      }
    }

    json_builder_add_string_value (ctx->builder, g_strstrip (description->str));
  }

  json_builder_set_member_name (ctx->builder, "keyword");
  json_builder_add_string_value (ctx->builder, "Feature");

  json_builder_set_member_name (ctx->builder, "language");
  json_builder_add_string_value (ctx->builder, "en");   /* FIXME! */

  _set_location (scanner, ctx, -1, -1);

  json_builder_set_member_name (ctx->builder, "name");
  tmp =
      g_strdup (&ctx->lines[ctx->symbol_line][ctx->indent_level - 1 +
          strlen ("Feature:")]);
  json_builder_add_string_value (ctx->builder, g_strstrip (tmp));
  g_free (tmp);


  return G_TOKEN_NONE;
}

static guint
_parse_scenario (GScanner * scanner, GherkinScannerContext * ctx)
{
  _maybe_end_mode (scanner, ctx, GHERKIN_TOKEN_SCENARIO);

  ctx->mode = GHERKIN_SCANNER_MODE_NONE;
  if (!ctx->scenarios) {
    json_builder_set_member_name (ctx->builder, "scenarioDefinitions");
    BEGIN_ARRAY ("scenarioDefinitions");

    ctx->scenarios = TRUE;
  } else {
    END_ARRAY ("Steps");
    END_OBJECT ("Scenario");
  }

  _set_current_indent (scanner, ctx);

  BEGIN_OBJECT ("Scenario");
  json_builder_set_member_name (ctx->builder, "keyword");
  json_builder_add_string_value (ctx->builder, "Scenario");

  _set_location (scanner, ctx, -1, -1);

  json_builder_set_member_name (ctx->builder, "name");
  json_builder_add_string_value (ctx->builder,
      &ctx->lines[ctx->symbol_line][ctx->indent_level - 1 +
          strlen ("Scenario:")]);

  json_builder_set_member_name (ctx->builder, "steps");
  BEGIN_ARRAY ("Steps");

  return G_TOKEN_NONE;
}

static guint
_parse_table (GScanner * scanner, GherkinScannerContext * ctx)
{
  GString *value = g_string_new (NULL);
  guint i = -1;

  _maybe_end_mode (scanner, ctx, GHERKIN_TOKEN_TABLE);

  if (!_peek_next_gherkin_token (scanner))
    return G_TOKEN_NONE;

  if (scanner->line != scanner->next_line)
    return G_TOKEN_NONE;

  if ((guint) scanner->next_token == GHERKIN_TOKEN_TABLE) {
    gchar *line = ctx->lines[scanner->line - 1];

    for (i = scanner->position; line[i] != '|'; i--)
      g_string_prepend_c (value, line[i]);
  }

  if (ctx->mode != GHERKIN_SCANNER_MODE_TABLE) {
    ctx->mode = GHERKIN_SCANNER_MODE_TABLE;

    json_builder_set_member_name (ctx->builder, "arguments");

    BEGIN_OBJECT ("Arguments");
    _set_location (scanner, ctx, -1, -1);

    json_builder_set_member_name (ctx->builder, "rows");
    BEGIN_ARRAY ("Rows");
  }

  if ((guint) ctx->last_row_line != scanner->line) {
    if (ctx->last_row_line > 0)
      _end_cells (scanner, ctx);

    BEGIN_OBJECT ("Cells");
    json_builder_set_member_name (ctx->builder, "cells");
    BEGIN_ARRAY ("Cells");
    ctx->last_row_line = scanner->line;
    ctx->last_row_column = i + 1;
  }

  BEGIN_OBJECT ("TableCell");
  _set_location (scanner, ctx, i + 3, -1);
  json_builder_set_member_name (ctx->builder, "type");
  json_builder_add_string_value (ctx->builder, "TableCell");

  json_builder_set_member_name (ctx->builder, "value");
  json_builder_add_string_value (ctx->builder, g_strstrip (value->str));
  END_OBJECT ("TableCell");

  g_string_free (value, TRUE);

  return G_TOKEN_NONE;
}

static guint
_parse_step (GScanner * scanner, GherkinScannerContext * ctx,
    const gchar * step_name, GherkinToken token)
{
  gchar *tmp;
  _maybe_end_mode (scanner, ctx, token);

  if (!ctx->scenarios)
    return G_TOKEN_NONE;

  _set_current_indent (scanner, ctx);

  BEGIN_OBJECT ("Step");

  json_builder_set_member_name (ctx->builder, "keyword");
  json_builder_add_string_value (ctx->builder, step_name);

  _set_location (scanner, ctx, -1, -1);

  json_builder_set_member_name (ctx->builder, "text");
  tmp =
      g_strdup (&ctx->lines[ctx->symbol_line][ctx->indent_level - 1 +
          strlen (step_name)]);
  json_builder_add_string_value (ctx->builder, g_strstrip (tmp));

  json_builder_set_member_name (ctx->builder, "type");
  json_builder_add_string_value (ctx->builder, "Step");

  ctx->mode = GHERKIN_SCANNER_MODE_STEP;

  return G_TOKEN_NONE;
}

guint
gherkin_scanner_parse_symbol (GScanner * scanner)
{
  guint res = G_TOKEN_NONE;
  gboolean new_symbol = FALSE;
  GherkinScannerContext *ctx = g_datalist_get_data (&scanner->qdata,
      "GherkinScannerContext");

  g_scanner_get_next_token (scanner);
  ctx->symbol_line = scanner->line - 1;
  switch ((GherkinToken) scanner->token) {
    case GHERKIN_TOKEN_FEATURE:
      new_symbol = TRUE;
      res = _parse_feature (scanner, ctx);
      break;
    case GHERKIN_TOKEN_SCENARIO:
      res = _parse_scenario (scanner, ctx);
      break;
    case GHERKIN_TOKEN_WHEN:
      _parse_step (scanner, ctx, "When", (GherkinToken) scanner->token);
      new_symbol = TRUE;
      break;
    case GHERKIN_TOKEN_THEN:
      _parse_step (scanner, ctx, "Then", (GherkinToken) scanner->token);
      new_symbol = TRUE;
      break;
    case GHERKIN_TOKEN_GIVEN:
      _parse_step (scanner, ctx, "Given", (GherkinToken) scanner->token);
      new_symbol = TRUE;
      break;
    case GHERKIN_TOKEN_TABLE:
      _parse_table (scanner, ctx);
      break;
  }

  if (new_symbol)
    _set_current_indent (scanner, ctx);

  if (_is_ending_token (g_scanner_peek_next_token (scanner))) {
    _maybe_end_mode (scanner, ctx, G_TOKEN_NONE);

    if (ctx->scenarios) {
      END_ARRAY ("Steps");
      END_OBJECT ("Scenario");
      END_ARRAY ("ScenarioDefinitionsn");
    }

    END_OBJECT ("Feature");
  }

  return res;
}

GScanner *
gherkin_scanner_new (gchar * input_text, guint text_length)
{
  GScanner *scanner;
  GherkinScannerContext *ctx;

  g_return_val_if_fail (input_text, NULL);


  scanner = g_scanner_new (NULL);

  ctx = _new_context (input_text);
  g_datalist_set_data_full (&scanner->qdata, "GherkinScannerContext",
      ctx, (GDestroyNotify) _free_context);

  /* feed in the text */
  g_scanner_input_text (scanner, input_text, text_length);

  /* convert non-floats (octal values, hex values...) to G_TOKEN_INT */
  scanner->config->numbers_2_int = TRUE;
  /* convert G_TOKEN_INT to G_TOKEN_FLOAT */
  scanner->config->int_2_float = TRUE;
  /* don't return G_TOKEN_SYMBOL, but the symbol's value */
  scanner->config->symbol_2_token = TRUE;

  /* load symbols into the scanner */
  while (symbol_p->symbol_name) {
    g_scanner_scope_add_symbol (scanner, 0,
        symbol_p->symbol_name, GINT_TO_POINTER (symbol_p->symbol_token));

    symbol_p++;
  }

  return scanner;
}

gboolean
gherkin_scanner_parse (GScanner * scanner)
{
  guint expected_token;
  GherkinScannerContext *ctx =
      g_datalist_get_data (&scanner->qdata, "GherkinScannerContext");

  g_return_val_if_fail (ctx, FALSE);

  do {
    expected_token = gherkin_scanner_parse_symbol (scanner);

    g_scanner_peek_next_token (scanner);
  }
  while (expected_token == G_TOKEN_NONE &&
      !_is_ending_token (scanner->next_token));

  /* give an error message upon syntax errors */
  if (expected_token != G_TOKEN_NONE) {
    g_scanner_unexp_token (scanner, expected_token, NULL, "symbol", NULL, NULL,
        TRUE);

    return FALSE;
  }

  return TRUE;
}

JsonNode *
gherkin_scanner_get_root (GScanner * scanner)
{
  GherkinScannerContext *ctx = g_datalist_get_data (&scanner->qdata,
      "GherkinScannerContext");

  return json_builder_get_root (ctx->builder);
}
