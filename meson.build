# SPDX-License-Identifier: MPL-2.0

project('libtyranny', 'c',
	version: '1.0.0',
	default_options: [
	    'c_std=c11',
	    'warning_level=2',
	    'werror=true',
	])

# For strndup
add_global_arguments('-D_POSIX_C_SOURCE=200809L', language: 'c')

yaml = dependency('yaml-0.1', required: true)
deps = [yaml]
libtyranny = library('tyranny', 'lib.c', 'tyranny.h',
		     dependencies: deps,
		     install: true)
pkg = import('pkgconfig')
pkg.generate(libtyranny)

install_headers('tyranny.h')

tyranny = executable('tyranny', 'main.c',
		     dependencies: deps,
		     link_with: libtyranny,
		     install: false)
