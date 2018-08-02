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
    MOBIData *data;

    obj = Data_Make_Struct(klass, MOBIData, mb_book_mark, mb_book_free, data);
    return obj;
}

VALUE mb_book_init(VALUE self)
{
    mb_BOOK *book = DATA_PTR(self);

    book->data = mobi_init();
    if (book->data == NULL) {
        mb_raise(MOBI_MALLOC_FAILED, "unable to allocate MOBIData struct");
    }

    return self;
}

static void init_mobi_book()
{
    mb_cBook = rb_define_class_under(mb_mMOBI, "Book", rb_cObject);
    rb_define_alloc_func(mb_cBook, mb_book_alloc);
    rb_define_method(mb_cBook, "initialize", mb_book_init, 0);
}

void Init_mobi_ext()
{
    mb_mMOBI = rb_define_module("MOBI");
    rb_define_const(mb_mMOBI, "LIB_VERSION", rb_str_freeze(rb_external_str_new_cstr(mobi_version())));
    mb_eError = rb_const_get(mb_mMOBI, rb_intern("Error"));

    init_mobi_book();
}
