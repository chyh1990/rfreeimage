module RFreeImage
	module ImageBPP
		GRAY = 8
		BGR = 24
		BGRA = 32
	end
	module Color
		BLUE    = 0xff0000ff
		GREEN   = 0xff00ff00
		RED     = 0xffff0000
		YELLOW  = 0xffffff00
		CYAN    = 0xff00ffff
		WHITE   = 0xffffffff
		BLACK   = 0xff000000
		GRAY    = 0xffa9a9a9
	end
	class Image
		def bytes
			@bytes ||= read_bytes
		end

		def destroy!
			release
			@bytes = nil
		end

		def gray?
			bpp == ImageBPP::GRAY
		end

		def to_gray
			return self if gray?
			to_bpp 8
		end

		def bgra?
			bpp == ImageBPP::BGRA
		end

		def to_bgra
			return self if bgra?
			to_bpp 32
		end


		alias_method :write, :save
		alias_method :columns, :cols
	end
end
