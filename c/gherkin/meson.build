gherkin_sources = ['gherkinscanner.c', 'gherkinparser.c', 'gherkinastbuilder.c', 'gherkintypes.c']
gherkin_headers = ['gherkinscanner.h', 'gherkinparser.h', 'gherkinastbuilder.h', 'gherkintypes.h']

gherkin = shared_library('gherkin-@0@'.format(apiversion),
    gherkin_sources,
    install: true,
    dependencies: [glib_dep, gobject_dep, json_glib_dep],
    include_directories: inc_dirs
)

pkgconfig_gen(libraries : gherkin,
    subdirs : '.',
    version : '0.1',
    name : 'gherkin',
    filebase : 'gherkin',
    description : 'A gherkin parser implementation in C.')

if build_gir
  gnome.generate_gir(gherkin,
    sources : gherkin_sources + gherkin_headers,
    namespace : 'Gherkin',
    nsversion : gherkin_version,
    link_with : gherkin,
    include_directories : inc_dirs,
    includes : ['GLib-2.0', 'GObject-2.0', 'Json-1.0'],
    install : true,
  )
endif
