require 'test/unit'
require 'rfreeimage'

class TestNativeVersion < Test::Unit::TestCase
	def test_version
		assert_equal RFreeImage.freeimage_version.size, 3
	end
end
