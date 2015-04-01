require "test/unit"
require 'tempfile'
require 'rfreeimage'
 
def get_image fn
	File.expand_path("../images/#{fn}", __FILE__)
end

def assert_dim img
	byte_per_pixel = img.bpp / 8
	assert img.stride % 4 == 0
	assert img.stride >= img.cols * byte_per_pixel
	assert_equal img.bytes.size, img.rows * img.cols * byte_per_pixel
end

include RFreeImage

class TestImageLoader < Test::Unit::TestCase
	def test_jpeg
		img = Image.new get_image("test.jpg")
		assert_equal img.cols, 500
		assert_equal img.rows, 588
		assert_equal img.bpp, 24
		assert_equal img.format, "JPEG"
		assert_dim img
	end

	def test_no_file
		assert_raise IOError do
			Image.new("XXX.jpg")
		end
	end

	def test_bad_text
		assert_raise IOError do
			Image.new(get_image "bad1.jpg")
		end
	end

	def test_bad_trunc
		img = Image.new(get_image "bad_trunc.jpg")
		assert_dim img
	end

	# TODO: find more malform image files
end

class TestImageMemoryManage < Test::Unit::TestCase
	def test_destroy
		img = Image.new get_image("test.jpg")
		assert_dim img
		assert img.bytes?
		img.destroy!
		assert_raise ImageError do
			img.bytes
		end
		assert !img.bytes?
	end

	def test_ping
		img = Image.ping get_image("test.jpg")
		assert_equal img.cols, 500
		assert_equal img.rows, 588
		assert_equal img.bpp, 24
		assert_equal img.format, "JPEG"
		assert !img.bytes?
	end
end

class TestImageBPP < Test::Unit::TestCase
	def setup
		@img = Image.new get_image("test.jpg")
	end

	def test_load_bpp
		[ImageBPP::GRAY, ImageBPP::BGR, ImageBPP::BGRA].each {|bpp|
			img = Image.new get_image("test.jpg"), bpp
			assert_equal img.bpp, bpp
		}
	end

	def test_to_bpp
		[ImageBPP::GRAY, ImageBPP::BGR, ImageBPP::BGRA].each {|bpp|
			img = @img.to_bpp bpp
			assert_equal img.bpp, bpp
			assert_dim img
		}
	end
end

class TestImageSave < Test::Unit::TestCase
	def setup
		@img = Image.new get_image("test.jpg")
		@w = @img.cols
		@h = @img.rows
		@img_2x = @img.resize @w * 2, @h * 2
	end

	def check_format ext, format
		ext = ext.gsub '.', ''
		v = case ext
		when "jpg", "jpeg"
			"JPEG"
		else
			ext.upcase
		end
		assert_equal format, v
	end

	def test_save_2x
		[".jpg", ".png", ".bmp", ".tiff"].each {|f|
			temp = Tempfile.new ['out', f]
			@img_2x.save temp.path
			info = Image.ping temp.path
			assert_equal info.cols, @w * 2
			assert_equal info.rows, @h * 2
			assert_equal info.bpp, @img.bpp
			check_format f, info.format
		}
	end
end

