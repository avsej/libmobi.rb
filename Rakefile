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

require 'bundler/gem_tasks'
require 'rake/testtask'

module Bundler
  class GemHelper

   def tag_version
      sh "git tag -s -m \"Version #{version}\" #{version_tag}"
      Bundler.ui.confirm "Tagged #{version_tag}."
      yield if block_given?
    rescue
      Bundler.ui.error "Untagging #{version_tag} due to error."
      sh_with_code "git tag -d #{version_tag}"
      raise
    end
  end
end

Rake::TestTask.new(:test) do |t|
  t.libs << 'test'
  t.libs << 'lib'
  t.test_files = FileList['test/**/*_test.rb']
end

task :default => :test
