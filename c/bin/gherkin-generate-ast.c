#include <glib.h>
#include <gherkinscanner.h>
#include <gherkinparser.h>

#include <json-glib/json-glib.h>
#include <json-glib/json-gobject.h>

int
main (int argc, char *argv[])
{
  gsize size;
  gchar *content;
  JsonNode *root;
  JsonGenerator *gen;
  GScanner *scanner;
  GherkinParser *parser;

  GError *err = NULL;

  if (argc > 1 && g_file_get_contents (argv[1], &content, &size, &err)) {
    scanner = gherkin_scanner_new (content, size, argv[1]);
  } else {
    g_printerr ("Error loading file '%s': %s\n",
        argv[1], err ? err->message : "Unkown reasons");

    return 1;
  }

  parser = gherkin_parser_new (scanner);
  if (!gherkin_parser_parse (parser)) {
    g_printerr ("Could not parser %s", argv[1]);

    return 1;
  }

  /* scanning loop, we parse the input until its end is reached,
   * the scanner encountered a lexing error, or our sub routine came
   * across invalid syntax
   */
  gen = json_generator_new ();
  root = gherkin_parser_get_ast (parser);
  json_generator_set_root (gen, root);
  g_object_set (gen, "pretty", TRUE, NULL);
  gchar *str = json_generator_to_data (gen, NULL);

  g_print ("%s", str);

  json_node_free (root);
  g_object_unref (gen);

  /* finsish parsing */
  g_scanner_destroy (scanner);

  return 0;
}
