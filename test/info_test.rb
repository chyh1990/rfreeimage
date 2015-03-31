require 'test/unit'
require 'rfreeimage'

class TestNativeVersion < Test::Unit::TestCase
	def test_version
		v1 = RFreeImage.freeimage_version
		v2 = RFreeImage.freeimage_string_version
		assert_equal v1.size, 3
		assert_equal v1.map(&:to_s), v2.split('.')
	end
end
