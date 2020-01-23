project('xturtle', 'cpp',
  version: '0.1.0',
  license: 'GPL-3.0',
  default_options: [
    'cpp_std=c++17',
    'cpp_args=-Wextra -Wpedantic -Wconversion',
])

deps = [
  dependency('xcb'),
  dependency('xcb-util'),
  dependency('cairo'),
  dependency('guile-2.2'),
]

spdlog_dep = dependency('spdlog', version: '>=1.5.0', required: false)
if not spdlog_dep.found()
  spdlog_proj = subproject('spdlog', default_options: [
    'enable_tests=false',
    'enable_examples=false',
  ])
  deps += spdlog_proj.get_variable('spdlog_dep')
endif

conf_data = configuration_data({
  'version': meson.project_version(),
  'XTURTLE_DEBUG_ENABLED': true,
})
configure_file(
  input: 'include/config.hh.in',
  output: 'config.hh',
  configuration: conf_data
)

subdir('include')
subdir('src')

executable('xturtle',
  srcs,
  include_directories: include,
  dependencies: deps,
  install: true
)
