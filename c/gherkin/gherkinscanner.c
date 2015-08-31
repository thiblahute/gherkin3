/* #include <glib.h>
 */
#include <string.h>
#include <glib/gstdio.h>
#include "gherkinscanner.h"
#include "gherkindebug.h"

/* define enumeration values to be returned for specific symbols */
/* symbol array */
typedef struct
{
  gchar *symbol_name;
  guint symbol_token;
} GherkinSymbols;

static const GherkinSymbols symbols[] = {{
  "Scenario", GHERKIN_TOKEN_SCENARIO,}, {
  "Feature", GHERKIN_TOKEN_FEATURE,}, {
  "When", GHERKIN_TOKEN_WHEN,}, {
  "Given", GHERKIN_TOKEN_GIVEN,}, {
  "Then", GHERKIN_TOKEN_THEN,}, {
  "|", GHERKIN_TOKEN_TABLE,}, {
  "And", GHERKIN_TOKEN_AND,}, {
  "@", GHERKIN_TOKEN_TAG,}, {
  NULL, 0,
  },
};

static gboolean
_is_gherkin_token (guint token)
{
  gint i;

  for (i = 0; symbols[i].symbol_name; i++)
    if (symbols[i].symbol_token == token)
      return TRUE;

  return FALSE;
}

/* API */

GherkinToken
gherkin_scanner_next_token (GScanner * scanner)
{
  while (!_is_gherkin_token (g_scanner_get_next_token (scanner)) &&
      !gherkin_scanner_is_ending_token (scanner->token))
    continue;

  return scanner->token;
}
GScanner *
gherkin_scanner_new (gchar * input_text, guint text_length,
    gchar *input_name)
{
  GScanner *scanner;
  gint i;

  g_return_val_if_fail (input_text, NULL);

  scanner = g_scanner_new (NULL);

  g_datalist_set_data (&scanner->qdata, "GherkinScanner",
      GINT_TO_POINTER (TRUE));

  /* feed in the text */
  g_scanner_input_text (scanner, input_text, text_length);

  /* convert non-floats (octal values, hex values...) to G_TOKEN_INT */
  scanner->config->numbers_2_int = TRUE;
  /* convert G_TOKEN_INT to G_TOKEN_FLOAT */
  scanner->config->int_2_float = TRUE;
  /* don't return G_TOKEN_SYMBOL, but the symbol's value */
  scanner->config->symbol_2_token = TRUE;
  scanner->config->case_sensitive = TRUE;

  /* give the error handler an idea on how the input is named */
  scanner->input_name = input_name;

  /* load symbols into the scanner */
  for (i = 0; symbols[i].symbol_name; i++) {
    g_scanner_scope_add_symbol (scanner, 0,
        symbols[i].symbol_name, GINT_TO_POINTER (symbols[i].symbol_token));
  }

  return scanner;
}

gboolean
gherkin_is_scanner (GScanner *scanner)
{
  return GPOINTER_TO_INT(g_datalist_get_data (&scanner->qdata,
      "GherkinScanner"));
}

gboolean
gherkin_scanner_peek_next_token (GScanner * scanner)
{
  while (!_is_gherkin_token (g_scanner_peek_next_token (scanner)) &&
      !gherkin_scanner_is_ending_token (scanner->next_token)) {
    g_scanner_get_next_token (scanner);
  }

  if (gherkin_scanner_is_ending_token (scanner->next_token))
    return FALSE;

  return TRUE;
}

gboolean
gherkin_scanner_is_ending_token (guint token)
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
