project('gherkin', 'c')

apiversion = '0.1'
gherkin_major = 0
gherkin_minor = 0
gherkin_micro = 9
gherkin_nano = 1
gherkin_version = '@0@.@1@.@2@'.format(gherkin_major, gherkin_minor, gherkin_micro)

glib_dep = dependency('glib-2.0', version: '>=2.40')
gobject_dep = dependency('gobject-2.0', version: '>=2.40')
json_glib_dep = dependency('json-glib-1.0')

inc_dirs = include_directories('gherkin/')

gir = find_program('g-ir-scanner', required : false)
build_gir = gir.found() and not meson.is_cross_build()
gnome = import('gnome')

subdir('gherkin')

subdir('bin')
