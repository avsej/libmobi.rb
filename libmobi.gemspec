# libmobi - library for handling Kindle (MOBI) formats of ebook documents
# Copyright (C) 2018 Sergey Avseyev <sergey.avseyev@gmail.com>
#
# This program is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation, either version 3 of the License, or
# (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program.  If not, see <https://www.gnu.org/licenses/>.

lib = File.expand_path('../lib', __FILE__)
$LOAD_PATH.unshift(lib) unless $LOAD_PATH.include?(lib)
require 'mobi/version'

Gem::Specification.new do |spec|
  spec.name = 'libmobi'
  spec.version = MOBI::VERSION
  spec.author = 'Sergey Avseyev'
  spec.email = 'sergey.avseyev@gmail.com'

  spec.summary = 'Library for handling Kindle (MOBI) formats of ebook documents'
  spec.homepage = 'https://github.com/avsej/libmobi.rb'

  spec.files = `git ls-files -z`.split("\x0").reject do |f|
    f.match(%r{^test/})
  end
  spec.bindir = 'exe'
  spec.executables = spec.files.grep(%r{^exe/}) { |f| File.basename(f) }
  spec.require_paths = ['lib']

  spec.add_development_dependency 'bundler', '~> 1.16'
  spec.add_development_dependency 'rake', '~> 12.3'
  spec.add_development_dependency 'minitest', '~> 5.11'
end
