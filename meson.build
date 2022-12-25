# SPDX-License-Identifier: MPL-2.0
# Copyright the libtyranny contributors

project('libtyranny', 'c',
	version: '1.0.0',
	default_options: [
	    'c_std=c11',
	    'warning_level=2',
	    'werror=true',
	])

# For strndup
if not meson.is_subproject()
    add_global_arguments('-D_POSIX_C_SOURCE=200809L', language: 'c')
endif

yaml = dependency('yaml-0.1', required: true)
deps = [yaml]
inc = include_directories('.')
libtyranny = library('tyranny', 'lib.c',
		     include_directories: inc,
		     dependencies: deps,
		     install: true)
pkg = import('pkgconfig')
pkg.generate(libtyranny)

libtyranny_dep = declare_dependency(include_directories: inc,
				    link_with: libtyranny)

install_headers('tyranny.h')

tyranny = executable('tyranny', 'main.c',
		     dependencies: deps,
		     link_with: libtyranny,
		     install: false)
