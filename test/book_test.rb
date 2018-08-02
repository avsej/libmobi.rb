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

require 'test_helper'

class BookTest < Minitest::Test
  def test_that_it_can_load_book
    MOBI::Book.new(fixture_path('lorem.azw3'))
  end

  def test_that_it_can_access_basic_meta
    book = MOBI::Book.new(fixture_path('lorem.azw3'))
    assert_equal 'Lorem Ipsum', book.full_name

    hdr = book.mobi_header
    assert_equal 'MOBI', hdr[:magic]
    assert_equal 2, hdr[:mobi_type]
    assert_equal 65001, hdr[:text_encoding]
    assert_equal :utf8, hdr[:text_encoding_sym]
    assert_equal 264, hdr[:header_length]
    assert_equal 'en', hdr[:locale_str]

    hdr = book.pdb_header
    assert_equal 'test', hdr[:name]
    assert_equal 'BOOK', hdr[:type]
    assert_equal 'MOBI', hdr[:creator]
    assert_equal '2018-08-02 11:29:46 UTC', hdr[:ctime_time].utc.to_s
  end
end
