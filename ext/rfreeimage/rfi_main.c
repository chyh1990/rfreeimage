#include <ruby.h>
#include <FreeImage.h>

static VALUE rb_mFI;
//static VALUE rb_eFI;
static VALUE Class_Image;
static VALUE Class_RFIError;

__attribute__((constructor))
static void __rfi_module_init() {
	FreeImage_Initialise(FALSE);
}

__attribute__((destructor))
static void __rfi_module_uninit() {
	FreeImage_DeInitialise();
}

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
	FREE_IMAGE_FORMAT fif;
	FIBITMAP *handle;
};

void Image_free(struct native_image* img)
{
	if(!img)
		return;
	if(img->handle)
		FreeImage_Unload(img->handle);
	free(img);
}

VALUE Image_alloc(VALUE self)
{
	/* allocate */
	struct native_image* img = malloc(sizeof(struct native_image));
	memset(img, 0, sizeof(struct native_image));

	/* wrap */
	return Data_Wrap_Struct(self, NULL, Image_free, img);
}

static inline char *rfi_value_to_str(VALUE v)
{
	char *filename;
	long f_len;
	Check_Type(v, T_STRING);
	f_len = RSTRING_LEN(v);
	filename = malloc(f_len + 1);
	memcpy(filename, RSTRING_PTR(v), f_len);
	filename[f_len] = 0;
	return filename;
}

static FIBITMAP *
convert_bpp(FIBITMAP *orig, unsigned int bpp) {
	FIBITMAP *h = NULL;
	switch(bpp) {
		case 8:
			h = FreeImage_ConvertToGreyscale(orig);
			break;
		case 32:
			h = FreeImage_ConvertTo32Bits(orig);
			break;
	}
	return h;
}

static void
rd_image(VALUE clazz, VALUE file, struct native_image *img, unsigned int bpp, BOOL ping)
{
	char *filename;
	FIBITMAP *h = NULL, *orig = NULL;
	FREE_IMAGE_FORMAT in_fif;

	filename = rfi_value_to_str(file);

	in_fif = FreeImage_GetFileType(filename, 0);
	if (in_fif == FIF_UNKNOWN) {
		free(filename);
		rb_raise(rb_eIOError, "Invalid image file");
	}

	orig = FreeImage_Load(in_fif, filename, ping ? FIF_LOAD_NOPIXELS : 0 );
	free(filename);
	if (!orig)
		rb_raise(rb_eIOError, "Fail to load image file");
	if (ping) {
		h = orig;
	} else {
		if (bpp <= 0) bpp = 32;
		h = convert_bpp(orig, bpp);
		FreeImage_Unload(orig);
		if (!h) rb_raise(rb_eArgError, "Invalid bpp");
	}
	img->handle = h;
	img->w = FreeImage_GetWidth(h);
	img->h = FreeImage_GetHeight(h);
	img->bpp = FreeImage_GetBPP(h);
	img->stride = FreeImage_GetPitch(h);
	img->fif = in_fif;
}

static void
rd_image_blob(VALUE clazz, VALUE blob, struct native_image *img, unsigned int bpp, BOOL ping)
{
	FIBITMAP *h = NULL, *orig = NULL;
	FIMEMORY *fmh;
	FREE_IMAGE_FORMAT in_fif;

	Check_Type(blob, T_STRING);
	fmh = FreeImage_OpenMemory((BYTE*)RSTRING_PTR(blob), RSTRING_LEN(blob));

	in_fif = FreeImage_GetFileTypeFromMemory(fmh, 0);
	if (in_fif == FIF_UNKNOWN) {
		FreeImage_CloseMemory(fmh);
		rb_raise(rb_eIOError, "Invalid image blob");
	}

	orig = FreeImage_LoadFromMemory(in_fif, fmh, ping ? FIF_LOAD_NOPIXELS : 0 );
	FreeImage_CloseMemory(fmh);
	if (!orig)
		rb_raise(rb_eIOError, "Fail to load image from memory");

	if (ping) {
		h = orig;
	} else {
		if (bpp <= 0) bpp = 32;
		h = convert_bpp(orig, bpp);
		FreeImage_Unload(orig);
		if (!h) rb_raise(rb_eArgError, "Invalid bpp");
	}
	img->handle = h;
	img->w = FreeImage_GetWidth(h);
	img->h = FreeImage_GetHeight(h);
	img->bpp = FreeImage_GetBPP(h);
	img->stride = FreeImage_GetPitch(h);
	img->fif = in_fif;
}

VALUE Image_initialize(int argc, VALUE *argv, VALUE self)
{
	struct native_image* img;
	/* unwrap */
	Data_Get_Struct(self, struct native_image, img);

	switch (argc)
	{
		case 1:
			rd_image(self, argv[0], img, 0, 0);
			break;
		case 2:
			rd_image(self, argv[0], img, NUM2INT(argv[1]), 0);
			break;
		default:
			rb_raise(rb_eArgError, "wrong number of arguments (%d for 1)", argc);
			break;
	}

	return self;
}

#define RFI_CHECK_IMG(x) \
	if (!img->handle) rb_raise(Class_RFIError, "Image pixels not loaded");

VALUE Image_save(VALUE self, VALUE file)
{
	char *filename;
	struct native_image* img;
	BOOL result;
	FREE_IMAGE_FORMAT out_fif;

	Data_Get_Struct(self, struct native_image, img);
	RFI_CHECK_IMG(img);

	file = rb_rescue(rb_String, file, file_arg_rescue, file);
	filename = rfi_value_to_str(file);

	out_fif = FreeImage_GetFIFFromFilename(filename);
	if (out_fif == FIF_UNKNOWN) {
		free(filename);
		rb_raise(Class_RFIError, "Invalid format");
	}

	if (out_fif == FIF_JPEG && img->bpp != 8 && img->bpp != 24) {
		FIBITMAP *to_save = FreeImage_ConvertTo24Bits(img->handle);
		result = FreeImage_Save(out_fif, to_save, filename, 0);
		FreeImage_Unload(to_save);
	} else {
		result = FreeImage_Save(out_fif, img->handle, filename, 0);
	}

	free(filename);

	if(!result)
		rb_raise(rb_eIOError, "Fail to save image");
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

VALUE Image_format(VALUE self)
{
	struct native_image* img;
	const char *p;
	Data_Get_Struct(self, struct native_image, img);
	p = FreeImage_GetFormatFromFIF(img->fif);
	return rb_str_new(p, strlen(p));
}

VALUE Image_release(VALUE self)
{
	struct native_image* img;
	Data_Get_Struct(self, struct native_image, img);
	if (img->handle)
		FreeImage_Unload(img->handle);
	img->handle = NULL;
	return Qnil;
}


VALUE Image_read_bytes(VALUE self)
{
	struct native_image* img;
	const char *p;
	char *ptr;
	unsigned stride_dst;
	int i;
	VALUE v;

	Data_Get_Struct(self, struct native_image, img);
	RFI_CHECK_IMG(img);
	p = (const char*)FreeImage_GetBits(img->handle);
	stride_dst = img->w * (img->bpp / 8);
	v = rb_str_new(NULL, stride_dst * img->h);

	/* up-side-down */
	ptr = RSTRING_PTR(v) + img->h * stride_dst;
	for(i = 0; i < img->h; i++) {
		ptr -= stride_dst;
		memcpy(ptr, p, stride_dst);
		p += img->stride;
	}
	return v;
}

VALUE Image_buffer_addr(VALUE self)
{
	struct native_image* img;
	const char *p;

	Data_Get_Struct(self, struct native_image, img);
	RFI_CHECK_IMG(img);
	p = (const char*)FreeImage_GetBits(img->handle);
	return ULONG2NUM((uintptr_t)p);
}

VALUE Image_has_bytes(VALUE self)
{
	struct native_image* img;
	Data_Get_Struct(self, struct native_image, img);
	return img->handle ? Qtrue : Qfalse;
}


static inline VALUE rfi_get_image(FIBITMAP *nh)
{
	struct native_image *new_img;
	new_img = malloc(sizeof(struct native_image));
	memset(new_img, 0, sizeof(struct native_image));
	new_img->handle = nh;
	new_img->stride = FreeImage_GetPitch(nh);
	new_img->bpp = FreeImage_GetBPP(nh);
	new_img->w = FreeImage_GetWidth(nh);
	new_img->h = FreeImage_GetHeight(nh);

	return Data_Wrap_Struct(Class_Image, NULL, Image_free, new_img);
}

VALUE Image_to_bpp(VALUE self, VALUE _bpp)
{
	struct native_image *img;
	FIBITMAP *nh;
	int bpp = NUM2INT(_bpp);
	Data_Get_Struct(self, struct native_image, img);
	RFI_CHECK_IMG(img);

	nh = convert_bpp(img->handle, bpp);
	if (!nh) rb_raise(rb_eArgError, "Invalid bpp");

	return rfi_get_image(nh);
}

VALUE Image_rotate(VALUE self, VALUE _angle)
{
	struct native_image *img;
	FIBITMAP *nh;
	double angle = NUM2DBL(_angle);

	Data_Get_Struct(self, struct native_image, img);
	RFI_CHECK_IMG(img);
	nh = FreeImage_Rotate(img->handle, angle, NULL);
	return rfi_get_image(nh);
}

VALUE Image_clone(VALUE self)
{
	struct native_image *img;
	FIBITMAP *nh;

	Data_Get_Struct(self, struct native_image, img);
	RFI_CHECK_IMG(img);
	nh = FreeImage_Clone(img->handle);
	return rfi_get_image(nh);
}

VALUE Image_resize(VALUE self, VALUE dst_width, VALUE dst_height)
{
	struct native_image *img;
	FIBITMAP *nh;
	int w = NUM2INT(dst_width);
	int h = NUM2INT(dst_height);
	if (w <= 0 || h <= 0)
		rb_raise(rb_eArgError, "Invalid size");

	Data_Get_Struct(self, struct native_image, img);
	RFI_CHECK_IMG(img);

	nh = FreeImage_Rescale(img->handle, w, h, FILTER_CATMULLROM);
	return rfi_get_image(nh);
}

VALUE Image_crop(VALUE self, VALUE _left, VALUE _top, VALUE _right, VALUE _bottom)
{
	struct native_image *img;
	FIBITMAP *nh;
	int left = NUM2INT(_left);
	int top = NUM2INT(_top);
	int right = NUM2INT(_right);
	int bottom = NUM2INT(_bottom);
	Data_Get_Struct(self, struct native_image, img);
	RFI_CHECK_IMG(img);

	if (left < 0 || top < 0
		|| right < left || bottom < top
		|| bottom > img->h || right > img->w )
		rb_raise(rb_eArgError, "Invalid boundary");

	nh = FreeImage_Copy(img->handle, left, top, right, bottom);
	return rfi_get_image(nh);
}

#define ALLOC_NEW_IMAGE(__v, img) \
	VALUE __v = Image_alloc(Class_Image);            \
	struct native_image* img;                        \
	Data_Get_Struct(__v, struct native_image, img)   \

VALUE Image_ping(VALUE self, VALUE file)
{
	ALLOC_NEW_IMAGE(v, img);

	rd_image(self, file, img, 0, 1);
	if (img->handle)
		FreeImage_Unload(img->handle);
	img->handle = NULL;

	return v;
}

VALUE Image_from_blob(int argc, VALUE *argv, VALUE self)
{
	ALLOC_NEW_IMAGE(v, img);

	switch (argc)
	{
		case 1:
			rd_image_blob(self, argv[0], img, 0, 0);
			break;
		case 2:
			rd_image_blob(self, argv[0], img, NUM2INT(argv[1]), 0);
			break;
		default:
			rb_raise(rb_eArgError, "wrong number of arguments (%d for 1)", argc);
			break;
	}

	return v;
}

VALUE Image_ping_blob(VALUE self, VALUE blob)
{
	ALLOC_NEW_IMAGE(v, img);

	rd_image_blob(self, blob, img, 0, 1);
	if (img->handle)
		FreeImage_Unload(img->handle);
	img->handle = NULL;

	return v;
}

void Init_rfreeimage(void)
{
	rb_mFI = rb_define_module("RFreeImage");
	rb_define_module_function(rb_mFI, "freeimage_version", rb_rfi_version, 0);
	rb_define_module_function(rb_mFI, "freeimage_string_version", rb_rfi_string_version, 0);

	Class_Image = rb_define_class_under(rb_mFI, "Image", rb_cObject);
	Class_RFIError = rb_define_class_under(rb_mFI, "ImageError", rb_eStandardError);
	// rb_define_singleton_method(Class_Image, "read", Image_read, 1);
	rb_define_alloc_func(Class_Image, Image_alloc);
	rb_define_method(Class_Image, "initialize", Image_initialize, -1);
	rb_define_method(Class_Image, "cols", Image_cols, 0);
	rb_define_method(Class_Image, "rows", Image_rows, 0);
	rb_define_method(Class_Image, "bpp", Image_bpp, 0);
	rb_define_method(Class_Image, "stride", Image_stride, 0);
	rb_define_method(Class_Image, "format", Image_format, 0);
	rb_define_method(Class_Image, "buffer_addr", Image_buffer_addr, 0);
	rb_define_method(Class_Image, "read_bytes", Image_read_bytes, 0);
	rb_define_method(Class_Image, "bytes?", Image_has_bytes, 0);
	rb_define_method(Class_Image, "save", Image_save, 1);
	rb_define_method(Class_Image, "clone", Image_clone, 0);
	rb_define_method(Class_Image, "release", Image_release, 0);

	rb_define_method(Class_Image, "to_bpp", Image_to_bpp, 1);
	rb_define_method(Class_Image, "rotate", Image_rotate, 1);
	rb_define_method(Class_Image, "resize", Image_resize, 2);
	rb_define_method(Class_Image, "crop", Image_crop, 4);

	rb_define_singleton_method(Class_Image, "ping", Image_ping, 1);
	rb_define_singleton_method(Class_Image, "from_blob", Image_from_blob, -1);
	rb_define_singleton_method(Class_Image, "ping_blob", Image_ping_blob, 1);
}
