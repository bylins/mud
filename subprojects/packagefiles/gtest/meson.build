project(
  'gtest',
  'cpp',
  version: 'latest',
  meson_version: '>=0.49.0',
  license: 'BSD-3-Clause',
)

override_cpp = meson.version().version_compare('>=1.3.0') ? 'cpp_std=gnu++17,c++17' : 'cpp_std=gnu++17'

subdir('googletest')

subdir('googlemock')
