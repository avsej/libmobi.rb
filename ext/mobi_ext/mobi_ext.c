/* -*- Mode: C; tab-width: 4; c-basic-offset: 4; indent-tabs-mode: nil -*- */

#include <mobi.h>

#include <ruby.h>

VALUE mb_mMOBI;
VALUE mb_eError;
VALUE mb_cBook;

static const char *mb_libmobi_strerror(MOBI_RET code)
{
    switch (code) {
        case MOBI_ERROR:
            return "generic error return value";
        case MOBI_PARAM_ERR:
            return "wrong function parameter";
        case MOBI_DATA_CORRUPT:
            return "corrupted data";
        case MOBI_FILE_NOT_FOUND:
            return "file not found";
        case MOBI_FILE_ENCRYPTED:
            return "unsupported encrypted data";
        case MOBI_FILE_UNSUPPORTED:
            return "unsupported document type";
        case MOBI_MALLOC_FAILED:
            return "memory allocation error";
        case MOBI_INIT_FAILED:
            return "initialization error";
        case MOBI_BUFFER_END:
            return "out of buffer error";
        case MOBI_XML_ERR:
            return "XMLwriter error";
        case MOBI_DRM_PIDINV:
            return "invalid DRM PID";
        case MOBI_DRM_KEYNOTFOUND:
            return "key not found";
        case MOBI_DRM_UNSUPPORTED:
            return "DRM support not included";
        case MOBI_WRITE_FAILED:
            return "writing to file failed";
        default:
            return "FIXME: unkwnown code";
    }
}

static void mb_raise_at(MOBI_RET code, const char *message, const char *file, int line)
{
    VALUE exc, str;

    str = rb_str_new_cstr(message);
    if (code > 0) {
        rb_str_catf(str, ": (0x%02x) \"%s\"", (int)code, mb_libmobi_strerror(code));
    }
    rb_str_buf_cat_ascii(str, " at ");
    rb_str_buf_cat_ascii(str, file);
    rb_str_catf(str, ":%d", line);
    exc = rb_exc_new3(mb_eError, str);
    rb_ivar_set(exc, rb_intern("@code"), INT2FIX(code));
    rb_exc_raise(exc);
}

#define mb_raise(code, message) mb_raise_at(code, message, __FILE__, __LINE__)
#define mb_raise_msg(message) mb_raise_at(0, message, __FILE__, __LINE__)

typedef struct mb_BOOK {
    MOBIData *data;
} mb_BOOK;

static void mb_book_mark(void *ptr)
{
    mb_BOOK *book = ptr;
    (void)book;
}

static void mb_book_free(void *ptr)
{
    mb_BOOK *book = ptr;
    if (book) {
        if (book->data) {
            mobi_free(book->data);
        }
        book->data = NULL;
    }
}

static VALUE mb_book_alloc(VALUE klass)
{
    VALUE obj;
    mb_BOOK *book;

    obj = Data_Make_Struct(klass, mb_BOOK, mb_book_mark, mb_book_free, book);
    return obj;
}

static VALUE mb_book_init(VALUE self, VALUE path)
{
    mb_BOOK *book = DATA_PTR(self);
    MOBI_RET rc;

    Check_Type(path, T_STRING);

    book->data = mobi_init();
    if (book->data == NULL) {
        mb_raise(MOBI_MALLOC_FAILED, "unable to allocate MOBIData struct");
    }

    rc = mobi_load_filename(book->data, RSTRING_PTR(path));
    if (rc != MOBI_SUCCESS) {
        mb_raise(rc, "unable to load book from path");
    }
    return self;
}

#define COPY_HEADER_INT(NAME)                                                                                          \
    if (hdr->NAME) {                                                                                                   \
        rb_hash_aset(res, ID2SYM(rb_intern(#NAME)), INT2FIX(*hdr->NAME));                                              \
    }
static VALUE mb_book_mobi_header(VALUE self)
{
    mb_BOOK *book = DATA_PTR(self);
    VALUE res = rb_hash_new();
    MOBIMobiHeader *hdr;

    if (book == NULL || book->data == NULL) {
        mb_raise(MOBI_INIT_FAILED, "the MOBI data is not loaded");
    }
    hdr = book->data->mh;
    if (hdr == NULL) {
        return Qnil;
    }

    rb_hash_aset(res, ID2SYM(rb_intern("magic")), rb_str_new_cstr(hdr->mobi_magic));
    COPY_HEADER_INT(header_length);
    COPY_HEADER_INT(mobi_type);
    if (hdr->text_encoding) {
        rb_hash_aset(res, ID2SYM(rb_intern("text_encoding")), INT2FIX(*hdr->text_encoding));
        switch (*hdr->text_encoding) {
            case MOBI_CP1252:
                rb_hash_aset(res, ID2SYM(rb_intern("text_encoding_sym")), ID2SYM(rb_intern("cp1252")));
                break;
            case MOBI_UTF8:
                rb_hash_aset(res, ID2SYM(rb_intern("text_encoding_sym")), ID2SYM(rb_intern("utf8")));
                break;
            case MOBI_UTF16:
                rb_hash_aset(res, ID2SYM(rb_intern("text_encoding_sym")), ID2SYM(rb_intern("utf16")));
                break;
            default:
                rb_hash_aset(res, ID2SYM(rb_intern("text_encoding_sym")), ID2SYM(rb_intern("unknown")));
                break;
        }
    }
    if (hdr->locale) {
        const char *locale_string = mobi_get_locale_string(*hdr->locale);
        if (locale_string) {
            rb_hash_aset(res, ID2SYM(rb_intern("locale_str")), rb_str_new_cstr(locale_string));
        }
        rb_hash_aset(res, ID2SYM(rb_intern("locale")), INT2FIX(*hdr->locale));
    }
    if (hdr->dict_input_lang) {
        const char *locale_string = mobi_get_locale_string(*hdr->dict_input_lang);
        if (locale_string) {
            rb_hash_aset(res, ID2SYM(rb_intern("dict_input_lang_str")), rb_str_new_cstr(locale_string));
        }
        rb_hash_aset(res, ID2SYM(rb_intern("dict_input_lang")), INT2FIX(*hdr->dict_input_lang));
    }
    if (hdr->dict_output_lang) {
        const char *locale_string = mobi_get_locale_string(*hdr->dict_output_lang);
        if (locale_string) {
            rb_hash_aset(res, ID2SYM(rb_intern("dict_output_lang_str")), rb_str_new_cstr(locale_string));
        }
        rb_hash_aset(res, ID2SYM(rb_intern("dict_output_lang")), INT2FIX(*hdr->dict_output_lang));
    }
    COPY_HEADER_INT(uid);
    COPY_HEADER_INT(version);
    COPY_HEADER_INT(orth_index);
    COPY_HEADER_INT(infl_index);
    COPY_HEADER_INT(names_index);
    COPY_HEADER_INT(keys_index);
    COPY_HEADER_INT(keys_index);
    COPY_HEADER_INT(extra0_index);
    COPY_HEADER_INT(extra1_index);
    COPY_HEADER_INT(extra2_index);
    COPY_HEADER_INT(extra3_index);
    COPY_HEADER_INT(extra4_index);
    COPY_HEADER_INT(extra5_index);
    COPY_HEADER_INT(non_text_index);
    COPY_HEADER_INT(full_name_offset);
    COPY_HEADER_INT(full_name_length);
    COPY_HEADER_INT(min_version);
    COPY_HEADER_INT(image_index);
    COPY_HEADER_INT(huff_rec_index);
    COPY_HEADER_INT(huff_rec_count);
    COPY_HEADER_INT(datp_rec_index);
    COPY_HEADER_INT(datp_rec_count);
    COPY_HEADER_INT(exth_flags);
    COPY_HEADER_INT(unknown6);
    COPY_HEADER_INT(drm_offset);
    COPY_HEADER_INT(drm_count);
    COPY_HEADER_INT(drm_size);
    COPY_HEADER_INT(drm_flags);
    COPY_HEADER_INT(first_text_index);
    COPY_HEADER_INT(last_text_index);
    COPY_HEADER_INT(fdst_index);
    COPY_HEADER_INT(fdst_section_count);
    COPY_HEADER_INT(fcis_index);
    COPY_HEADER_INT(fcis_count);
    COPY_HEADER_INT(flis_index);
    COPY_HEADER_INT(flis_count);
    COPY_HEADER_INT(unknown10);
    COPY_HEADER_INT(unknown11);
    COPY_HEADER_INT(srcs_index);
    COPY_HEADER_INT(srcs_count);
    COPY_HEADER_INT(unknown12);
    COPY_HEADER_INT(unknown13);
    COPY_HEADER_INT(extra_flags);
    COPY_HEADER_INT(ncx_index);
    COPY_HEADER_INT(unknown14);
    COPY_HEADER_INT(unknown15);
    COPY_HEADER_INT(fragment_index);
    COPY_HEADER_INT(skeleton_index);
    COPY_HEADER_INT(datp_index);
    COPY_HEADER_INT(unknown16);
    COPY_HEADER_INT(guide_index);
    COPY_HEADER_INT(unknown17);
    COPY_HEADER_INT(unknown18);
    COPY_HEADER_INT(unknown19);
    COPY_HEADER_INT(unknown20);

    return res;
}
#undef COPY_HEADER_INT

static VALUE mb_book_pdb_header(VALUE self)
{
    mb_BOOK *book = DATA_PTR(self);
    VALUE res = rb_hash_new();
    MOBIPdbHeader *hdr;

    if (book == NULL || book->data == NULL) {
        mb_raise(MOBI_INIT_FAILED, "the MOBI data is not loaded");
    }
    hdr = book->data->ph;
    if (hdr == NULL) {
        return Qnil;
    }

    rb_hash_aset(res, ID2SYM(rb_intern("name")), rb_str_new_cstr(hdr->name));
    rb_hash_aset(res, ID2SYM(rb_intern("attributes")), INT2FIX(hdr->attributes));
    rb_hash_aset(res, ID2SYM(rb_intern("version")), INT2FIX(hdr->version));
    rb_hash_aset(res, ID2SYM(rb_intern("ctime")), INT2FIX(hdr->ctime));
    if (hdr->ctime) {
        rb_hash_aset(res, ID2SYM(rb_intern("ctime_time")), rb_time_new(mktime(mobi_pdbtime_to_time(hdr->ctime)), 0));
    }
    rb_hash_aset(res, ID2SYM(rb_intern("mtime")), INT2FIX(hdr->mtime));
    if (hdr->mtime) {
        rb_hash_aset(res, ID2SYM(rb_intern("mtime_time")), rb_time_new(mktime(mobi_pdbtime_to_time(hdr->mtime)), 0));
    }
    rb_hash_aset(res, ID2SYM(rb_intern("btime")), INT2FIX(hdr->btime));
    if (hdr->btime) {
        rb_hash_aset(res, ID2SYM(rb_intern("btime_time")), rb_time_new(mktime(mobi_pdbtime_to_time(hdr->btime)), 0));
    }
    rb_hash_aset(res, ID2SYM(rb_intern("mod_num")), INT2FIX(hdr->mod_num));
    rb_hash_aset(res, ID2SYM(rb_intern("appinfo_offset")), INT2FIX(hdr->appinfo_offset));
    rb_hash_aset(res, ID2SYM(rb_intern("sortinfo_offset")), INT2FIX(hdr->sortinfo_offset));
    rb_hash_aset(res, ID2SYM(rb_intern("type")), rb_str_new_cstr(hdr->type));
    rb_hash_aset(res, ID2SYM(rb_intern("creator")), rb_str_new_cstr(hdr->creator));
    rb_hash_aset(res, ID2SYM(rb_intern("uid")), INT2FIX(hdr->uid));
    rb_hash_aset(res, ID2SYM(rb_intern("next_rec")), INT2FIX(hdr->next_rec));
    rb_hash_aset(res, ID2SYM(rb_intern("rec_count")), INT2FIX(hdr->rec_count));
    return res;
}

static VALUE mb_book_record0_header(VALUE self)
{
    mb_BOOK *book = DATA_PTR(self);
    VALUE res = rb_hash_new();
    MOBIRecord0Header *hdr;

    if (book == NULL || book->data == NULL) {
        mb_raise(MOBI_INIT_FAILED, "the MOBI data is not loaded");
    }
    hdr = book->data->rh;
    if (hdr == NULL) {
        return Qnil;
    }

    rb_hash_aset(res, ID2SYM(rb_intern("compression_type")), INT2FIX(hdr->compression_type));
    switch (hdr->compression_type) {
        case 1:
            rb_hash_aset(res, ID2SYM(rb_intern("compression_type_sym")), ID2SYM(rb_intern("none")));
            break;
        case 2:
            rb_hash_aset(res, ID2SYM(rb_intern("compression_type_sym")), ID2SYM(rb_intern("palm_doc")));
            break;
        case 17480:
            rb_hash_aset(res, ID2SYM(rb_intern("compression_type_sym")), ID2SYM(rb_intern("huff_cdic")));
            break;
    }
    rb_hash_aset(res, ID2SYM(rb_intern("text_length")), INT2FIX(hdr->text_length));
    rb_hash_aset(res, ID2SYM(rb_intern("text_record_count")), INT2FIX(hdr->text_record_count));
    rb_hash_aset(res, ID2SYM(rb_intern("text_record_size")), INT2FIX(hdr->text_record_size));
    rb_hash_aset(res, ID2SYM(rb_intern("encryption_type")), INT2FIX(hdr->encryption_type));
    switch (hdr->encryption_type) {
        case 0:
            rb_hash_aset(res, ID2SYM(rb_intern("encryption_type_sym")), ID2SYM(rb_intern("none")));
            break;
        case 1:
            rb_hash_aset(res, ID2SYM(rb_intern("encryption_type_sym")), ID2SYM(rb_intern("old")));
            break;
        case 2:
            rb_hash_aset(res, ID2SYM(rb_intern("encryption_type_sym")), ID2SYM(rb_intern("mobi")));
            break;
    }
    rb_hash_aset(res, ID2SYM(rb_intern("unknown1")), INT2FIX(hdr->unknown1));
    return res;
}

static VALUE mb_book_exth_header(VALUE self)
{
    mb_BOOK *book = DATA_PTR(self);
    VALUE res = rb_ary_new();
    MOBIExthHeader *hdr;

    if (book == NULL || book->data == NULL) {
        mb_raise(MOBI_INIT_FAILED, "the MOBI data is not loaded");
    }
    hdr = book->data->eh;
    if (hdr == NULL) {
        return Qnil;
    }

    while (hdr != NULL) {
        MOBIExthMeta tag = mobi_get_exthtagmeta_by_tag(hdr->tag);
        uint32_t val32;
        VALUE item = rb_hash_new();

        rb_hash_aset(item, ID2SYM(rb_intern("code")), INT2FIX(tag.tag));
        switch (hdr->tag) {
            case EXTH_SAMPLE:
                rb_hash_aset(item, ID2SYM(rb_intern("id")), ID2SYM(rb_intern("sample")));
                break;
            case EXTH_STARTREADING:
                rb_hash_aset(item, ID2SYM(rb_intern("id")), ID2SYM(rb_intern("start_reading")));
                break;
            case EXTH_KF8BOUNDARY:
                rb_hash_aset(item, ID2SYM(rb_intern("id")), ID2SYM(rb_intern("kf8_boundary")));
                break;
            case EXTH_COUNTRESOURCES:
                rb_hash_aset(item, ID2SYM(rb_intern("id")), ID2SYM(rb_intern("count_resources")));
                break;
            case EXTH_RESCOFFSET:
                rb_hash_aset(item, ID2SYM(rb_intern("id")), ID2SYM(rb_intern("resc_offset")));
                break;
            case EXTH_COVEROFFSET:
                rb_hash_aset(item, ID2SYM(rb_intern("id")), ID2SYM(rb_intern("cover_offset")));
                break;
            case EXTH_THUMBOFFSET:
                rb_hash_aset(item, ID2SYM(rb_intern("id")), ID2SYM(rb_intern("thumb_offset")));
                break;
            case EXTH_HASFAKECOVER:
                rb_hash_aset(item, ID2SYM(rb_intern("id")), ID2SYM(rb_intern("has_fake_cover")));
                break;
            case EXTH_CREATORSOFT:
                rb_hash_aset(item, ID2SYM(rb_intern("id")), ID2SYM(rb_intern("creator_soft")));
                break;
            case EXTH_CREATORMAJOR:
                rb_hash_aset(item, ID2SYM(rb_intern("id")), ID2SYM(rb_intern("creator_major")));
                break;
            case EXTH_CREATORMINOR:
                rb_hash_aset(item, ID2SYM(rb_intern("id")), ID2SYM(rb_intern("creator_minor")));
                break;
            case EXTH_CREATORBUILD:
                rb_hash_aset(item, ID2SYM(rb_intern("id")), ID2SYM(rb_intern("creator_build")));
                break;
            case EXTH_CLIPPINGLIMIT:
                rb_hash_aset(item, ID2SYM(rb_intern("id")), ID2SYM(rb_intern("clipping_limit")));
                break;
            case EXTH_PUBLISHERLIMIT:
                rb_hash_aset(item, ID2SYM(rb_intern("id")), ID2SYM(rb_intern("publisher_limit")));
                break;
            case EXTH_TTSDISABLE:
                rb_hash_aset(item, ID2SYM(rb_intern("id")), ID2SYM(rb_intern("tts_disabled")));
                break;
            case EXTH_RENTAL:
                rb_hash_aset(item, ID2SYM(rb_intern("id")), ID2SYM(rb_intern("rental")));
                break;
            case EXTH_DRMSERVER:
                rb_hash_aset(item, ID2SYM(rb_intern("id")), ID2SYM(rb_intern("drm_server")));
                break;
            case EXTH_DRMCOMMERCE:
                rb_hash_aset(item, ID2SYM(rb_intern("id")), ID2SYM(rb_intern("drm_commerce")));
                break;
            case EXTH_DRMEBOOKBASE:
                rb_hash_aset(item, ID2SYM(rb_intern("id")), ID2SYM(rb_intern("drm_ebookbase")));
                break;
            case EXTH_TITLE:
                rb_hash_aset(item, ID2SYM(rb_intern("id")), ID2SYM(rb_intern("title")));
                break;
            case EXTH_AUTHOR:
                rb_hash_aset(item, ID2SYM(rb_intern("id")), ID2SYM(rb_intern("creator")));
                break;
            case EXTH_PUBLISHER:
                rb_hash_aset(item, ID2SYM(rb_intern("id")), ID2SYM(rb_intern("publisher")));
                break;
            case EXTH_IMPRINT:
                rb_hash_aset(item, ID2SYM(rb_intern("id")), ID2SYM(rb_intern("imprint")));
                break;
            case EXTH_DESCRIPTION:
                rb_hash_aset(item, ID2SYM(rb_intern("id")), ID2SYM(rb_intern("description")));
                break;
            case EXTH_ISBN:
                rb_hash_aset(item, ID2SYM(rb_intern("id")), ID2SYM(rb_intern("isbn")));
                break;
            case EXTH_SUBJECT:
                rb_hash_aset(item, ID2SYM(rb_intern("id")), ID2SYM(rb_intern("subject")));
                break;
            case EXTH_PUBLISHINGDATE:
                rb_hash_aset(item, ID2SYM(rb_intern("id")), ID2SYM(rb_intern("published")));
                break;
            case EXTH_REVIEW:
                rb_hash_aset(item, ID2SYM(rb_intern("id")), ID2SYM(rb_intern("review")));
                break;
            case EXTH_CONTRIBUTOR:
                rb_hash_aset(item, ID2SYM(rb_intern("id")), ID2SYM(rb_intern("contributor")));
                break;
            case EXTH_RIGHTS:
                rb_hash_aset(item, ID2SYM(rb_intern("id")), ID2SYM(rb_intern("rights")));
                break;
            case EXTH_SUBJECTCODE:
                rb_hash_aset(item, ID2SYM(rb_intern("id")), ID2SYM(rb_intern("subject_code")));
                break;
            case EXTH_TYPE:
                rb_hash_aset(item, ID2SYM(rb_intern("id")), ID2SYM(rb_intern("type")));
                break;
            case EXTH_SOURCE:
                rb_hash_aset(item, ID2SYM(rb_intern("id")), ID2SYM(rb_intern("source")));
                break;
            case EXTH_ASIN:
                rb_hash_aset(item, ID2SYM(rb_intern("id")), ID2SYM(rb_intern("asin")));
                break;
            case EXTH_VERSION:
                rb_hash_aset(item, ID2SYM(rb_intern("id")), ID2SYM(rb_intern("version")));
                break;
            case EXTH_ADULT:
                rb_hash_aset(item, ID2SYM(rb_intern("id")), ID2SYM(rb_intern("adult")));
                break;
            case EXTH_PRICE:
                rb_hash_aset(item, ID2SYM(rb_intern("id")), ID2SYM(rb_intern("price")));
                break;
            case EXTH_CURRENCY:
                rb_hash_aset(item, ID2SYM(rb_intern("id")), ID2SYM(rb_intern("currency")));
                break;
            case EXTH_FIXEDLAYOUT:
                rb_hash_aset(item, ID2SYM(rb_intern("id")), ID2SYM(rb_intern("fixed_layout")));
                break;
            case EXTH_BOOKTYPE:
                rb_hash_aset(item, ID2SYM(rb_intern("id")), ID2SYM(rb_intern("book_type")));
                break;
            case EXTH_ORIENTATIONLOCK:
                rb_hash_aset(item, ID2SYM(rb_intern("id")), ID2SYM(rb_intern("orientation_lock")));
                break;
            case EXTH_ORIGRESOLUTION:
                rb_hash_aset(item, ID2SYM(rb_intern("id")), ID2SYM(rb_intern("orig_resolution")));
                break;
            case EXTH_ZEROGUTTER:
                rb_hash_aset(item, ID2SYM(rb_intern("id")), ID2SYM(rb_intern("zero_gutter")));
                break;
            case EXTH_ZEROMARGIN:
                rb_hash_aset(item, ID2SYM(rb_intern("id")), ID2SYM(rb_intern("zero_margin")));
                break;
            case EXTH_KF8COVERURI:
                rb_hash_aset(item, ID2SYM(rb_intern("id")), ID2SYM(rb_intern("kf8_cover_uri")));
                break;
            case EXTH_REGIONMAGNI:
                rb_hash_aset(item, ID2SYM(rb_intern("id")), ID2SYM(rb_intern("region_magnification")));
                break;
            case EXTH_DICTNAME:
                rb_hash_aset(item, ID2SYM(rb_intern("id")), ID2SYM(rb_intern("dict_name")));
                break;
            case EXTH_WATERMARK:
                rb_hash_aset(item, ID2SYM(rb_intern("id")), ID2SYM(rb_intern("watermark")));
                break;
            case EXTH_DOCTYPE:
                rb_hash_aset(item, ID2SYM(rb_intern("id")), ID2SYM(rb_intern("doc_type")));
                break;
            case EXTH_LASTUPDATE:
                rb_hash_aset(item, ID2SYM(rb_intern("id")), ID2SYM(rb_intern("last_update")));
                break;
            case EXTH_UPDATEDTITLE:
                rb_hash_aset(item, ID2SYM(rb_intern("id")), ID2SYM(rb_intern("updated_title")));
                break;
            case EXTH_ASIN504:
                rb_hash_aset(item, ID2SYM(rb_intern("id")), ID2SYM(rb_intern("asin_504")));
                break;
            case EXTH_TITLEFILEAS:
                rb_hash_aset(item, ID2SYM(rb_intern("id")), ID2SYM(rb_intern("title_file_as")));
                break;
            case EXTH_CREATORFILEAS:
                rb_hash_aset(item, ID2SYM(rb_intern("id")), ID2SYM(rb_intern("creator_file_as")));
                break;
            case EXTH_PUBLISHERFILEAS:
                rb_hash_aset(item, ID2SYM(rb_intern("id")), ID2SYM(rb_intern("publisher_file_as")));
                break;
            case EXTH_LANGUAGE:
                rb_hash_aset(item, ID2SYM(rb_intern("id")), ID2SYM(rb_intern("language")));
                break;
            case EXTH_ALIGNMENT:
                rb_hash_aset(item, ID2SYM(rb_intern("id")), ID2SYM(rb_intern("alignment")));
                break;
            case EXTH_PAGEDIR:
                rb_hash_aset(item, ID2SYM(rb_intern("id")), ID2SYM(rb_intern("page_dir")));
                break;
            case EXTH_OVERRIDEFONTS:
                rb_hash_aset(item, ID2SYM(rb_intern("id")), ID2SYM(rb_intern("override_fonts")));
                break;
            case EXTH_SORCEDESC:
                rb_hash_aset(item, ID2SYM(rb_intern("id")), ID2SYM(rb_intern("source_desc")));
                break;
            case EXTH_DICTLANGIN:
                rb_hash_aset(item, ID2SYM(rb_intern("id")), ID2SYM(rb_intern("dict_lang_in")));
                break;
            case EXTH_DICTLANGOUT:
                rb_hash_aset(item, ID2SYM(rb_intern("id")), ID2SYM(rb_intern("dict_lang_out")));
                break;
            case EXTH_INPUTSOURCE:
                rb_hash_aset(item, ID2SYM(rb_intern("id")), ID2SYM(rb_intern("input_source")));
                break;
            case EXTH_CREATORBUILDREV:
                rb_hash_aset(item, ID2SYM(rb_intern("id")), ID2SYM(rb_intern("creator_build_rev")));
                break;
            case EXTH_CREATORSTRING:
                rb_hash_aset(item, ID2SYM(rb_intern("id")), ID2SYM(rb_intern("creator_string")));
                break;
            case EXTH_TAMPERKEYS:
                rb_hash_aset(item, ID2SYM(rb_intern("id")), ID2SYM(rb_intern("tamper_keys")));
                break;
            case EXTH_FONTSIGNATURE:
                rb_hash_aset(item, ID2SYM(rb_intern("id")), ID2SYM(rb_intern("font_signature")));
                break;
            case EXTH_UNK403:
                rb_hash_aset(item, ID2SYM(rb_intern("id")), ID2SYM(rb_intern("unknown_403")));
                break;
            case EXTH_UNK405:
                rb_hash_aset(item, ID2SYM(rb_intern("id")), ID2SYM(rb_intern("unknown_405")));
                break;
            case EXTH_UNK407:
                rb_hash_aset(item, ID2SYM(rb_intern("id")), ID2SYM(rb_intern("unknown_407")));
                break;
            case EXTH_UNK450:
                rb_hash_aset(item, ID2SYM(rb_intern("id")), ID2SYM(rb_intern("unknown_450")));
                break;
            case EXTH_UNK451:
                rb_hash_aset(item, ID2SYM(rb_intern("id")), ID2SYM(rb_intern("unknown_451")));
                break;
            case EXTH_UNK452:
                rb_hash_aset(item, ID2SYM(rb_intern("id")), ID2SYM(rb_intern("unknown_452")));
                break;
            case EXTH_UNK453:
                rb_hash_aset(item, ID2SYM(rb_intern("id")), ID2SYM(rb_intern("unknown_453")));
                break;
        }
        if (tag.tag == 0) {
            rb_hash_aset(item, ID2SYM(rb_intern("val_bin")), rb_str_new((const char *)hdr->data, hdr->size));
            val32 = mobi_decode_exthvalue(hdr->data, hdr->size);
            rb_hash_aset(item, ID2SYM(rb_intern("val_num")), INT2FIX(val32));
        } else {
            char *str;
            rb_hash_aset(item, ID2SYM(rb_intern("name")), rb_str_new_cstr(tag.name));
            switch (tag.type) {
                case EXTH_NUMERIC:
                    val32 = mobi_decode_exthvalue(hdr->data, hdr->size);
                    rb_hash_aset(item, ID2SYM(rb_intern("val_num")), INT2FIX(val32));
                    break;
                case EXTH_STRING:
                    str = mobi_decode_exthstring(book->data, hdr->data, hdr->size);
                    if (str) {
                        rb_hash_aset(item, ID2SYM(rb_intern("val_str")), rb_str_new_cstr(str));
                        free(str);
                    }
                    break;
                case EXTH_BINARY:
                    rb_hash_aset(item, ID2SYM(rb_intern("val_bin")), rb_str_new((const char *)hdr->data, hdr->size));
                    break;
                default:
                    break;
            }
        }
        rb_ary_push(res, item);
        hdr = hdr->next;
    }
    return res;
}

#define FULL_NAME_MAX 1024
static VALUE mb_book_full_name(VALUE self)
{
    mb_BOOK *book = DATA_PTR(self);
    MOBI_RET rc;
    char full_name[FULL_NAME_MAX + 1] = {0};

    if (book == NULL || book->data == NULL) {
        mb_raise(MOBI_INIT_FAILED, "the MOBI data is not loaded");
    }
    rc = mobi_get_fullname(book->data, full_name, FULL_NAME_MAX);
    if (rc != MOBI_SUCCESS) {
        mb_raise(rc, "unable to fetch book full name");
    }
    return rb_str_new_cstr(full_name);
}
#undef FULL_NAME_MAX

#define DEFINE_META_GETTER_STR(ATTR)                                                                                   \
    static VALUE mb_book_##ATTR(VALUE self)                                                                            \
    {                                                                                                                  \
        mb_BOOK *book = DATA_PTR(self);                                                                                \
        const char *str;                                                                                               \
                                                                                                                       \
        if (book == NULL || book->data == NULL) {                                                                      \
            mb_raise(MOBI_INIT_FAILED, "the MOBI data is not loaded");                                                 \
        }                                                                                                              \
        str = mobi_meta_get_##ATTR(book->data);                                                                        \
        if (str) {                                                                                                     \
            return rb_str_new_cstr(str);                                                                               \
        }                                                                                                              \
        return Qnil;                                                                                                   \
    }

DEFINE_META_GETTER_STR(title)
DEFINE_META_GETTER_STR(author)
DEFINE_META_GETTER_STR(publisher)
DEFINE_META_GETTER_STR(imprint)
DEFINE_META_GETTER_STR(description)
DEFINE_META_GETTER_STR(isbn)
DEFINE_META_GETTER_STR(subject)
DEFINE_META_GETTER_STR(publishdate)
DEFINE_META_GETTER_STR(review)
DEFINE_META_GETTER_STR(contributor)
DEFINE_META_GETTER_STR(copyright)
DEFINE_META_GETTER_STR(asin)
DEFINE_META_GETTER_STR(language)

#define DEFINE_PREDICATE(METHOD, FUNCTION)                                                                             \
    static VALUE mb_book_p_##METHOD(VALUE self)                                                                        \
    {                                                                                                                  \
        mb_BOOK *book = DATA_PTR(self);                                                                                \
                                                                                                                       \
        if (book == NULL || book->data == NULL) {                                                                      \
            mb_raise(MOBI_INIT_FAILED, "the MOBI data is not loaded");                                                 \
        }                                                                                                              \
        if (FUNCTION(book->data)) {                                                                                    \
            return Qtrue;                                                                                              \
        }                                                                                                              \
        return Qfalse;                                                                                                 \
    }

DEFINE_PREDICATE(has_mobi_header, mobi_exists_mobiheader)
DEFINE_PREDICATE(has_fdst, mobi_exists_fdst)
DEFINE_PREDICATE(has_skeleton_index, mobi_exists_skel_indx)
DEFINE_PREDICATE(has_fragments_index, mobi_exists_frag_indx)
DEFINE_PREDICATE(has_guide_index, mobi_exists_guide_indx)
DEFINE_PREDICATE(has_ncx_index, mobi_exists_ncx)
DEFINE_PREDICATE(has_orth_index, mobi_exists_orth)
DEFINE_PREDICATE(has_infl_index, mobi_exists_infl)
DEFINE_PREDICATE(is_hybrid, mobi_is_hybrid)
DEFINE_PREDICATE(is_encrypted, mobi_is_encrypted)
DEFINE_PREDICATE(is_mobipocket, mobi_is_mobipocket)
DEFINE_PREDICATE(is_dictionary, mobi_is_dictionary)
DEFINE_PREDICATE(is_kf8, mobi_is_kf8)

static void init_mobi_book()
{
    mb_cBook = rb_define_class_under(mb_mMOBI, "Book", rb_cObject);
    rb_define_alloc_func(mb_cBook, mb_book_alloc);
    rb_define_method(mb_cBook, "initialize", mb_book_init, 1);
    rb_define_method(mb_cBook, "mobi_header", mb_book_mobi_header, 0);
    rb_define_method(mb_cBook, "pdb_header", mb_book_pdb_header, 0);
    rb_define_method(mb_cBook, "record0_header", mb_book_record0_header, 0);
    rb_define_method(mb_cBook, "exth_header", mb_book_exth_header, 0);
    rb_define_method(mb_cBook, "full_name", mb_book_full_name, 0);
    rb_define_method(mb_cBook, "title", mb_book_title, 0);
    rb_define_method(mb_cBook, "author", mb_book_author, 0);
    rb_define_method(mb_cBook, "publisher", mb_book_publisher, 0);
    rb_define_method(mb_cBook, "imprint", mb_book_imprint, 0);
    rb_define_method(mb_cBook, "description", mb_book_description, 0);
    rb_define_method(mb_cBook, "isbn", mb_book_isbn, 0);
    rb_define_method(mb_cBook, "subject", mb_book_subject, 0);
    rb_define_method(mb_cBook, "publishdate", mb_book_publishdate, 0);
    rb_define_method(mb_cBook, "review", mb_book_review, 0);
    rb_define_method(mb_cBook, "contributor", mb_book_contributor, 0);
    rb_define_method(mb_cBook, "copyright", mb_book_copyright, 0);
    rb_define_method(mb_cBook, "asin", mb_book_asin, 0);
    rb_define_method(mb_cBook, "language", mb_book_language, 0);
    rb_define_method(mb_cBook, "has_mobi_header?", mb_book_p_has_mobi_header, 0);
    rb_define_method(mb_cBook, "has_fdst?", mb_book_p_has_fdst, 0);
    rb_define_method(mb_cBook, "has_skeleton_index?", mb_book_p_has_skeleton_index, 0);
    rb_define_method(mb_cBook, "has_fragments_index?", mb_book_p_has_fragments_index, 0);
    rb_define_method(mb_cBook, "has_guide_index?", mb_book_p_has_guide_index, 0);
    rb_define_method(mb_cBook, "has_ncx_index?", mb_book_p_has_ncx_index, 0);
    rb_define_method(mb_cBook, "has_orth_index?", mb_book_p_has_orth_index, 0);
    rb_define_method(mb_cBook, "has_infl_index?", mb_book_p_has_infl_index, 0);
    rb_define_method(mb_cBook, "is_hybrid?", mb_book_p_is_hybrid, 0);
    rb_define_method(mb_cBook, "is_encrypted?", mb_book_p_is_encrypted, 0);
    rb_define_method(mb_cBook, "is_mobipocket?", mb_book_p_is_mobipocket, 0);
    rb_define_method(mb_cBook, "is_dictionary?", mb_book_p_is_dictionary, 0);
    rb_define_method(mb_cBook, "is_kf8?", mb_book_p_is_kf8, 0);
}

void Init_mobi_ext()
{
    mb_mMOBI = rb_define_module("MOBI");
    rb_define_const(mb_mMOBI, "LIB_VERSION", rb_str_freeze(rb_external_str_new_cstr(mobi_version())));
    mb_eError = rb_const_get(mb_mMOBI, rb_intern("Error"));

    init_mobi_book();
}
