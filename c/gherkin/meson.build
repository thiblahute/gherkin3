custom_target('gherkintypes.generated.c',
    input : '../razor/gherkin-types.c.razor',
    output : 'gherkintypes.generated.c',
    command : [mono, './../../bin/berp.exe',
         '-g', '../../gherkin.berp',
         '-t', '@INPUT@',
         '-o', '@OUTPUT@'],
    build_always : true,
    )

custom_target('gherkintypes.generated.h',
    input : '../razor/gherkin-types.h.razor',
    output : 'gherkintypes.generated.h',
    command : [mono, './../../bin/berp.exe',
         '-g', '../../gherkin.berp',
         '-t', '@INPUT@',
         '-o', '@OUTPUT@'],
    build_always : true,
    )

gherkin_sources = ['gherkinscanner.c', 'gherkinparser.c',
                   'gherkinastbuilder.c', 'gherkintypes.c',
                   'gherkinformatter.c', 'gherkintokenformatter.c',
                   'gherkindebug.c', 'gherkintypes.generated.c']

gherkin_headers = ['gherkinscanner.h', 'gherkinparser.h',
                   'gherkinastbuilder.h', 'gherkintypes.h',
                   'gherkinformatter.h', 'gherkintokenformatter.h',
                   'gherkindebug.h', 'gherkintypes.generated.h']

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
