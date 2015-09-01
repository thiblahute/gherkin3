#include <glib.h>
#include "../gherkin/gherkinscanner.h"
#include "../gherkin/gherkinscanner.h"
#include "../gherkin/gherkinparser.h"
#include "../gherkin/gherkintokenformatter.h"

#include <json-glib/json-glib.h>
#include <json-glib/json-gobject.h>

int
main (int argc, char *argv[])
{
  gsize size;
  gchar *content;
  GScanner *scanner;
  GherkinParser *parser;
  GherkinFormatter *formatter;

  GError *err = NULL;

  if (argc > 1 && g_file_get_contents (argv[1], &content, &size, &err)) {
    scanner = gherkin_scanner_new (content, size, argv[1]);
  } else {
    g_printerr ("Error loading file '%s': %s\n",
        argv[1], err ? err->message : "Unkown reasons");

    return 1;
  }

  parser = gherkin_parser_new (scanner, NULL);
  if (!gherkin_parser_parse (parser)) {
    g_printerr ("Could not parser %s", argv[1]);

    return 1;
  }

  /* finsish parsing */
  g_scanner_destroy (scanner);

  return 0;
}
