#include <ruby.h>
#include <FreeImage.h>

static VALUE rb_mFI;
static VALUE rb_eFI;
static VALUE Class_Image;

static VALUE rb_rfi_version(VALUE self)
{
	return rb_ary_new3(3, INT2NUM(FREEIMAGE_MAJOR_VERSION),
		INT2NUM(FREEIMAGE_MINOR_VERSION), INT2NUM(FREEIMAGE_RELEASE_SERIAL));
}

static VALUE rb_rfi_string_version(VALUE self)
{
	VALUE result = Qnil;
	const char *v = FreeImage_GetVersion();
	result = rb_str_new(v, strlen(v));
	return result;
}

static VALUE
file_arg_rescue(VALUE arg)
{
	rb_raise(rb_eTypeError, "argument must be path name or open file (%s given)",
			rb_class2name(CLASS_OF(arg)));
	return(VALUE)0;
}

struct native_image {
	int w;
	int h;
	int bpp;
	int stride;
	FIBITMAP *handle;
	char *filename;
};

void Image_free(struct native_image* img)
{
	if(!img)
		return;
	if(img->handle)
		FreeImage_Unload(img->handle);
	free(img->filename);
	free(img);
}

VALUE Image_alloc(VALUE self)
{
	/* allocate */
	struct native_image* img = malloc(sizeof(struct native_image));

	/* wrap */
	return Data_Wrap_Struct(self, NULL, Image_free, img);
}

static void
rd_image(VALUE clazz, VALUE file, struct native_image *img, int bpp)
{
	char *filename;
	long f_len;
	FIBITMAP *h = NULL, *orig = NULL;

	file = rb_rescue(rb_String, file, file_arg_rescue, file);
	f_len = RSTRING_LEN(file);
	filename = malloc(f_len + 1);
	memcpy(filename, RSTRING_PTR(file), f_len);
	filename[f_len] = 0;

	orig = FreeImage_Load(FIF_JPEG, filename, 0);
	if (!orig)
		rb_raise(rb_eArgError, "Invalid image file");
	if (FreeImage_GetBPP(orig) == bpp) {
		h = orig;
	} else {
		switch(bpp) {
			case 8:
				h = FreeImage_ConvertTo8Bits(orig);
				break;
			case 24:
				h = FreeImage_ConvertTo24Bits(orig);
				break;
			case 32:
				h = FreeImage_ConvertTo32Bits(orig);
				break;
			default:
				rb_raise(rb_eArgError, "Invalid bpp");
		}
		FreeImage_Unload(orig);
	}
	img->handle = h;
	img->filename = filename;
	img->w = FreeImage_GetWidth(h);
	img->h = FreeImage_GetHeight(h);
	img->bpp = FreeImage_GetBPP(h);
	img->stride = FreeImage_GetLine(h);
}

VALUE Image_initialize(int argc, VALUE *argv, VALUE self)
{
	struct native_image* img;
	/* unwrap */
	Data_Get_Struct(self, struct native_image, img);

	memset(img, 0, sizeof(struct native_image));
	switch (argc)
	{
		case 1:
			rd_image(self, argv[0], img, 32);
			break;
		case 2:
			rd_image(self, argv[0], img, NUM2INT(argv[1]));
			break;
		default:
			rb_raise(rb_eArgError, "wrong number of arguments (%d for 1)", argc);
			break;
	}

	return self;
}

VALUE Image_save(VALUE self, VALUE file)
{
	char *filename;
	long f_len;
	FIBITMAP *h = NULL, *orig = NULL;
	struct native_image* img;
	BOOL result;

	Data_Get_Struct(self, struct native_image, img);

	file = rb_rescue(rb_String, file, file_arg_rescue, file);
	f_len = RSTRING_LEN(file);
	filename = malloc(f_len + 1);
	memcpy(filename, RSTRING_PTR(file), f_len);
	filename[f_len] = 0;

	result = FreeImage_Save(FIF_PNG, img->handle, filename, 0);
	if(!result)
		rb_raise(rb_eArgError, "Fail to save image");
	free(filename);
	return Qnil;
}

VALUE Image_cols(VALUE self)
{
	struct native_image* img;
	Data_Get_Struct(self, struct native_image, img);
	return INT2NUM(img->w);
}

VALUE Image_rows(VALUE self)
{
	struct native_image* img;
	Data_Get_Struct(self, struct native_image, img);
	return INT2NUM(img->h);
}

VALUE Image_bpp(VALUE self)
{
	struct native_image* img;
	Data_Get_Struct(self, struct native_image, img);
	return INT2NUM(img->bpp);
}

VALUE Image_stride(VALUE self)
{
	struct native_image* img;
	Data_Get_Struct(self, struct native_image, img);
	return INT2NUM(img->stride);
}

VALUE Image_bytes(VALUE self)
{
	struct native_image* img;
	Data_Get_Struct(self, struct native_image, img);
	return rb_str_new(FreeImage_GetBits(img->handle), img->stride * img->h);
}

void Init_rfreeimage(void)
{
	rb_mFI = rb_define_module("RFreeImage");
	rb_define_module_function(rb_mFI, "freeimage_version", rb_rfi_version, 0);
	rb_define_module_function(rb_mFI, "freeimage_string_version", rb_rfi_string_version, 0);

	Class_Image = rb_define_class_under(rb_mFI, "Image", rb_cObject);
	// rb_define_singleton_method(Class_Image, "read", Image_read, 1);
	rb_define_alloc_func(Class_Image, Image_alloc);
	rb_define_method(Class_Image, "initialize", Image_initialize, -1);
	rb_define_method(Class_Image, "cols", Image_cols, 0);
	rb_define_method(Class_Image, "rows", Image_rows, 0);
	rb_define_method(Class_Image, "bpp", Image_bpp, 0);
	rb_define_method(Class_Image, "stride", Image_stride, 0);
	rb_define_method(Class_Image, "bytes", Image_bytes, 0);
	rb_define_method(Class_Image, "save", Image_save, 1);
}


